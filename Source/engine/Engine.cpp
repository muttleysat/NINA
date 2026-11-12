#include "Engine.h"
#include <limits>

namespace sl
{
void Engine::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    maxBlock = juce::jmax (16, maxBlockSize);
    tailSamples = (int) (sr * 4.0);
    previewVoice.kill();
    for (auto& v : voices)
        v.kill();

    scratchL.assign ((size_t) maxBlock, 0.0f);
    scratchR.assign ((size_t) maxBlock, 0.0f);
    for (auto& c : cellFx)
    {
        if (c == nullptr)
            c = std::make_unique<FxChain>();
        c->prepare (sr, maxBlock);
    }
    cellTail.fill (0);
    lockMask.fill (0);
    masterFx.prepare (sr, maxBlock);
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
        lockMask.fill (0);                                       // i lock finiscono con lo stop
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

    // NoteOn: i lock di questo step valgono finche' la cella non riceve il prossimo step
    lockMask[(size_t) e.cell] = e.lockMask;
    lockValues[(size_t) e.cell] = e.lockValues;
    applyLocks();

    const auto& cell = current->cells[(size_t) e.cell];
    if (cell.data == nullptr)
        return;

    const auto d = liveDsp (e.cell);
    allocateVoice()->start (cell.data, d, e.cell, d.rootNote, e.velocity, sampleRate, ++ageCounter, e.semis, true);
}

void Engine::applyLocks() noexcept
{
    for (int c = 0; c < kMaxCells; ++c)
    {
        const auto mask = lockMask[(size_t) c];
        if (mask == 0)
            continue;
        for (int f = 0; f < (int) FxF::Count; ++f)
            if ((mask >> f) & 1u)
                setFxFloat (effFx[(size_t) c], (FxF) f, lockValues[(size_t) c][(size_t) f]);
    }
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

// Renderizza le voci cella per cella: le celle con effetti passano da un buffer dedicato e dalla propria catena.
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
        const auto& fx = effFx[(size_t) c];

        if (! fx.anyOn())
        {
            cellTail[(size_t) c] = 0;
            if (voiceOn)
                for (auto& v : voices)
                    if (v.isActive() && v.getCellIndex() == c)
                        v.render (l, r, n);
            continue;
        }

        if (! voiceOn && cellTail[(size_t) c] <= 0)
            continue;

        std::fill (scratchL.begin(), scratchL.begin() + n, 0.0f);
        std::fill (scratchR.begin(), scratchR.begin() + n, 0.0f);
        if (voiceOn)
            for (auto& v : voices)
                if (v.isActive() && v.getCellIndex() == c)
                    v.render (scratchL.data(), scratchR.data(), n);

        cellFx[(size_t) c]->process (scratchL.data(), scratchR.data(), n, fx, live.bpm);

        for (int i = 0; i < n; ++i)
        {
            l[i] += scratchL[(size_t) i];
            r[i] += scratchR[(size_t) i];
        }

        cellTail[(size_t) c] = voiceOn ? (fx.hasTail() ? tailSamples : 0) : cellTail[(size_t) c] - n;
    }
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

    // Effetti del blocco: valori correnti dei parametri + lock degli step ancora attivi
    effFx = live.fx;
    applyLocks();

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

    // Effetti globali del kit, poi volume master
    masterFx.process (l, r, numSamples, live.masterFx, live.bpm);

    if (live.masterGain != 1.0f)
        buffer.applyGain (live.masterGain);

    for (auto& a : activity)
        a.store (0, std::memory_order_relaxed);
    for (const auto& v : voices)
        if (v.isActive() && v.getCellIndex() >= 0 && v.getCellIndex() < kMaxCells)
            activity[(size_t) v.getCellIndex()].fetch_add (1, std::memory_order_relaxed);
}
}
