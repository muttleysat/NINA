#pragma once

#include "PluginProcessor.h"
#include "ui/LookAndFeel.h"
#include "ui/PadGrid.h"
#include "ui/WaveStrip.h"
#include "ui/ModulesBar.h"
#include "ui/CellPanel.h"
#include "ui/FxPanel.h"
#include "ui/SeqPanel.h"
#include "ui/SlicePanel.h"
#include "ui/SongPanel.h"
#include "ui/MixerPanel.h"
#include "ui/SampleBrowser.h"

class SampleLabEditor final : public juce::AudioProcessorEditor,
                              public juce::DragAndDropContainer,
                              private juce::Timer
{
public:
    explicit SampleLabEditor (SampleLabProcessor&);
    ~SampleLabEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;      // barra spaziatrice = Play/Stop del sequencer

private:
    void toggleTransport();
    void doUndoRedo (bool redo);
    void timerCallback() override;
    void showTab (int tab);
    void updatePlayMode();              // Play: song nella scheda Song, pattern nelle altre
    juce::Rectangle<int> tabRect (int index) const;
    void showSizeMenu();
    void applyScale (float zoom);

    // Kit
    void showKitMenu();
    void refreshTopBar();
    void refreshAll();
    void refreshCellViews();
    void confirmDiscard (std::function<void()> proceed);
    void doNew();
    void doOpen();
    void doOpenFile (const juce::File& f);
    void doOpenAdjacent (int direction);
    void doSave();
    void doSaveAs();
    void doSaveSoundsOnly();            // salva i soli suoni (senza sequenze e song)
    void doOpenKeepSequences();         // apre un kit tenendo le sequenze e la song correnti
    void showError (const juce::String& message);

    // Celle
    void loadIntoCell (int cell, const juce::File& file);
    void requestLoadDialog (int cell);

    SampleLabLookAndFeel lnf;
    SampleLabProcessor& processor;

    // Ogni evento del mouse nell'applicazione segnala un'azione dell'utente alla cronologia di annullamento
    struct InputWatcher final : public juce::MouseListener
    {
        explicit InputWatcher (SampleLabProcessor& p) : proc (p) {}
        void mouseDown (const juce::MouseEvent&) override { proc.noteUserInput(); }
        void mouseUp (const juce::MouseEvent&) override { proc.noteUserInput(); }
        void mouseDrag (const juce::MouseEvent&) override { proc.noteUserInput(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { proc.noteUserInput(); }
        SampleLabProcessor& proc;
    };
    InputWatcher inputWatcher { processor };

    juce::Label kitLabel;               // nome del kit (come il file salvato; "Untitled" finche' non si salva)
    juce::TextButton kitMenuButton { "Kit" };                 // menu del kit: Nuovo, Apri, Salva, Recenti...
    juce::TextButton prevKitButton { "<" }, nextKitButton { ">" }, saveButton { "Save" }, sizeButton { "Size 100%" }, infoButton { "i" };
    juce::File lastKitFile;
    juce::Array<juce::File> neighbourKits;                    // kit nella stessa cartella (per le frecce)
    juce::TextButton transportButton { "Play" };    // Play/Stop del sequencer, sempre visibile
    juce::Label bpmLabel;               // BPM: della DAW (sola lettura) oppure interno (modificabile) nello standalone
    SampleBrowser browser;
    PadGrid grid;
    WaveStrip waveStrip;
    CellPanel cellPanel;
    ModulesBar modules;
    FxPanel fxPanel;
    SeqPanel seqPanel;
    SlicePanel slicePanel;
    SongPanel songPanel;
    MixerPanel mixerPanel;
    int sliceCell = -1;
    int kickTick = 1000;                // conta i tick dopo l'aggancio alla finestra: a intervalli si rifa il layout di tutto
    int lastLoggedW = 0, lastLoggedH = 0;
    int currentTab = 0;                 // 0 = Main, 1 = Seq, 3 = FX, 2 = Mixer, 4 = Slice, 5 = Song (Mixer arrivera' con le fasi successive)
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> topBar, tabBar;

    static constexpr int baseWidth = 1280;
    static constexpr int baseHeight = 760;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLabEditor)
};
