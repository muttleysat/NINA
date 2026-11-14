#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "sequencer/SequencerExport.h"
#include <set>

// Main + 15 uscite stereo aggiuntive (spente di default: le attiva la DAW). Le celle si assegnano alle uscite dal pannello della cella.
juce::AudioProcessor::BusesProperties SampleLabProcessor::makeBuses()
{
    auto buses = BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true);
    for (int i = 2; i <= sl::Engine::kMaxOutputs; ++i)
        buses = buses.withOutput ("Out " + juce::String (i), juce::AudioChannelSet::stereo(), false);
    return buses;
}

SampleLabProcessor::SampleLabProcessor()
    : AudioProcessor (makeBuses())
{
    // Parametri esposti alla DAW (automazione): gruppi per cella ed effetto, nomi leggibili, ID stabili
    for (auto& group : params.releaseGroups())
        addParameterGroup (std::move (group));

    for (auto* p : params.all())
        p->addListener (&dirtyListener);

    formats.registerBasicFormats();
    cellData.resize (kit.cells.size());
    tempoTimer.fn = [this] { tempoTick(); };
    tempoTimer.startTimer (300);
    publishKit (true);
    params.readFromKit (kit);
    dirty = false;
}

SampleLabProcessor::~SampleLabProcessor()
{
    alive->store (false);
    tempoTimer.stopTimer();
    stretchPool.removeAllJobs (true, 5000);
    for (auto* p : params.all())
        p->removeListener (&dirtyListener);
}

void SampleLabProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    seqRunner.reset();
    engine.prepare (sampleRate, juce::jmax (512, samplesPerBlock));
    keyboardState.reset();
}

bool SampleLabProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i)
        if (! layouts.outputBuses[i].isDisabled() && layouts.outputBuses[i] != juce::AudioChannelSet::stereo())
            return false;
    return true;
}

void SampleLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // Chop: le note vengono registrate e tolte dal flusso (non suonano le celle); gli altri messaggi (CC...) passano
    if (chopCapture.load (std::memory_order_relaxed) && ! midi.isEmpty())
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        const double sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        juce::MidiBuffer rest;
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
            {
                const auto h = chopHead.load (std::memory_order_relaxed);
                chopTimes[h % chopTimes.size()].store (now + 1000.0 * (double) meta.samplePosition / sr, std::memory_order_relaxed);
                chopHead.store (h + 1, std::memory_order_release);
            }
            else                                           // note-off compresi: chiudono le note gia' tenute prima del Chop
                rest.addEvent (m, meta.samplePosition);
        }
        midi.swapWith (rest);
    }

    // MIDI learn: i CC assegnati muovono i parametri (prima della lettura dei valori, nello stesso blocco)
    midiLearn.process (midi);

    // Valori correnti dei parametri (manopole, automazione della DAW) letti a ogni blocco: l'automazione
    // e' quindi applicata anche nell'export offline, senza passare dal thread dell'interfaccia.
    params.fillLive (engine.live);

    // Tempo: dalla DAW se disponibile, altrimenti il BPM interno (standalone)
    double bpm = internalBpm.load();
    bool fromHost = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostBpm = pos->getBpm(); hostBpm.hasValue() && *hostBpm > 0.0)
            {
                bpm = *hostBpm;
                fromHost = true;
            }
    if (fromHost)
        lastHostBpm.store (bpm, std::memory_order_relaxed);
    else if (wrapperType != juce::AudioProcessor::wrapperType_Standalone && lastHostBpm.load (std::memory_order_relaxed) > 0.0)
    {
        bpm = lastHostBpm.load (std::memory_order_relaxed);        // la DAW non manda il tempo a trasporto fermo: si tiene l'ultimo ricevuto
        fromHost = true;
    }
    engine.live.bpm = bpm;
    currentBpm.store (bpm, std::memory_order_relaxed);
    hostTempo.store (fromHost, std::memory_order_relaxed);

    // Sequencer: pattern attivo (pubblicato dal thread UI) e trasporto. Nel plugin si segue la DAW (play/stop e posizione),
    // nello standalone il trasporto e' interno.
    const bool newPatternMsg = patternExchange.tryAcquire (audioMsg);
    songExchange.tryAcquire (songMsg);

    const int numSamples = buffer.getNumSamples();
    // Trasporto: se la DAW sta suonando il sequencer la segue; altrimenti (DAW ferma o standalone) parte dal tasto Play
    // del plugin con il trasporto interno, cosi' si puo' provare il pattern senza avviare Cubase.
    bool playing = seqRunning.load();
    double ppq0 = internalPpq;
    bool useHostPpq = false;
    bool hostRollingNow = false;
    const auto mode = getHostSeqMode();
    if (mode != HostSeqMode::Ignore)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto hostPpq = pos->getPpqPosition(); hostPpq.hasValue() && pos->getIsPlaying())
                {
                    if (mode == HostSeqMode::MuteOnRecord && pos->getIsRecording())
                    {
                        playing = false;                    // la DAW registra: il sequencer sta zitto (non si mischia a cio' che suoni)
                        hostRollingNow = true;              // ...ma il trasporto e' della DAW: il tasto Play del plugin resta inattivo
                    }
                    else
                    {
                        useHostPpq = true;
                        hostRollingNow = true;
                        playing = true;
                        ppq0 = *hostPpq;
                    }
                }
    hostRolling.store (hostRollingNow, std::memory_order_relaxed);

    if (! useHostPpq)
    {
        if (playing && ! wasRunning)                        // Play dall'inizio; con la registrazione accesa prima c'e' il conteggio iniziale
        {
            const bool inSong = songMode.load (std::memory_order_relaxed);
            const bool countIn = seqRecord.load (std::memory_order_relaxed) && ! inSong;
            internalPpq = ppq0 = inSong ? songStart.load (std::memory_order_relaxed)           // song: dal blocco scelto
                                        : (countIn ? -4.0 * (double) countInBars.load (std::memory_order_relaxed) : 0.0);
        }
        wasRunning = playing;
        if (playing)
            internalPpq += (double) numSamples * bpm / (60.0 * currentSampleRate);
    }

    // Quale pattern suona: da fermo il pattern selezionato; in riproduzione si finisce il ciclo del pattern in corso e si passa
    // a quello selezionato al punto di cambio successivo (multiplo della durata del ciclo corrente).
    if (audioMsg != nullptr && (newPatternMsg || ! playing || curPat == nullptr))
    {
        const auto& m = *audioMsg;
        if (! playing || curIndex < 0)
        {
            curPat = &m.shown;
            curIndex = m.shownIndex;
            nextPat = nullptr;
            nextIndex = -1;
        }
        else if (m.hasPlay)
        {
            if (curIndex == m.shownIndex)                   // il cambio e' gia' avvenuto nel frattempo
            {
                curPat = &m.shown;
                nextPat = nullptr;
                nextIndex = -1;
            }
            else
            {
                curPat = &m.play;
                curIndex = m.playIndex;
                nextPat = &m.shown;
                nextIndex = m.shownIndex;
            }
        }
        else
        {
            curPat = &m.shown;
            curIndex = m.shownIndex;
            nextPat = nullptr;
            nextIndex = -1;
        }
    }

    const sl::Pattern* switchTo = nullptr;
    double switchPpq = 0.0;
    if (playing && curPat != nullptr && nextPat != nullptr)
    {
        const double length = juce::jmax (0.25, curPat->lengthBeats());
        const double ppq1 = ppq0 + (double) numSamples * bpm / (60.0 * currentSampleRate);
        const double boundary = std::ceil (ppq0 / length - 1.0e-9) * length;      // primo multiple del ciclo da qui in avanti
        if (boundary < ppq1 - 1.0e-9)
        {
            if (boundary <= ppq0 + 1.0e-9)
            {
                curPat = nextPat;                           // il cambio cade proprio all'inizio del blocco
                curIndex = nextIndex;
                nextPat = nullptr;
                nextIndex = -1;
            }
            else
            {
                switchTo = nextPat;                         // il cambio cade dentro il blocco: preciso al campione
                switchPpq = boundary;
            }
        }
    }

    const bool useSong = songMode.load (std::memory_order_relaxed) && songMsg != nullptr && ! songMsg->segs.empty();
    if (useSong)
    {
        // Song: la catena di pattern. Senza loop, alla fine il trasporto interno si ferma (e riparte dall'inizio al prossimo Play)
        if (playing && ! songMsg->loop && ppq0 >= songMsg->total)
        {
            playing = false;
            if (! useHostPpq)
                seqRunning.store (false);
        }
        seqRunner.runBlockSong (songMsg->segs, songMsg->loop ? songMsg->total : 0.0, playing, ppq0, bpm, currentSampleRate, numSamples,
                                songMsg->loop ? std::numeric_limits<double>::infinity() : songMsg->total);
    }
    else
    {
        seqRunner.runBlockSwitch (curPat, switchTo, switchPpq, playing, ppq0, bpm, currentSampleRate, numSamples);
        if (switchTo != nullptr)
        {
            curPat = switchTo;
            curIndex = nextIndex;
            nextPat = nullptr;
            nextIndex = -1;
        }
    }
    seqPlaying.store (playing, std::memory_order_relaxed);
    seqPlayIndex.store (playing && ! useSong ? curIndex : -1, std::memory_order_relaxed);
    seqPpq.store (ppq0, std::memory_order_relaxed);

    buffer.clear();

    // Uscite aggiuntive attive: il buffer contiene i canali dei bus accesi, in ordine (Main = 0/1)
    {
        int ch = 2;
        for (int k = 1; k < sl::Engine::kMaxOutputs; ++k)
        {
            auto* bus = getBus (false, k);
            if (bus != nullptr && bus->isEnabled())
            {
                engine.outputMap[(size_t) k] = ch;
                ch += bus->getNumberOfChannels();
            }
            else
                engine.outputMap[(size_t) k] = -1;
        }
    }
    // Registrazione: ogni note-on con la sua posizione musicale (il trasporto deve girare)
    if (playing && seqRecord.load (std::memory_order_relaxed) && ! songMode.load (std::memory_order_relaxed) && ! midi.isEmpty() && bpm > 0.0
        && currentSampleRate > 0.0)
    {
        const double perSample = bpm / (60.0 * currentSampleRate);
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (! m.isNoteOn())
                continue;
            if (ppq0 + (double) meta.samplePosition * perSample < -0.5)
                continue;                                   // durante il conteggio iniziale non si registra (tranne l'anticipo sul primo quarto)
            const auto head = recHead.load (std::memory_order_relaxed);
            recRing[head % recRing.size()] = RecNote { ppq0 + (double) meta.samplePosition * perSample, m.getNoteNumber(), (int) m.getVelocity() };
            recHead.store (head + 1, std::memory_order_release);
        }
    }

    engine.process (buffer, midi, &seqRunner.getEvents());

    // Metronomo / conteggio iniziale: un click a ogni quarto (accento sul primo della battuta), sommato alle uscite Main
    {
        const bool counting = playing && ppq0 < 0.0;
        int trig[256];
        bool accent[256];
        int nTrig = 0;
        if (playing && bpm > 0.0 && (clickOn.load (std::memory_order_relaxed) || counting))
        {
            const double ppq1 = ppq0 + (double) numSamples * bpm / (60.0 * currentSampleRate);
            const double samplesPerBeat = 60.0 * currentSampleRate / bpm;
            for (double b = std::ceil (ppq0 - 1.0e-9); b < ppq1 - 1.0e-9 && nTrig < 256; b += 1.0)
            {
                if (! clickOn.load (std::memory_order_relaxed) && b >= 0.0)
                    break;                                  // metronomo spento: solo i quarti del conteggio iniziale
                trig[nTrig] = juce::jlimit (0, numSamples - 1, (int) ((b - ppq0) * samplesPerBeat));
                accent[nTrig] = (((long long) std::llround (b) % 4) + 4) % 4 == 0;
                ++nTrig;
            }
        }

        if (nTrig > 0 || clickLeft > 0)
        {
            const int chans = juce::jmin (2, buffer.getNumChannels());
            const float clickGain = clickVol.load (std::memory_order_relaxed) * 1.5f;
            int next = 0;
            for (int i = 0; i < numSamples; ++i)
            {
                while (next < nTrig && trig[next] == i)
                {
                    clickTotal = clickLeft = (int) (0.025 * currentSampleRate);          // 25 ms
                    clickFreq = accent[next] ? 1760.0f : 1175.0f;
                    clickAmp = accent[next] ? 0.5f : 0.34f;
                    clickPhase = 0.0f;
                    ++next;
                }
                if (clickLeft > 0)
                {
                    const float env = (float) clickLeft / (float) clickTotal;
                    const float v = std::sin (clickPhase) * clickAmp * env * env * clickGain;
                    clickPhase += juce::MathConstants<float>::twoPi * clickFreq / (float) currentSampleRate;
                    --clickLeft;
                    for (int ch = 0; ch < chans; ++ch)
                        buffer.getWritePointer (ch)[i] += v;
                }
            }
        }
    }
}

