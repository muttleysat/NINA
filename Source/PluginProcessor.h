#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <map>
#include <deque>
#include <atomic>
#include "engine/Engine.h"
#include "kit/KitIO.h"
#include "params/ParamSet.h"
#include "params/MidiLearn.h"

class SampleLabProcessor final : public juce::AudioProcessor
{
public:
    SampleLabProcessor();
    ~SampleLabProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Parametri automatizzabili (fonte di verita' dei valori) ----
    const sl::ParamSet& getParams() const { return params; }

    // ---- Sequencer (thread messaggi, tranne dove indicato) ----
    sl::Pattern& activePattern() { return kit.patterns[(size_t) kit.activePattern]; }
    const sl::Pattern& activePattern() const { return kit.patterns[(size_t) kit.activePattern]; }
    int getActivePatternIndex() const { return kit.activePattern; }
    void setActivePattern (int index);
    void patternEdited();                                   // dopo ogni modifica del pattern: pubblica al thread audio
    bool addLaneForCell (int cell);                         // "Aggiungi al sequencer" (tasto destro sulla cella)
    void removeLaneForCell (int cell);

    // Trasporto: nello standalone e' interno (Play/Stop del plugin), nel plugin segue la DAW
    void setSequencerRunning (bool run);
    bool isSequencerRunning() const { return seqRunning.load(); }
    bool isSequencerPlaying() const { return seqPlaying.load(); }     // in riproduzione ora (DAW o interno)
    double getSequencerPpq() const { return seqPpq.load(); }         // posizione corrente in quarti (per l'indicatore degli step)

    // Esportazione del pattern attivo: clip MIDI e audio (WAV 24 bit con effetti, lock e tail)
    bool exportPatternMidi (const juce::File& file, int bars, juce::String& error);
    bool bouncePatternWav (const juce::File& file, int bars, juce::String& error);

    // ---- MIDI learn dei parametri ----
    sl::MidiLearn& getMidiLearn() { return midiLearn; }
    const sl::MidiLearn& getMidiLearn() const { return midiLearn; }
    void markDirty() { dirty = true; }
    void pollMidiLearn() { if (midiLearn.poll()) dirty = true; }     // chiamato dal timer dell'interfaccia

    // ---- Kit (da usare solo dal thread messaggi) ----
    // Attenzione: i valori automatizzabili (level, pan, inviluppo, effetti...) stanno nei parametri;
    // il kit li contiene solo dopo un salvataggio. Per mostrarli nell'interfaccia usare getParams().
    sl::Kit& getKit() { return kit; }
    const sl::Kit& getKit() const { return kit; }
    bool isDirty() const { return dirty.load(); }

    bool loadSampleIntoCell (int cell, const juce::File& file, juce::String& error);
    void clearCell (int cell);
    void cellEdited (int cell);                    // parametri modificati: marca modificato e pubblica
    sl::SampleDataPtr getCellData (int cell) const;

    void newKit();
    bool openKit (const juce::File& file, juce::String& error);
    bool saveKit (const juce::File& file, juce::String& error);

    // Kit recenti (ultimi 5, il piu' recente in alto), condivisi tra tutte le istanze
    juce::StringArray getRecentKits() const;
    void removeRecentKit (const juce::String& path);
    void clearRecentKits();

    std::pair<float, float> getMasterLevels() const { return engine.getMasterLevels(); }

    // Tempo per gli effetti sincronizzati: BPM della DAW se la DAW lo fornisce, altrimenti il BPM interno (standalone)
    double getCurrentBpm() const { return currentBpm.load(); }
    bool isHostTempo() const { return hostTempo.load(); }
    void setInternalBpm (double bpm) { internalBpm.store (juce::jlimit (20.0, 400.0, bpm)); }

    // Anteprima dei file del browser (caricamento in background, riproduzione su una voce dedicata)
    void auditionFile (const juce::File& file, float gainDb);
    void stopAudition();

    // Anteprima da UI (pad cliccato) e attivita' per l'indicatore delle celle
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }
    int getCellActivity (int cell) const { return engine.getCellActivity (cell); }

    // Dimensione/zoom dell'interfaccia (persistiti nello stato del progetto, UI-12)
    int uiWidth = 1280;
    int uiHeight = 760;
    float uiZoom = 1.0f;

    static constexpr int kMaxRecentKits = 5;

private:
    void publishKit (bool newGeneration);
    void publishPattern();
    std::shared_ptr<sl::KitSnapshot> buildSnapshot() const;
    void loadKitSamples();
    void addRecentKit (const juce::File& file);
    juce::PropertiesFile& getSettings() const;

    // Segna il kit come modificato quando cambia un parametro (da qualsiasi thread: UI, automazione, MIDI)
    struct DirtyListener final : juce::AudioProcessorParameter::Listener
    {
        explicit DirtyListener (std::atomic<bool>& f) : flag (f) {}
        void parameterValueChanged (int, float) override { flag.store (true); }
        void parameterGestureChanged (int, bool) override {}
        std::atomic<bool>& flag;
    };

    sl::ParamSet params;
    sl::MidiLearn midiLearn { params };
    sl::Engine engine;

    // Sequencer: pattern attivo pubblicato al thread audio, generatore di eventi e trasporto interno
    sl::SharedExchange<sl::Pattern> patternExchange;
    std::shared_ptr<const sl::Pattern> audioPattern;        // solo thread audio
    sl::SequencerRunner seqRunner;
    std::atomic<bool> seqRunning { false }, seqPlaying { false };
    std::atomic<double> seqPpq { 0.0 };
    double internalPpq = 0.0;                               // solo thread audio
    bool wasRunning = false;                                // solo thread audio
    double currentSampleRate = 44100.0;
    juce::MidiKeyboardState keyboardState;
    juce::AudioFormatManager formats;

    sl::Kit kit = sl::Kit::makeDefault();
    std::vector<sl::SampleDataPtr> cellData;
    std::map<juce::String, sl::SampleDataPtr> sampleCache;   // i campioni restano in memoria per la sessione
    std::atomic<bool> dirty { false };
    DirtyListener dirtyListener { dirty };
    std::atomic<double> internalBpm { 120.0 }, currentBpm { 120.0 };
    std::atomic<bool> hostTempo { false };
    int generation = 0;

    mutable std::unique_ptr<juce::ApplicationProperties> appProps;

    // Anteprima: gli ultimi campioni restano vivi finche' la voce audio puo' ancora leggerli
    juce::CriticalSection auditionLock;
    std::deque<sl::SampleDataPtr> auditionKeep;
    std::atomic<int> auditionToken { 0 };
    juce::ThreadPool auditionPool { 1 };   // ultimo membro: viene distrutto (e atteso) per primo

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLabProcessor)
};
