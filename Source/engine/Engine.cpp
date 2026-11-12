#include "Engine.h"
#include <limits>

namespace sl
{
namespace
{
// Interruttore di un effetto per una voce: il lock dello step (se c'e') altrimenti il valore della cella
bool effectOn (const Voice& v, FxB b, bool cellValue) noexcept
{
    const int i = lockIndex (b);
    return v.hasLock (i) ? v.lockValue (i) >= 0.5f : cellValue;
}

float lockedOr (const Voice& v, FxF f, float cellValue) noexcept
{
    return v.hasLock ((int) f) ? v.lockValue ((int) f) : cellValue;
}

// Parametri degli effetti per colpo di una voce: valori della cella (live) con i lock del suo step sopra
InsertParams voiceInsert (const FxParams& cell, const Voice& v) noexcept
{
    InsertParams p = InsertParams::from (cell);
    p.satOn = effectOn (v, FxB::Sat, cell.satOn);
    p.lofiOn = effectOn (v, FxB::Lofi, cell.lofiOn);
    p.filterOn = effectOn (v, FxB::Filter, cell.filterOn);
    p.drive = lockedOr (v, FxF::Drive, cell.drive);
    p.satMix = lockedOr (v, FxF::SatMix, cell.satMix);
    p.bits = lockedOr (v, FxF::Bits, cell.bits);
    p.downsample = lockedOr (v, FxF::Downsample, cell.downsample);
    p.lofiMix = lockedOr (v, FxF::LofiMix, cell.lofiMix);
    p.cutoff = lockedOr (v, FxF::Cutoff, cell.cutoff);
    p.resonance = lockedOr (v, FxF::Resonance, cell.resonance);
    return p;
}

// Mandate della voce al delay e al riverbero (0 = niente)
float delaySendOf (const FxParams& cell, const Voice& v) noexcept
{
    return effectOn (v, FxB::Delay, cell.delayOn) ? juce::jlimit (0.0f, 1.0f, lockedOr (v, FxF::DelayMix, cell.delayMix)) : 0.0f;
}

float reverbSendOf (const FxParams& cell, const Voice& v) noexcept
{
    return effectOn (v, FxB::Reverb, cell.reverbOn) ? juce::jlimit (0.0f, 1.0f, lockedOr (v, FxF::ReverbMix, cell.reverbMix)) : 0.0f;
}

void zero (std::vector<float>& a, int n) noexcept { std::fill (a.begin(), a.begin() + n, 0.0f); }
}

void Engine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    maxBlock = juce::jmax (16, maxBlockSize);
    tailSamples = (int) (sr * 4.0);
    previewVoice.kill();
    for (auto& v : voices)
    {
        v.kill();
        v.prepareFx (sr);
    }

    for (auto* buf : { &busL, &busR, &voiceL, &voiceR, &sendDL, &sendDR, &sendRL, &sendRR })
        buf->assign ((size_t) maxBlock, 0.0f);

    for (auto& c : cellFx)
    {
        if (c == nullptr)
            c = std::make_unique<FxChain>();
        c->prepare (sr, maxBlock);
    }
    delayTail.fill (0);
    reverbTail.fill (0);
    masterFx.prepare (sr, maxBlock);
    masterInsert.prepare (sr);
    masterDelayWasOn = masterReverbWasOn = false;
}

void Engine::playPreview (const SampleData* data, float gain)
{
    previewGain.store (gain, std::memory_order_relaxed);

    auto snap = std::make_shared<KitSnapshot>();
    if (data != nullptr)
    {
        CellRuntime r;
        r.cellIndex = -1;
        r.data = data;
        snap->cells.push_back (r);
    }
    previewExchange.publish (std::move (snap));
}

void Engine::stopPreview()
{
    playPreview (nullptr, 1.0f);
}