int SampleLabProcessor::drainRecorded (RecNote* out, int maxCount)
{
    const auto head = recHead.load (std::memory_order_acquire);
    if (head - recTail > (uint32_t) recRing.size())
        recTail = head - (uint32_t) recRing.size();
    int n = 0;
    while (recTail != head && n < maxCount)
        out[n++] = recRing[recTail++ % recRing.size()];
    return n;
}

juce::AudioProcessorEditor* SampleLabProcessor::createEditor()
{
    return new SampleLabEditor (*this);
}

// ------------------------------------------------------------- Anteprima

void SampleLabProcessor::auditionFile (const juce::File& file, float gainDb)
{
    const int token = ++auditionToken;

    auditionPool.addJob ([this, file, gainDb, token]
    {
        if (token != auditionToken.load())
            return;                                       // nel frattempo e' stato scelto un altro file

        auto data = sl::loadSampleFile (formats, file);
        if (data == nullptr || token != auditionToken.load())
            return;

        {
            const juce::ScopedLock lock (auditionLock);
            auditionKeep.push_back (data);
            while (auditionKeep.size() > 8)               // la voce legge solo l'ultimo: gli altri sono sicuri da liberare
                auditionKeep.pop_front();
        }
        engine.playPreview (data.get(), sl::dbToGain (gainDb));
    });
}

void SampleLabProcessor::auditionCellRegion (int cell, float start, float end, float gainDb)
{
    if (cell < 0 || cell >= (int) cellData.size() || cellData[(size_t) cell] == nullptr)
        return;
    auditionData (cellData[(size_t) cell], start, end, gainDb);
}

sl::SampleDataPtr SampleLabProcessor::loadAudioFile (const juce::File& file)
{
    return file.existsAsFile() ? sl::loadSampleFile (formats, file) : nullptr;
}

void SampleLabProcessor::auditionData (sl::SampleDataPtr data, float start, float end, float gainDb)
{
    if (data == nullptr)
        return;
    {
        const juce::ScopedLock lock (auditionLock);
        auditionKeep.push_back (data);                      // la voce legge dal buffer: deve restare vivo
        while (auditionKeep.size() > 8)
            auditionKeep.pop_front();
    }
    ++auditionToken;                                         // annulla un'eventuale anteprima di file in caricamento
    engine.playPreview (data.get(), sl::dbToGain (gainDb), start, end);
}

