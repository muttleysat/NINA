#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "IconButton.h"
#include "DragHandle.h"

// Scheda Song (Pattern Sequencer): i pattern creati nella scheda Seq si concatenano in una catena di blocchi (pattern + ripetizioni).
// Con "Song mode" acceso il Play del sequencer suona la catena invece del pattern attivo. Qui ci sono anche l'esportazione (MIDI,
// audio, una traccia per cella) di tutta la song e il salvataggio/apertura della catena in un file .slsong.
class SongPanel final : public juce::Component, private juce::Timer
{
public:
    explicit SongPanel (SampleLabProcessor&);
    ~SongPanel() override;

    void refresh();                    // ricostruisce i blocchi dalla song del kit (apertura kit, undo, cambio di nomi dei pattern)

    void paint (juce::Graphics&) override;
    void resized() override;

    static juce::Colour patternColour (int pattern);

private:
    class BlockCard;
    class Strip;

    void timerCallback() override;
    sl::Song& song() { return processor.song(); }
    void edited();                     // pubblica la song al thread audio
    void addBlock (int pattern);
    void showAddMenu();
    void showMoreMenu();
    void moveBlock (int index, int delta);
    void duplicateBlock (int index);
    void removeBlock (int index);
    void saveSong();
    void openSong();
    void exportMidi();
    void bounceWav();
    void exportStems();
    juce::File defaultFile (const juce::String& folder, const juce::String& extension) const;
    juce::File dragFolder() const;
    juce::String baseName() const;
    void showMessage (const juce::String& text);
    juce::String lengthText() const;

    SampleLabProcessor& processor;

    juce::TextButton songModeButton { "Song mode" }, loopButton { "Loop" };
    IconButton addButton { "+ Block", IconButton::Icon::None }, moreButton { {}, IconButton::Icon::More },
               openButton { "Open song", IconButton::Icon::None }, saveButton { "Save song", IconButton::Icon::None },
               midiButton { "Export MIDI", IconButton::Icon::Midi }, wavButton { "Bounce WAV", IconButton::Icon::Wave },
               stemsButton { "Export Lanes", IconButton::Icon::Layers };
    juce::TextEditor nameBox;
    juce::ComboBox tailBox;
    std::unique_ptr<DragHandle> dragMidi, dragAudio, dragLanes;

    std::unique_ptr<Strip> strip;
    juce::Viewport viewport;

    juce::Rectangle<int> row1Rect, row2Rect, infoRect, mapRect, stripRect;
    std::vector<std::pair<int, int>> separatorsX;
    int playingBlock = -1;             // blocco in riproduzione (-1 = fermo)
    double playFraction = 0.0;         // avanzamento dentro il blocco (0..1)
    double playPos = 0.0;              // posizione nella song, in quarti
    int selected = -1;

    std::unique_ptr<juce::FileChooser> chooser;
};