// Parametri live della cella (automazione) + mapping delle note preso dallo snapshot
CellDsp Engine::liveDsp (int cell) const noexcept
{
    CellDsp d = live.dsp[(size_t) cell];
    if (current != nullptr && cell < (int) current->cells.size())
    {
        const auto& c = current->cells[(size_t) cell];
        d.rootNote = c.rootNote;
        d.lowNote = juce::jmin (c.lowNote, c.highNote);
        d.highNote = juce::jmax (c.lowNote, c.highNote);
    }
    return d;
}

// Voce libera, altrimenti si ruba la piu' vecchia
Voice* Engine::allocateVoice() noexcept
{
    for (auto& v : voices)
        if (! v.isActive())
            return &v;

    Voice* oldest = &voices[0];
    for (auto& v : voices)
        if (v.getAge() < oldest->getAge())
            oldest = &v;
    return oldest;
}

void Engine::noteOn (int note, float velocity) noexcept
{
    if (current == nullptr)
        return;

    for (const auto& cell : current->cells)
    {
        const int c = cell.cellIndex;
        if (cell.data == nullptr || c < 0 || c >= kMaxCells)
            continue;

        const auto d = liveDsp (c);
        if (note < d.lowNote || note > d.highNote)
            continue;

        allocateVoice()->start (cell.data, d, c, note, velocity, sampleRate, ++ageCounter);
    }
}

void Engine::noteOff (int note) noexcept
{
    for (auto& v : voices)
        if (v.isActive() && ! v.isFromSequencer() && v.getNote() == note && v.getMode() != TriggerMode::OneShot)
            v.release();
}

// Eventi del sequencer: suonano direttamente la cella (senza passare dal mapping delle note)
void Engine::seqEvent (const SeqEvent& e) noexcept
{
    if (e.type == SeqEvent::Type::Stop)
    {
        for (auto& v : voices)
            if (v.isActive() && v.isFromSequencer())
                v.release();
        return;
    }

    if (e.cell < 0 || e.cell >= kMaxCells || current == nullptr || e.cell >= (int) current->cells.size())
        return;

    if (e.type == SeqEvent::Type::NoteOff)
    {
        for (auto& v : voices)
            if (v.isActive() && v.isFromSequencer() && v.getCellIndex() == e.cell && v.getMode() != TriggerMode::OneShot)
                v.release();
        return;
    }

    const auto& cell = current->cells[(size_t) e.cell];
    if (cell.data == nullptr)
        return;

    const auto d = liveDsp (e.cell);
    auto* v = allocateVoice();
    v->start (cell.data, d, e.cell, d.rootNote, e.velocity, sampleRate, ++ageCounter, e.semis, true, e.reverse, e.mode);
    v->setLocks (e.lockMask, e.lockValues);      // i lock valgono solo per questo colpo
}

void Engine::renderVoices (float* l, float* r, int start, int count) noexcept
{
    while (count > 0)
    {
        const int n = juce::jmin (count, maxBlock);
        renderChunk (l + start, r + start, n);
        start += n;
        count -= n;
    }
}