int SampleLabProcessor::distributeSliceData (sl::SampleDataPtr data, const juce::String& baseName, const std::vector<float>& bounds,
                                              int firstTarget, float bpm, bool sync, const juce::File& dir)
{
    if (data == nullptr || bounds.size() < 2 || firstTarget < 0 || data->numSamples() <= 0)
        return 0;
    dir.createDirectory();

    const int n = data->numSamples();
    const int chans = juce::jlimit (1, 2, data->numChannels());
    int filled = 0;
    for (int i = 0; i + 1 < (int) bounds.size(); ++i)
    {
        const int target = firstTarget + i;
        if (target >= kit.numCells())
            break;

        const int a = juce::jlimit (0, n, (int) std::floor ((double) bounds[(size_t) i] * (double) n));
        const int b = juce::jlimit (a, n, (int) std::floor ((double) bounds[(size_t) i + 1] * (double) n));
        if (b - a < 32)
            continue;

        const auto name = baseName + " " + juce::String (i + 1).paddedLeft ('0', 2);
        const auto file = dir.getChildFile (sl::KitIO::sanitiseName (name) + ".wav");
        file.deleteFile();
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
            if (stream == nullptr)
                break;
            std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), data->sampleRate, (unsigned int) chans, 24, {}, 0));
            if (writer == nullptr)
                break;
            stream.release();                                      // il writer ne e' proprietario
            writer->writeFromAudioSampleBuffer (data->audio, a, b - a);
        }

        sampleCache.erase (file.getFullPathName());
        juce::String err;
        if (! loadSampleIntoCell (target, file, err))
            break;

        auto& c = kit.cells[(size_t) target];
        c.name = name;
        c.slices.clear();
        c.tempoSync = sync;
        c.loopBpm = bpm;
        c.rootNote = c.lowNote = c.highNote = juce::jlimit (0, 127, sl::Kit::kFirstNote + target);   // tasto proprio: nessuna trasposizione residua
        for (const auto f : { sl::CellF::Tune, sl::CellF::Pitch })                                     // le slice suonano all'intonazione originale
            params.cell (target).at (f)->setValueNotifyingHost (static_cast<juce::RangedAudioParameter*> (params.cell (target).at (f))->getDefaultValue());
        cellEdited (target);
        ++filled;
    }
    return filled;
}

int SampleLabProcessor::distributeSlices (int sourceCell, const std::vector<float>& bounds, int firstTarget)
{
    if (sourceCell < 0 || sourceCell >= kit.numCells() || bounds.size() < 2 || firstTarget < 0 || ! kit.cells[(size_t) sourceCell].hasSample())
        return 0;

    const auto path = kit.cells[(size_t) sourceCell].samplePath;
    const auto baseName = kit.cells[(size_t) sourceCell].name;
    // Le slice ereditano il tempo del loop: il BPM originale (stimato se manca), Sync e Key lock. Lo stretch riguarda tutto il file
    // e Start/End sono posizioni relative, quindi ogni slice segue il tempo del progetto restando al suo posto.
    float loopBpm = kit.cells[(size_t) sourceCell].loopBpm;
    if (loopBpm <= 0.0f && cellData[(size_t) sourceCell] != nullptr)
    {
        const auto& sp = params.cell (sourceCell);
        loopBpm = (float) sl::estimateLoopBpm (*cellData[(size_t) sourceCell], sp.at (sl::CellF::Start)->get(), sp.at (sl::CellF::End)->get());
        kit.cells[(size_t) sourceCell].loopBpm = loopBpm;      // anche la cella originale mostra il valore stimato (modificabile)
    }
    const bool srcSync = kit.cells[(size_t) sourceCell].tempoSync, srcKeyLock = kit.cells[(size_t) sourceCell].keyLock;
    const juce::File file (path);
    int filled = 0;

    for (int i = 0; i + 1 < (int) bounds.size(); ++i)
    {
        const int target = firstTarget + i;
        if (target >= kit.numCells())
            break;

        juce::String err;
        if (! loadSampleIntoCell (target, file, err))       // stesso file (in cache): nessuna copia dei dati
            break;

        auto& c = kit.cells[(size_t) target];
        c.name = baseName + " " + juce::String (i + 1).paddedLeft ('0', 2);
        c.slices.clear();
        c.tempoSync = srcSync;
        c.keyLock = srcKeyLock;
        c.loopBpm = loopBpm;
        c.rootNote = c.lowNote = c.highNote = juce::jlimit (0, 127, sl::Kit::kFirstNote + target);   // tasto proprio: nessuna trasposizione residua
        for (const auto f : { sl::CellF::Tune, sl::CellF::Pitch })                                     // intonazione come quella del loop
            params.cell (target).at (f)->setValueNotifyingHost (static_cast<juce::RangedAudioParameter*> (params.cell (sourceCell).at (f))->getValue());
        const auto& refs = params.cell (target);
        refs.at (sl::CellF::Start)->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, bounds[(size_t) i]));
        refs.at (sl::CellF::End)->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, bounds[(size_t) i + 1]));
        cellEdited (target);
        ++filled;
    }
    return filled;
}

void SampleLabProcessor::stopAudition()
{
    ++auditionToken;
    engine.stopPreview();
}

// ---------------------------------------------------------------- Kit

std::shared_ptr<sl::KitSnapshot> SampleLabProcessor::buildSnapshot() const
{
    auto snap = std::make_shared<sl::KitSnapshot>();
    snap->generation = generation;
    snap->cells.reserve (kit.cells.size());

    for (int i = 0; i < kit.numCells(); ++i)
    {
        const auto& c = kit.cells[(size_t) i];
        sl::CellRuntime r;
        r.cellIndex = i;
        r.data = (size_t) i < playData.size() ? playData[(size_t) i].get() : nullptr;
        r.rootNote = c.rootNote;
        r.lowNote = c.lowNote;
        r.highNote = c.highNote;
        r.output = c.output;
        snap->cells.push_back (r);
    }
    return snap;
}

void SampleLabProcessor::publishKit (bool newGeneration)
{
    if (newGeneration)
        ++generation;

    ensurePlayData();

    engine.publish (buildSnapshot());
    publishPattern();
}

// ------------------------------------------------------------- Loop e stretch

void SampleLabProcessor::ensurePlayData()
{
    playData.resize (cellData.size());
    stretch.resize (cellData.size());
    for (size_t i = 0; i < cellData.size(); ++i)
    {
        auto& st = stretch[i];
        if (st.source != cellData[i])                         // campione cambiato (o cella svuotata): si riparte dall'originale
        {
            st.source = cellData[i];
            playData[i] = cellData[i];
            st.sync = false;
            st.loopBpm = st.bpm = st.pendingLoopBpm = st.pendingBpm = 0.0;
            ++st.token;
            st.retired.clear();
        }
        refreshStretch ((int) i);
    }
}

