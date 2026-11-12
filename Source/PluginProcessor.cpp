#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "sequencer/SequencerExport.h"

SampleLabProcessor::SampleLabProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true))
{
    // Parametri esposti alla DAW (automazione): gruppi per cella ed effetto, nomi leggibili, ID stabili
    for (auto& group : params.releaseGroups())
        addParameterGroup (std::move (group));

    for (auto* p : params.all())
        p->addListener (&dirtyListener);

    formats.registerBasicFormats();
    cellData.resize (kit.cells.size());
    publishKit (true);
    params.readFromKit (kit);
    dirty = false;
}

SampleLabProcessor::~SampleLabProcessor()
{
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
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SampleLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

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
    engine.live.bpm = bpm;
    currentBpm.store (bpm, std::memory_order_relaxed);
    hostTempo.store (fromHost, std::memory_order_relaxed);

    // Sequencer: pattern attivo (pubblicato dal thread UI) e trasporto. Nel plugin si segue la DAW (play/stop e posizione),
    // nello standalone il trasporto e' interno.
    patternExchange.tryAcquire (audioPattern);

    const int numSamples = buffer.getNumSamples();
    // Trasporto: se la DAW sta suonando il sequencer la segue; altrimenti (DAW ferma o standalone) parte dal tasto Play
    // del plugin con il trasporto interno, cosi' si puo' provare il pattern senza avviare Cubase.
    bool playing = seqRunning.load();
    double ppq0 = internalPpq;
    bool useHostPpq = false;
    bool hostRollingNow = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostPpq = pos->getPpqPosition(); hostPpq.hasValue() && pos->getIsPlaying())
            {
                useHostPpq = true;
                hostRollingNow = true;
                playing = true;
                ppq0 = *hostPpq;
            }
    hostRolling.store (hostRollingNow, std::memory_order_relaxed);

    if (! useHostPpq)
    {
        if (playing && ! wasRunning)
            internalPpq = ppq0 = 0.0;                       // Play dall'inizio
        wasRunning = playing;
        if (playing)
            internalPpq += (double) numSamples * bpm / (60.0 * currentSampleRate);
    }

    seqRunner.runBlock (audioPattern.get(), playing, ppq0, bpm, currentSampleRate, numSamples);
    seqPlaying.store (playing, std::memory_order_relaxed);
    seqPpq.store (ppq0, std::memory_order_relaxed);

    buffer.clear();
    engine.process (buffer, midi, &seqRunner.getEvents());
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
        r.data = cellData[(size_t) i].get();
        r.rootNote = c.rootNote;
        r.lowNote = c.lowNote;
        r.highNote = c.highNote;
        snap->cells.push_back (r);
    }
    return snap;
}

void SampleLabProcessor::publishKit (bool newGeneration)
{
    if (newGeneration)
        ++generation;

    engine.publish (buildSnapshot());
    publishPattern();
}

// ------------------------------------------------------------- Sequencer

void SampleLabProcessor::publishPattern()
{
    patternExchange.publish (std::make_shared<const sl::Pattern> (activePattern()));
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
    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
    const double bpm = currentBpm.load();
    constexpr int block = 512;

    auto eng = std::make_unique<sl::Engine>();
    eng->prepare (sr, block);
    eng->publish (buildSnapshot());
    params.fillLive (eng->live);
    eng->live.bpm = bpm;

    auto out = sl::SequencerExport::renderOffline (*eng, activePattern(), bars, bpm, sr);
    const int totalSamples = out.getNumSamples();

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

    // Si taglia il silenzio finale oltre la coda
    int lastAudible = totalSamples;
    while (lastAudible > 0 && out.getMagnitude (0, lastAudible - 1, 1) < 1.0e-5f && out.getMagnitude (1, lastAudible - 1, 1) < 1.0e-5f)
        --lastAudible;
    lastAudible = juce::jmin (totalSamples, lastAudible + (int) (0.05 * sr));

    return writer->writeFromAudioSampleBuffer (out, 0, lastAudible);
}

sl::SampleDataPtr SampleLabProcessor::getCellData (int cell) const
{
    return (cell >= 0 && cell < (int) cellData.size()) ? cellData[(size_t) cell] : nullptr;
}

bool SampleLabProcessor::loadSampleIntoCell (int cell, const juce::File& file, juce::String& error)
{
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

    const auto& refs = params.cell (cell);
    refs.at (sl::CellF::Start)->setValueNotifyingHost (0.0f);
    refs.at (sl::CellF::End)->setValueNotifyingHost (1.0f);
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
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SampleLabProcessor();
}