// Cella per cella: ogni voce passa dai suoi effetti per colpo, poi la cella somma le voci, applica il compressore e
// aggiunge delay e riverbero alimentati dalle mandate delle voci.
void Engine::renderChunk (float* l, float* r, int n) noexcept
{
    if (current == nullptr)
        return;

    std::array<bool, kMaxCells> hasVoice {};
    for (const auto& v : voices)
        if (v.isActive() && v.getCellIndex() >= 0 && v.getCellIndex() < kMaxCells)
            hasVoice[(size_t) v.getCellIndex()] = true;

    const int numCells = juce::jmin ((int) current->cells.size(), (int) kMaxCells);
    for (int c = 0; c < numCells; ++c)
    {
        const bool voiceOn = hasVoice[(size_t) c];
        if (! voiceOn && delayTail[(size_t) c] <= 0 && reverbTail[(size_t) c] <= 0)
            continue;

        const auto& fx = effFx[(size_t) c];
        auto& chain = *cellFx[(size_t) c];

        zero (busL, n); zero (busR, n);
        zero (sendDL, n); zero (sendDR, n);
        zero (sendRL, n); zero (sendRR, n);
        bool delaySent = false, reverbSent = false;

        for (auto& v : voices)
        {
            if (! v.isActive() || v.getCellIndex() != c)
                continue;

            const auto ip = voiceInsert (fx, v);
            const float dS = delaySendOf (fx, v), rS = reverbSendOf (fx, v);

            if (! ip.any() && dS <= 0.0f && rS <= 0.0f)
            {
                v.render (busL.data(), busR.data(), n);          // percorso diretto: nessun effetto per questa voce
                continue;
            }

            zero (voiceL, n); zero (voiceR, n);
            v.render (voiceL.data(), voiceR.data(), n);
            if (ip.any())
                v.insert.process (voiceL.data(), voiceR.data(), n, ip);

            for (int i = 0; i < n; ++i)
            {
                busL[(size_t) i] += voiceL[(size_t) i];
                busR[(size_t) i] += voiceR[(size_t) i];
                sendDL[(size_t) i] += voiceL[(size_t) i] * dS;
                sendDR[(size_t) i] += voiceR[(size_t) i] * dS;
                sendRL[(size_t) i] += voiceL[(size_t) i] * rS;
                sendRR[(size_t) i] += voiceR[(size_t) i] * rS;
            }
            delaySent = delaySent || dS > 0.0f;
            reverbSent = reverbSent || rS > 0.0f;
        }

        chain.compress (busL.data(), busR.data(), n, fx);

        // Delay e riverbero restano attivi finche' dura la coda dell'ultima mandata: l'eco di un colpo risuona anche dopo la nota
        if (delaySent) delayTail[(size_t) c] = tailSamples;
        if (reverbSent) reverbTail[(size_t) c] = tailSamples;

        if (delayTail[(size_t) c] > 0)
        {
            chain.delay (sendDL.data(), sendDR.data(), busL.data(), busR.data(), n, fx, live.bpm);
            if (! delaySent && (delayTail[(size_t) c] -= n) <= 0)
                chain.resetDelay();
        }
        if (reverbTail[(size_t) c] > 0)
        {
            chain.reverbFx (sendRL.data(), sendRR.data(), busL.data(), busR.data(), n, fx);
            if (! reverbSent && (reverbTail[(size_t) c] -= n) <= 0)
                chain.resetReverb();
        }

        for (int i = 0; i < n; ++i)
        {
            l[i] += busL[(size_t) i];
            r[i] += busR[(size_t) i];
        }
    }
}

// Effetti del master: saturazione/LoFi/filtro sul mix, compressore, poi delay e riverbero con "Mix" come mandata
void Engine::renderMaster (float* l, float* r, int n) noexcept
{
    const auto& fx = live.masterFx;

    const auto ip = InsertParams::from (fx);
    if (ip.any())
        masterInsert.process (l, r, n, ip);
    else
        masterInsert.reset();

    masterFx.compress (l, r, n, fx);

    if (fx.delayOn)
    {
        if (! masterDelayWasOn)
            masterFx.resetDelay();
        const float send = juce::jlimit (0.0f, 1.0f, fx.delayMix);
        for (int i = 0; i < n; ++i) { sendDL[(size_t) i] = l[i] * send; sendDR[(size_t) i] = r[i] * send; }
        masterFx.delay (sendDL.data(), sendDR.data(), l, r, n, fx, live.bpm);
    }
    masterDelayWasOn = fx.delayOn;

    if (fx.reverbOn)
    {
        if (! masterReverbWasOn)
            masterFx.resetReverb();
        const float send = juce::jlimit (0.0f, 1.0f, fx.reverbMix);
        for (int i = 0; i < n; ++i) { sendRL[(size_t) i] = l[i] * send; sendRR[(size_t) i] = r[i] * send; }
        masterFx.reverbFx (sendRL.data(), sendRR.data(), l, r, n, fx);
    }
    masterReverbWasOn = fx.reverbOn;
}