void SampleLabProcessor::refreshStretch (int cell)
{
    if (cell < 0 || cell >= (int) stretch.size() || cell >= kit.numCells())
        return;

    auto& st = stretch[(size_t) cell];
    const auto& c = kit.cells[(size_t) cell];
    const double bpm = std::round (currentBpm.load() * 100.0) / 100.0;
    const double loopBpm = (double) c.loopBpm;
    const bool wantStretch = c.tempoSync && loopBpm > 20.0 && bpm > 20.0 && st.source != nullptr;

    if (! wantStretch)
    {
        if (playData[(size_t) cell] != st.source)
        {
            st.retired.push_back (playData[(size_t) cell]);
            playData[(size_t) cell] = st.source;
            while (st.retired.size() > 6)
                st.retired.pop_front();
            st.sync = false;
        }
        ++st.token;                                            // annulla eventuali elaborazioni in corso
        st.pendingBpm = st.pendingLoopBpm = 0.0;
        return;
    }

    const bool sameAsApplied = st.sync && st.keyLock == c.keyLock && std::abs (st.loopBpm - loopBpm) < 0.005 && std::abs (st.bpm - bpm) < 0.05;
    const bool sameAsPending = st.pendingBpm > 0.0 && st.pendingKeyLock == c.keyLock && std::abs (st.pendingLoopBpm - loopBpm) < 0.005
                               && std::abs (st.pendingBpm - bpm) < 0.05;
    if (sameAsApplied || sameAsPending)
        return;

    const int token = ++st.token;
    st.pendingBpm = bpm;
    st.pendingLoopBpm = loopBpm;
    st.pendingKeyLock = c.keyLock;

    // Celle con lo stesso file (le slice di un loop) e gli stessi parametri condividono un solo risultato: si riusa quello
    // gia' pronto, oppure si aspetta quello in elaborazione (il job lo applica a tutte le celle in attesa)
    for (size_t j = 0; j < stretch.size(); ++j)
    {
        if ((int) j == cell || stretch[j].source != st.source)
            continue;
        auto& o = stretch[j];
        if (o.sync && o.keyLock == c.keyLock && std::abs (o.loopBpm - loopBpm) < 0.005 && std::abs (o.bpm - bpm) < 0.05 && o.pendingBpm == 0.0)
        {
            st.retired.push_back (playData[(size_t) cell]);
            while (st.retired.size() > 6)
                st.retired.pop_front();
            playData[(size_t) cell] = playData[j];
            st.sync = true;
            st.keyLock = c.keyLock;
            st.bpm = bpm;
            st.loopBpm = loopBpm;
            st.pendingBpm = st.pendingLoopBpm = 0.0;
            engine.publish (buildSnapshot());
            return;
        }
        if (o.pendingBpm > 0.0 && o.pendingKeyLock == c.keyLock && std::abs (o.pendingLoopBpm - loopBpm) < 0.005 && std::abs (o.pendingBpm - bpm) < 0.05)
            return;                                            // un'altra cella sta gia' elaborando lo stesso risultato
    }

    const double ratio = loopBpm / bpm;                       // durata nuova / durata originale
    const double pitch = c.keyLock ? 1.0 : 1.0 / ratio;       // senza Key Lock cambia anche l'altezza (varispeed)
    const bool keyLock = c.keyLock;
    auto source = st.source;
    auto flag = alive;

    stretchPool.addJob ([this, cell, token, ratio, pitch, source, keyLock, bpm, loopBpm, flag]
    {
        auto result = sl::stretchSample (*source, ratio, pitch);
        juce::MessageManager::callAsync ([this, cell, token, result, source, keyLock, bpm, loopBpm, flag]
        {
            if (! flag->load() || cell >= (int) stretch.size())
                return;
            auto applyTo = [&] (size_t k)
            {
                auto& s = stretch[k];
                s.pendingBpm = s.pendingLoopBpm = 0.0;
                if (result == nullptr)
                    return;
                s.retired.push_back (playData[k]);
                while (s.retired.size() > 6)
                    s.retired.pop_front();
                playData[k] = result;
                s.sync = true;
                s.keyLock = keyLock;
                s.bpm = bpm;
                s.loopBpm = loopBpm;
            };

            bool any = false;
            if (stretch[(size_t) cell].token == token && stretch[(size_t) cell].source == source)   // altrimenti e' obsoleto per questa cella
            {
                applyTo ((size_t) cell);
                any = true;
            }
            for (size_t k = 0; k < stretch.size(); ++k)       // celle in attesa dello stesso risultato (stesso file e parametri)
            {
                const auto& w = stretch[k];
                if ((int) k != cell && w.source == source && w.pendingBpm > 0.0 && w.pendingKeyLock == keyLock
                    && std::abs (w.pendingLoopBpm - loopBpm) < 0.005 && std::abs (w.pendingBpm - bpm) < 0.05)
                {
                    applyTo (k);
                    any = true;
                }
            }
            if (any)
                engine.publish (buildSnapshot());
        });
    });
}

bool SampleLabProcessor::isStretching (int cell) const
{
    return cell >= 0 && cell < (int) stretch.size() && stretch[(size_t) cell].pendingBpm > 0.0;
}

void SampleLabProcessor::tempoTick()
{
    for (int i = 0; i < (int) stretch.size(); ++i)
        if (i < kit.numCells() && kit.cells[(size_t) i].tempoSync)
            refreshStretch (i);
}

void SampleLabProcessor::detectLoopBpm (int cell)
{
    if (cell < 0 || cell >= kit.numCells() || cellData[(size_t) cell] == nullptr)
        return;
    auto& c = kit.cells[(size_t) cell];
    const auto& refs = params.cell (cell);
    auto* startP = refs.at (sl::CellF::Start);
    auto* endP = refs.at (sl::CellF::End);
    c.loopBpm = (float) sl::estimateLoopBpm (*cellData[(size_t) cell], startP->get(), endP->get());
    cellEdited (cell);
}

// ------------------------------------------------------------- Sequencer

void SampleLabProcessor::publishPattern()
{
    auto msg = std::make_shared<PatternMsg>();
    msg->shown = activePattern();
    msg->shownIndex = kit.activePattern;

    // Se il sequencer sta suonando un altro pattern, lo si manda anche: continua fino alla fine del ciclo, poi parte quello scelto
    const int playing = seqPlayIndex.load (std::memory_order_relaxed);
    if (seqPlaying.load (std::memory_order_relaxed) && playing >= 0 && playing < sl::kNumPatterns && playing != kit.activePattern)
    {
        msg->hasPlay = true;
        msg->play = kit.patterns[(size_t) playing];
        msg->playIndex = playing;
    }
    patternExchange.publish (std::move (msg));
    if (songMode.load())
        publishSong();
}

void SampleLabProcessor::publishSong()
{
    auto msg = std::make_shared<SongMsg>();
    msg->pats = kit.patterns;
    msg->segs = sl::makeSegments (kit.song, msg->pats);
    msg->total = kit.song.totalBeats (msg->pats);
    msg->loop = kit.song.loop;
    songExchange.publish (std::move (msg));
}

void SampleLabProcessor::songEdited()
{
    dirty = true;
    publishSong();
}

void SampleLabProcessor::setSongMode (bool on)
{
    songMode.store (on);
    if (on)
        publishSong();
}

bool SampleLabProcessor::saveSongFile (const juce::File& file, juce::String& error) const
{
    if (! file.replaceWithText (sl::KitIO::songToJson (kit.song)))
    {
        error = "Impossibile scrivere " + file.getFullPathName();
        return false;
    }
    return true;
}

bool SampleLabProcessor::loadSongFile (const juce::File& file, juce::String& error)
{
    sl::Song s;
    if (! file.existsAsFile() || ! sl::KitIO::songFromJson (file.loadFileAsString(), s, error))
    {
        if (error.isEmpty())
            error = "Impossibile leggere " + file.getFullPathName();
        return false;
    }
    noteUserInput();
    kit.song = s;
    songEdited();
    return true;
}

