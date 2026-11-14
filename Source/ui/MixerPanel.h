#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"
#include "KnobBox.h"
#include "LearnMenu.h"

// Scheda Mixer: una "console" con un canale per cella (fader, meter digitale, Mute/Solo, pan, mandate al delay e al riverbero,
// interruttori degli effetti, uscita) e il master a destra. Tutti i controlli sono gli stessi parametri automatizzabili delle altre
// schede (Level, Pan, Delay/Reverb Mix, ...): muovendoli qui si muovono anche altrove, e il MIDI learn funziona con il tasto destro.
class MixerPanel final : public juce::Component, private juce::Timer
{
public:
    explicit MixerPanel (NinaProcessor&);
    ~MixerPanel() override;

    void refresh();                              // ricostruisce i canali (apertura kit, campioni caricati o tolti)
    void setSelectedCell (int cell);
    std::function<void (int cell)> onCellSelected;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class MeterView;
    class Strip;
    class MasterStrip;
    class Holder;

    void timerCallback() override;
    std::vector<int> visibleCells() const;
    void clearMutes();
    void clearSolos();

    NinaProcessor& processor;
    juce::ToggleButton allCellsToggle { "All cells" };
    juce::TextButton resetMuteButton { "Reset mutes" }, resetSoloButton { "Reset solos" };

    std::unique_ptr<Holder> holder;
    juce::Viewport viewport;
    std::unique_ptr<MasterStrip> master;
    std::vector<int> shownCells;
    int selectedCell = -1;
    juce::Rectangle<int> headerRect;
};