void Engine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                      const std::vector<SeqEvent>* seq) noexcept
{
    if (exchange.tryAcquire (current) && current != nullptr && current->generation != lastGeneration)
    {
        for (auto& v : voices)
            v.kill();
        lastGeneration = current->generation;
    }

    if (buffer.getNumChannels() < 1)
        return;

    const int numSamples = buffer.getNumSamples();
    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;

    // Anteprima del browser: una voce dedicata, indipendente dal kit
    if (previewExchange.tryAcquire (previewCurrent) && previewCurrent != nullptr)
    {
        previewVoice.kill();
        if (! previewCurrent->cells.empty())
        {
            CellDsp d;
            d.gain = previewGain.load (std::memory_order_relaxed);
            d.rootNote = d.lowNote = d.highNote = 60;
            d.attack = 0.001f;
            d.release = 0.02f;
            previewVoice.start (previewCurrent->cells[0].data, d, -1, 60, 1.0f, sampleRate, 0);
        }
    }
    previewVoice.render (l, r, numSamples);

    // Meter master: picco del blocco dopo il guadagno master (calcolato anche senza kit)
    struct PeakGuard
    {
        Engine& e; juce::AudioBuffer<float>& b;
        ~PeakGuard()
        {
            e.peakL.store (b.getMagnitude (0, 0, b.getNumSamples()), std::memory_order_relaxed);
            e.peakR.store (b.getNumChannels() > 1 ? b.getMagnitude (1, 0, b.getNumSamples()) : b.getMagnitude (0, 0, b.getNumSamples()),
                           std::memory_order_relaxed);
        }
    } peakGuard { *this, buffer };

    if (current == nullptr)
        return;

    // Le voci in corso seguono subito i valori automatizzati / le manopole
    for (auto& v : voices)
        if (v.isActive() && v.getCellIndex() >= 0 && v.getCellIndex() < kMaxCells)
            v.updateDsp (liveDsp (v.getCellIndex()));

    effFx = live.fx;

    // Eventi MIDI ed eventi del sequencer, fusi in ordine di tempo: si renderizza a segmenti tra un evento e il successivo
    constexpr int kNone = std::numeric_limits<int>::max();
    auto midiIt = midi.begin();
    const auto midiEnd = midi.end();
    size_t seqIdx = 0;
    const size_t seqCount = seq != nullptr ? seq->size() : 0;

    int pos = 0;
    for (;;)
    {
        const int nextMidi = midiIt != midiEnd ? (*midiIt).samplePosition : kNone;
        const int nextSeq = seqIdx < seqCount ? (*seq)[seqIdx].samplePos : kNone;
        if (nextMidi == kNone && nextSeq == kNone)
            break;

        const bool takeSeq = nextSeq <= nextMidi;
        const int at = juce::jlimit (pos, numSamples, takeSeq ? nextSeq : nextMidi);
        renderVoices (l, r, pos, at - pos);
        pos = at;

        if (takeSeq)
        {
            seqEvent ((*seq)[seqIdx++]);
        }
        else
        {
            const auto m = (*midiIt).getMessage();
            ++midiIt;
            if (m.isNoteOn())
                noteOn (m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff())
                noteOff (m.getNoteNumber());
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                for (auto& v : voices)
                    v.release();
        }
    }
    renderVoices (l, r, pos, numSamples - pos);

    // Effetti globali del kit (a blocchi di maxBlock), poi volume master
    for (int done = 0; done < numSamples; done += maxBlock)
        renderMaster (l + done, r + done, juce::jmin (maxBlock, numSamples - done));

    if (live.masterGain != 1.0f)
        buffer.applyGain (live.masterGain);

    for (auto& a : activity)
        a.store (0, std::memory_order_relaxed);
    for (const auto& v : voices)
        if (v.isActive() && v.getCellIndex() >= 0 && v.getCellIndex() < kMaxCells)
            activity[(size_t) v.getCellIndex()].fetch_add (1, std::memory_order_relaxed);
}
}