// Piano di esportazione della song: copia dei pattern (con, se richiesto, solo le righe di una cella) e relativi segmenti
void SampleLabProcessor::songPlan (std::vector<sl::Pattern>& pats, std::vector<sl::Segment>& segs, double& total, int isolateCell) const
{
    pats = kit.patterns;
    if (isolateCell >= 0)
        for (auto& p : pats)
            for (auto& l : p.lanes)
            {
                l.mute = l.cell != isolateCell;
                l.solo = false;
            }
    segs = sl::makeSegments (kit.song, pats);
    total = kit.song.totalBeats (pats);
}

bool SampleLabProcessor::exportSongMidi (const juce::File& file, juce::String& error)
{
    std::vector<sl::Pattern> pats;
    std::vector<sl::Segment> segs;
    double total = 0.0;
    songPlan (pats, segs, total, -1);
    if (segs.empty() || total <= 0.0)
    {
        error = "The song is empty.";
        return false;
    }
    auto midi = sl::SequencerExport::toMidiFileSegments (kit.song.name, segs, total, kit, currentBpm.load());
    if (! sl::SequencerExport::writeMidiFile (midi, file))
    {
        error = "Impossibile scrivere " + file.getFullPathName();
        return false;
    }
    return true;
}

bool SampleLabProcessor::bounceSongWav (const juce::File& file, juce::String& error)
{
    if (! exportTempoKnown())
    {
        error = tempoUnknownMessage();
        return false;
    }
    std::vector<sl::Pattern> pats;
    std::vector<sl::Segment> segs;
    double total = 0.0;
    songPlan (pats, segs, total, -1);
    if (segs.empty() || total <= 0.0)
    {
        error = "The song is empty.";
        return false;
    }

    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
    const double bpm = currentBpm.load();
    auto eng = std::make_unique<sl::Engine>();
    eng->prepare (sr, 512);
    eng->publish (buildSnapshot());
    params.fillLive (eng->live);
    eng->live.bpm = bpm;

    const auto tail = exportTail.load();
    auto out = sl::SequencerExport::renderBounceSegments (*eng, segs, total, bpm, sr, tail);
    int numSamples = out.getNumSamples();
    if (tail == sl::SequencerExport::TailMode::Keep)
    {
        int lastAudible = numSamples;
        while (lastAudible > 0 && out.getMagnitude (0, lastAudible - 1, 1) < 1.0e-5f && out.getMagnitude (1, lastAudible - 1, 1) < 1.0e-5f)
            --lastAudible;
        numSamples = juce::jmin (numSamples, lastAudible + (int) (0.05 * sr));
    }
    return writeWav24 (file, out, sr, numSamples, error);
}

// Un WAV per ogni cella suonata dalla song (tutti della stessa lunghezza), con gli effetti della cella ma senza gli effetti master
int SampleLabProcessor::exportSongStems (const juce::File& folder, juce::String& error)
{
    if (! exportTempoKnown())
    {
        error = tempoUnknownMessage();
        return -1;
    }
    std::set<int> cells;
    for (const auto& b : kit.song.blocks)
        if (b.pattern >= 0 && b.pattern < (int) kit.patterns.size())
            for (const auto& l : kit.patterns[(size_t) b.pattern].lanes)
                if (l.cell >= 0 && l.cell < kit.numCells())
                    cells.insert (l.cell);

    if (! folder.createDirectory().wasOk())
    {
        error = "Impossibile creare la cartella " + folder.getFullPathName();
        return -1;
    }

    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
    const double bpm = currentBpm.load();
    int written = 0;
    std::set<juce::String> used;
    for (const int cell : cells)
    {
        std::vector<sl::Pattern> pats;
        std::vector<sl::Segment> segs;
        double total = 0.0;
        songPlan (pats, segs, total, cell);
        if (segs.empty() || total <= 0.0)
            continue;

        auto eng = std::make_unique<sl::Engine>();
        eng->prepare (sr, 512);
        eng->publish (buildSnapshot());
        params.fillLive (eng->live);
        eng->live.bpm = bpm;
        eng->live.masterFx = sl::FxParams();
        eng->live.mute.fill (false);                                // le tracce separate non seguono Mute/Solo del mixer
        eng->live.solo.fill (false);
        eng->live.masterGain = 1.0f;

        auto audio = sl::SequencerExport::renderBounceSegments (*eng, segs, total, bpm, sr, exportTail.load());

        juce::String base = sl::KitIO::sanitiseName (sl::cellLabel (cell, kit.cols) + "_" + (kit.cells[(size_t) cell].name.isEmpty() ? juce::String ("cell") : kit.cells[(size_t) cell].name));
        juce::String name = base;
        for (int n = 2; used.count (name) > 0; ++n)
            name = base + "_" + juce::String (n);
        used.insert (name);

        if (! writeWav24 (folder.getChildFile (name + ".wav"), audio, sr, audio.getNumSamples(), error))
            return -1;
        ++written;
    }
    return written;
}

void SampleLabProcessor::patternEdited()
{
    dirty = true;
    publishPattern();
}

void SampleLabProcessor::setActivePattern (int index)
{
    kit.activePattern = juce::jlimit (0, sl::kNumPatterns - 1, index);
    dirty = true;
    publishPattern();
}

namespace
{
// Copia il contenuto (righe, swing, seed...) in un altro pattern senza toccarne il nome
void copyPatternContent (const sl::Pattern& from, sl::Pattern& to)
{
    const auto name = to.name;
    to = from;
    to.name = name;
}
}

bool SampleLabProcessor::duplicateActivePattern()
{
    const int n = sl::kNumPatterns;
    for (int k = 1; k < n; ++k)
    {
        const int target = (kit.activePattern + k) % n;
        if (kit.patterns[(size_t) target].lanes.empty())
        {
            copyPatternContent (activePattern(), kit.patterns[(size_t) target]);
            kit.activePattern = target;
            dirty = true;
            publishPattern();
            return true;
        }
    }
    return false;
}

void SampleLabProcessor::copyActivePattern()
{
    patternClipboard = activePattern();
}

void SampleLabProcessor::pasteIntoActivePattern()
{
    if (! patternClipboard.has_value())
        return;
    copyPatternContent (*patternClipboard, activePattern());
    dirty = true;
    publishPattern();
}

void SampleLabProcessor::clearActivePattern()
{
    auto& p = activePattern();
    const auto name = p.name;
    p = sl::Pattern();
    p.name = name;
    dirty = true;
    publishPattern();
}

void SampleLabProcessor::renamePattern (int index, const juce::String& name)
{
    if (index < 0 || index >= sl::kNumPatterns)
        return;
    kit.patterns[(size_t) index].name = name.trim().isEmpty() ? juce::String ("Pattern " + juce::String (index + 1)) : name.trim();
    dirty = true;
}

bool SampleLabProcessor::addLaneForCell (int cell)
{
    if (cell < 0 || cell >= kit.numCells() || activePattern().findLane (cell) != nullptr)
        return false;

    sl::Lane lane;
    lane.cell = cell;
    activePattern().lanes.push_back (lane);
    patternEdited();
    return true;
}

void SampleLabProcessor::removeLaneForCell (int cell)
{
    auto& lanes = activePattern().lanes;
    lanes.erase (std::remove_if (lanes.begin(), lanes.end(), [cell] (const sl::Lane& l) { return l.cell == cell; }), lanes.end());
    patternEdited();
}

void SampleLabProcessor::setSequencerRunning (bool run)
{
    seqRunning.store (run);
}

bool SampleLabProcessor::exportPatternMidi (const juce::File& file, int bars, juce::String& error)
{
    auto midi = sl::SequencerExport::toMidiFile (activePattern(), kit, bars, currentBpm.load());
    if (! sl::SequencerExport::writeMidiFile (midi, file))
    {
        error = "Impossibile scrivere " + file.getFullPathName();
        return false;
    }
    return true;
}

// Rendering offline del pattern attivo: stesso motore, stessi parametri correnti (inclusi effetti, lock, tail).
// L'automazione della DAW non e' inclusa: si usano i valori correnti dei parametri.
bool SampleLabProcessor::bouncePatternWav (const juce::File& file, int bars, juce::String& error)
{
    if (! exportTempoKnown())
    {
        error = tempoUnknownMessage();
        return false;
    }
    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
    const double bpm = currentBpm.load();
    constexpr int block = 512;

    auto eng = std::make_unique<sl::Engine>();
    eng->prepare (sr, block);
    eng->publish (buildSnapshot());
    params.fillLive (eng->live);
    eng->live.bpm = bpm;

    const auto tail = exportTail.load();
    auto out = sl::SequencerExport::renderBounce (*eng, activePattern(), bars, bpm, sr, tail);
    int numSamples = out.getNumSamples();

    // Solo con la coda intera: si taglia il silenzio finale oltre la coda. Con Cut/Loop la lunghezza resta esattamente quella delle battute.
    if (tail == sl::SequencerExport::TailMode::Keep)
    {
        int lastAudible = numSamples;
        while (lastAudible > 0 && out.getMagnitude (0, lastAudible - 1, 1) < 1.0e-5f && out.getMagnitude (1, lastAudible - 1, 1) < 1.0e-5f)
            --lastAudible;
        numSamples = juce::jmin (numSamples, lastAudible + (int) (0.05 * sr));
    }

    return writeWav24 (file, out, sr, numSamples, error);
}

bool SampleLabProcessor::writeWav24 (const juce::File& file, const juce::AudioBuffer<float>& audio, double sr, int numSamples, juce::String& error)
{
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
    {
        error = "Impossibile creare " + file.getFullPathName();
        return false;
    }
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
    if (writer == nullptr)
    {
        error = "Formato WAV non disponibile";
        return false;
    }
    stream.release();                                                  // il writer ne e' proprietario
    return writer->writeFromAudioSampleBuffer (audio, 0, numSamples);
}

// Un file WAV per ogni riga del pattern (le "tracce" separate), con gli effetti della sua cella (filtro, saturazione, send,
// delay sincronizzato al tempo, riverbero e relative code) ma senza gli effetti master. Tutti della stessa lunghezza,
// per allinearli in Cubase.
int SampleLabProcessor::exportPatternStems (const juce::File& folder, int bars, juce::String& error)
{
    if (! exportTempoKnown())
    {
        error = tempoUnknownMessage();
        return -1;
    }
    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
    const double bpm = currentBpm.load();
    const auto& pattern = activePattern();

    if (! folder.createDirectory().wasOk())
    {
        error = "Impossibile creare la cartella " + folder.getFullPathName();
        return -1;
    }

    int written = 0;
    std::set<juce::String> used;
    for (int i = 0; i < (int) pattern.lanes.size(); ++i)
    {
        const int cell = pattern.lanes[(size_t) i].cell;
        if (cell < 0 || cell >= kit.numCells())
            continue;

        auto eng = std::make_unique<sl::Engine>();
        eng->prepare (sr, 512);
        eng->publish (buildSnapshot());
        params.fillLive (eng->live);
        eng->live.bpm = bpm;
        eng->live.masterFx = sl::FxParams();
        eng->live.mute.fill (false);                                // le tracce separate non seguono Mute/Solo del mixer
        eng->live.solo.fill (false);                        // niente effetti master negli stem: si rimixano nella DAW
        eng->live.masterGain = 1.0f;

        auto audio = sl::SequencerExport::renderBounce (*eng, sl::SequencerExport::isolateLane (pattern, i), bars, bpm, sr, exportTail.load());

        juce::String base = sl::KitIO::sanitiseName (sl::cellLabel (cell, kit.cols) + "_" + (kit.cells[(size_t) cell].name.isEmpty() ? juce::String ("cell") : kit.cells[(size_t) cell].name));
        juce::String name = base;
        for (int n = 2; used.count (name) > 0; ++n)
            name = base + "_" + juce::String (n);
        used.insert (name);

        if (! writeWav24 (folder.getChildFile (name + ".wav"), audio, sr, audio.getNumSamples(), error))
            return -1;
        ++written;
    }
    return written;
}

sl::SampleDataPtr SampleLabProcessor::getCellData (int cell) const
{
    return (cell >= 0 && cell < (int) cellData.size()) ? cellData[(size_t) cell] : nullptr;
}

bool SampleLabProcessor::loadSampleIntoCell (int cell, const juce::File& file, juce::String& error)
{
    noteUserInput();
    if (cell < 0 || cell >= kit.numCells())
        return false;

    const auto key = file.getFullPathName();
    sl::SampleDataPtr data;
    if (auto it = sampleCache.find (key); it != sampleCache.end())
        data = it->second;
    else
    {
        data = sl::loadSampleFile (formats, file);
        if (data == nullptr)
        {
            error = "Impossibile leggere il file audio: " + file.getFileName();
            return false;
        }
        sampleCache[key] = data;
    }

    auto& c = kit.cells[(size_t) cell];
    c.samplePath = key;
    c.name = file.getFileNameWithoutExtension();
    c.missing = false;
    cellData[(size_t) cell] = data;

    c.loopBpm = (float) sl::estimateLoopBpm (*data);          // stima iniziale (modificabile): il loop si sincronizza solo se si accende Sync
    c.tempoSync = false;

    // Un nuovo campione riparte con la regione intera (Start/End sono parametri automatizzabili)
    const auto& refs = params.cell (cell);
    refs.at (sl::CellF::Start)->setValueNotifyingHost (0.0f);
    refs.at (sl::CellF::End)->setValueNotifyingHost (1.0f);

    cellEdited (cell);
    return true;
}

void SampleLabProcessor::clearCell (int cell)
{
    if (cell < 0 || cell >= kit.numCells())
        return;

    auto& c = kit.cells[(size_t) cell];
    c.samplePath = {};
    c.name = {};
    c.missing = false;
    cellData[(size_t) cell] = nullptr;
    // Una cella svuotata torna al suo tasto di partenza: un range esteso non deve continuare a coprire i pad vicini
    c.rootNote = c.lowNote = c.highNote = juce::jlimit (0, 127, sl::Kit::kFirstNote + cell);
    c.output = 0;
    c.slices.clear();
    c.tempoSync = false;
    c.loopBpm = 0.0f;

    const auto& refs = params.cell (cell);
    refs.at (sl::CellF::Start)->setValueNotifyingHost (0.0f);
    refs.at (sl::CellF::End)->setValueNotifyingHost (1.0f);
    cellEdited (cell);
}

void SampleLabProcessor::resetKeyRange (int cell)
{
    if (cell < 0 || cell >= kit.numCells())
        return;
    auto& c = kit.cells[(size_t) cell];
    c.rootNote = c.lowNote = c.highNote = juce::jlimit (0, 127, sl::Kit::kFirstNote + cell);
    cellEdited (cell);
}

float SampleLabProcessor::normalizeCell (int cell, float targetDb)
{
    const auto data = getCellData (cell);
    if (data == nullptr || cell < 0 || cell >= sl::kNumCells)
        return 0.0f;

    const auto& refs = params.cell (cell);
    const float peak = sl::samplePeak (*data, refs.at (sl::CellF::Start)->get(), refs.at (sl::CellF::End)->get());
    if (peak < 1.0e-5f)
        return 0.0f;

    // Il Level della cella ha un intervallo limitato (-60..+12 dB): un campione molto basso puo' non arrivare al target
    auto* level = refs.at (sl::CellF::Level);
    const auto range = level->getNormalisableRange();
    const float gain = juce::jlimit (range.start, range.end, sl::normalizeGainDb (peak, targetDb));
    level->setValueNotifyingHost (level->convertTo0to1 (gain));
    dirty = true;
    return gain;
}

int SampleLabProcessor::normalizeAllCells (float targetDb)
{
    int count = 0;
    for (int i = 0; i < kit.numCells(); ++i)
        if (getCellData (i) != nullptr && sl::samplePeak (*getCellData (i)) >= 1.0e-5f)
        {
            normalizeCell (i, targetDb);
            ++count;
        }
    return count;
}

// ------------------------------------------------------------- Annulla / ripeti

namespace
{
inline void mixHash (juce::uint64& h, juce::uint64 v) { h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); }
inline juce::uint64 bitsOf (float f) { juce::uint32 u = 0; std::memcpy (&u, &f, sizeof u); return u; }
}

namespace
{
juce::uint64 hashPattern (const sl::Pattern& p)
{
    juce::uint64 h = 1099511628211ULL;
    mixHash (h, (juce::uint64) p.name.hashCode64());
    mixHash (h, bitsOf (p.swing));
    mixHash (h, p.seed);
    for (const auto& l : p.lanes)
    {
        mixHash (h, (juce::uint64) (l.cell + 2) * 131u + (juce::uint64) l.division * 7u + (juce::uint64) l.length * 1009u);
        mixHash (h, (l.mute ? 1u : 0u) | (l.solo ? 2u : 0u) | (l.dry ? 4u : 0u));
        for (const auto& st : l.steps)
        {
            mixHash (h, (st.on ? 1u : 0u) | (st.reverse ? 2u : 0u) | ((juce::uint64) st.velocity << 8) | ((juce::uint64) st.probability << 16)
                            | ((juce::uint64) st.ratchet << 24) | ((juce::uint64) (st.pitch + 64) << 32) | ((juce::uint64) (st.mode + 2) << 40));
            if (! st.on && st.lockMask == 0 && st.nudge == 0.0f)
                continue;                                    // step spento e senza modifiche: il resto non conta
            mixHash (h, bitsOf (st.nudge));
            mixHash (h, bitsOf (st.gate));
            mixHash (h, st.lockMask);
            for (const float v : st.lockValues)
                mixHash (h, bitsOf (v));
        }
    }
    return h;
}
}

std::vector<juce::uint64> SampleLabProcessor::patternHashes() const
{
    std::vector<juce::uint64> out;
    out.reserve (kit.patterns.size());
    for (const auto& p : kit.patterns)
        out.push_back (hashPattern (p));
    return out;
}

const std::vector<juce::RangedAudioParameter*>& SampleLabProcessor::undoParams() const
{
    if (undoParamList.empty())
        undoParamList = params.all();
    return undoParamList;
}

juce::uint64 SampleLabProcessor::hashUndoState() const
{
    juce::uint64 h = 1469598103934665603ULL;
    for (size_t i = 0; i < kit.cells.size(); ++i)
    {
        const auto& cl = kit.cells[i];
        mixHash (h, (juce::uint64) cl.name.hashCode64());
        mixHash (h, (juce::uint64) cl.samplePath.hashCode64());
        mixHash (h, cl.colour);
        mixHash (h, (juce::uint64) ((cl.rootNote * 1000003) + (cl.lowNote * 1009) + cl.highNote));
        mixHash (h, (juce::uint64) (cl.output + 1));
        mixHash (h, (cl.tempoSync ? 1u : 0u) | (cl.keyLock ? 2u : 0u));
        mixHash (h, bitsOf (cl.loopBpm));
        mixHash (h, (juce::uint64) cl.slices.size());
        for (const float v : cl.slices)
            mixHash (h, bitsOf (v));
        mixHash (h, i < cellData.size() ? (juce::uint64) (std::uintptr_t) cellData[i].get() : 0u);
    }
    for (auto* p : undoParams())
        mixHash (h, bitsOf (p->getValue()));
    for (const auto ph : patternHashes())
        mixHash (h, ph);
    mixHash (h, kit.song.loop ? 1u : 0u);
    mixHash (h, (juce::uint64) kit.song.name.hashCode64());
    for (const auto& b : kit.song.blocks)
        mixHash (h, (juce::uint64) (b.pattern * 1000 + b.repeats + 7));
    return h;
}

SampleLabProcessor::UndoSnap SampleLabProcessor::captureUndo() const
{
    UndoSnap s;
    s.cells = kit.cells;
    s.data = cellData;
    s.params.reserve (undoParams().size());
    for (auto* p : undoParams())
        s.params.push_back (p->getValue());
    s.song = kit.song;
    s.patHashes = patternHashes();
    patCache.resize (kit.patterns.size());
    for (size_t i = 0; i < kit.patterns.size(); ++i)
    {
        auto& pc = patCache[i];
        if (pc.ptr == nullptr || pc.hash != s.patHashes[i])
        {
            pc.ptr = std::make_shared<const sl::Pattern> (kit.patterns[i]);
            pc.hash = s.patHashes[i];
        }
        s.patterns.push_back (pc.ptr);
    }
    s.hash = hashUndoState();
    return s;
}

void SampleLabProcessor::applyUndo (const UndoSnap& s)
{
    kit.cells = s.cells;
    cellData = s.data;
    kit.song = s.song;
    if (songMode.load())
        publishSong();

    // Pattern modificati rispetto a ora: si ripristinano e si mostra quello cambiato (il pattern attivo, se e' tra questi)
    {
        const auto cur = patternHashes();
        int show = -1;
        for (size_t i = 0; i < cur.size() && i < s.patHashes.size() && i < kit.patterns.size(); ++i)
            if (cur[i] != s.patHashes[i] && s.patterns[i] != nullptr)
            {
                kit.patterns[i] = *s.patterns[i];
                if (show < 0 || (int) i == kit.activePattern)
                    show = (int) i;
            }
        if (show >= 0)
        {
            kit.activePattern = show;
            publishPattern();
        }
    }

    const auto& list = undoParams();
    for (size_t i = 0; i < list.size() && i < s.params.size(); ++i)
        if (std::abs (list[i]->getValue() - s.params[i]) > 1.0e-7f)
            list[i]->setValueNotifyingHost (s.params[i]);
    dirty = true;
    publishKit (true);
}

void SampleLabProcessor::undoTick (bool mouseDown)
{
    if (mouseDown)
        return;                                            // durante una trascinata non si registra nulla: la gesto e' una voce sola
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (! undoBaselineValid)
    {
        undoBaseline = captureUndo();
        undoBaselineValid = true;
        undoBaselineAt = now;
        return;
    }
    if (hashUndoState() == undoBaseline.hash)
    {
        undoBaselineAt = now;                              // nulla e' cambiato: da qui in poi un'azione dell'utente conta come nuova
        return;
    }

    if (lastUserInputMs >= undoBaselineAt)                 // c'e' stata un'azione dell'utente: lo stato precedente diventa un passo di annullamento
    {
        undoBaseline.when = now;
        undoStack.push_back (std::move (undoBaseline));
        if (undoStack.size() > 100)
            undoStack.erase (undoStack.begin());
        redoStack.clear();
    }
    undoBaseline = captureUndo();                          // altrimenti (automazione della DAW) lo stato di riferimento si aggiorna in silenzio
    undoBaselineAt = now;
}

bool SampleLabProcessor::undo()
{
    undoTick (false);                                      // registra subito eventuali modifiche appena fatte
    if (undoStack.empty())
        return false;
    auto target = std::move (undoStack.back());
    undoStack.pop_back();
    auto cur = captureUndo();
    cur.when = juce::Time::getMillisecondCounterHiRes();
    redoStack.push_back (std::move (cur));
    applyUndo (target);
    undoBaseline = captureUndo();
    undoBaselineValid = true;
    undoBaselineAt = juce::Time::getMillisecondCounterHiRes();
    return true;
}

bool SampleLabProcessor::redo()
{
    if (redoStack.empty())
        return false;
    auto target = std::move (redoStack.back());
    redoStack.pop_back();
    auto cur = captureUndo();
    cur.when = juce::Time::getMillisecondCounterHiRes();
    undoStack.push_back (std::move (cur));
    applyUndo (target);
    undoBaseline = captureUndo();
    undoBaselineValid = true;
    undoBaselineAt = juce::Time::getMillisecondCounterHiRes();
    return true;
}

void SampleLabProcessor::resetUndo()
{
    undoStack.clear();
    redoStack.clear();
    patCache.clear();
    undoBaselineValid = false;
}

void SampleLabProcessor::cellEdited (int)
{
    dirty = true;
    publishKit (false);
}

void SampleLabProcessor::loadKitSamples()
{
    cellData.assign (kit.cells.size(), nullptr);
    for (int i = 0; i < kit.numCells(); ++i)
    {
        auto& c = kit.cells[(size_t) i];
        if (! c.hasSample())
            continue;

        const juce::File f (c.samplePath);
        if (auto it = sampleCache.find (f.getFullPathName()); it != sampleCache.end())
        {
            cellData[(size_t) i] = it->second;
            c.missing = false;
            continue;
        }

        auto data = f.existsAsFile() ? sl::loadSampleFile (formats, f) : nullptr;
        c.missing = (data == nullptr);
        if (data != nullptr)
        {
            sampleCache[f.getFullPathName()] = data;
            cellData[(size_t) i] = data;
        }
    }
}

void SampleLabProcessor::newKit()
{
    kit = sl::Kit::makeDefault();
    cellData.assign (kit.cells.size(), nullptr);
    publishKit (true);
    params.readFromKit (kit);
    midiLearn.setMappings (kit.midiMap);
    dirty = false;          // dopo readFromKit: impostare i parametri li marca come modificati
    resetUndo();
}

bool SampleLabProcessor::openKit (const juce::File& file, juce::String& error)
{
    sl::Kit k;
    if (! sl::KitIO::load (file, k, error))
        return false;

    kit = std::move (k);
    loadKitSamples();
    publishKit (true);
    params.readFromKit (kit);
    midiLearn.setMappings (kit.midiMap);
    dirty = false;
    resetUndo();
    addRecentKit (file);
    return true;
}

bool SampleLabProcessor::saveKit (const juce::File& file, juce::String& error)
{
    params.writeToKit (kit);        // i valori automatizzabili vivono nei parametri: si copiano nel kit prima di scrivere
    kit.midiMap = midiLearn.mappings();
    if (! sl::KitIO::save (kit, file, error))
        return false;

    // I percorsi dei sample ora puntano alla copia nella cartella del kit: si riallinea la cache
    for (int i = 0; i < kit.numCells(); ++i)
    {
        const auto& c = kit.cells[(size_t) i];
        if (c.hasSample() && cellData[(size_t) i] != nullptr)
            sampleCache[juce::File (c.samplePath).getFullPathName()] = cellData[(size_t) i];
    }

    dirty = false;
    addRecentKit (kit.file);
    return true;
}

// ------------------------------------------------------- Kit recenti

juce::PropertiesFile& SampleLabProcessor::getSettings() const
{
    if (appProps == nullptr)
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "SampleLab";
        o.filenameSuffix = ".settings";
        o.folderName = "SampleLab";
        o.osxLibrarySubFolder = "Application Support";
        o.commonToAllUsers = false;
        o.storageFormat = juce::PropertiesFile::storeAsXML;
        appProps = std::make_unique<juce::ApplicationProperties>();
        appProps->setStorageParameters (o);
    }
    return *appProps->getUserSettings();
}

juce::StringArray SampleLabProcessor::getRecentKits() const
{
    juce::StringArray list;
    list.addTokens (getSettings().getValue ("recentKits"), "|", "");
    list.removeEmptyStrings();
    return list;
}

void SampleLabProcessor::addRecentKit (const juce::File& file)
{
    auto list = getRecentKits();
    list.removeString (file.getFullPathName());
    list.insert (0, file.getFullPathName());
    while (list.size() > kMaxRecentKits)
        list.remove (list.size() - 1);

    getSettings().setValue ("recentKits", list.joinIntoString ("|"));
    getSettings().saveIfNeeded();
}

void SampleLabProcessor::removeRecentKit (const juce::String& path)
{
    auto list = getRecentKits();
    list.removeString (path);
    getSettings().setValue ("recentKits", list.joinIntoString ("|"));
    getSettings().saveIfNeeded();
}

void SampleLabProcessor::clearRecentKits()
{
    getSettings().setValue ("recentKits", "");
    getSettings().saveIfNeeded();
}

// ------------------------------------------------------------- Stato DAW

void SampleLabProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    params.writeToKit (kit);
    kit.midiMap = midiLearn.mappings();
    juce::ValueTree state ("SampleLabState");
    state.setProperty ("uiWidth", uiWidth, nullptr);
    state.setProperty ("uiHeight", uiHeight, nullptr);
    state.setProperty ("uiZoom", uiZoom, nullptr);
    state.setProperty ("internalBpm", internalBpm.load(), nullptr);
    state.setProperty ("bpmEdited", bpmEdited.load(), nullptr);
    state.setProperty ("hostSeqMode", hostSeqMode.load(), nullptr);
    state.setProperty ("metronome", clickOn.load(), nullptr);
    state.setProperty ("clickVolume", clickVol.load(), nullptr);
    state.setProperty ("countInBars", countInBars.load(), nullptr);
    state.setProperty ("songMode", songMode.load(), nullptr);
    state.setProperty ("kitJson", sl::KitIO::toJson (kit, false), nullptr);   // percorsi assoluti
    state.setProperty ("kitFile", kit.file.getFullPathName(), nullptr);
    state.setProperty ("dirty", dirty.load(), nullptr);

    juce::MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void SampleLabProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);
    if (! state.isValid())
        return;

    uiWidth  = state.getProperty ("uiWidth", uiWidth);
    uiHeight = state.getProperty ("uiHeight", uiHeight);
    uiZoom   = state.getProperty ("uiZoom", uiZoom);
    setInternalBpm (state.getProperty ("internalBpm", 120.0));
    bpmEdited.store ((bool) state.getProperty ("bpmEdited", false));
    hostSeqMode.store (juce::jlimit (0, 2, (int) state.getProperty ("hostSeqMode", (int) HostSeqMode::MuteOnRecord)));
    songMode.store ((bool) state.getProperty ("songMode", false));
    clickOn.store ((bool) state.getProperty ("metronome", false));
    clickVol.store (juce::jlimit (0.0f, 1.0f, (float) (double) state.getProperty ("clickVolume", 0.67)));
    countInBars.store (juce::jlimit (0, 2, (int) state.getProperty ("countInBars", 1)));

    const juce::String json = state.getProperty ("kitJson", juce::String());
    if (json.isNotEmpty())
    {
        sl::Kit k;
        juce::String error;
        if (sl::KitIO::fromJson (json, juce::File(), k, error))
        {
            k.file = juce::File (state.getProperty ("kitFile", juce::String()).toString());
            kit = std::move (k);
            loadKitSamples();
            publishKit (true);
            params.readFromKit (kit);
            midiLearn.setMappings (kit.midiMap);
            dirty = (bool) state.getProperty ("dirty", false);
            resetUndo();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SampleLabProcessor();
}
