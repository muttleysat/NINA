#pragma once

#include "../engine/SampleData.h"
#include "Theme.h"

// Forma d'onda con marker Start/End trascinabili (ED-1, ED-2).
class WaveformView final : public juce::Component
{
public:
    void setSample (sl::SampleDataPtr data, juce::Colour colour);
    void setRegion (float start, float end);

    std::function<void()> onRegionDragStart;                         // un marker viene afferrato (inizio gesto)
    std::function<void (float start, float end)> onRegionChanged;   // durante il trascinamento
    std::function<void()> onRegionCommitted;                         // marker rilasciato (fine gesto)

    bool isDragging() const { return dragging != Handle::None; }

    void paint (juce::Graphics&) override;
    void resized() override { peaksDirty = true; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    enum class Handle { None, Start, End };
    Handle handleAt (int x) const;
    float xToNorm (int x) const;
    void rebuildPeaks();

    sl::SampleDataPtr sample;
    juce::Colour colour { theme::accent };   // sostituito dal colore della cella
    float start = 0.0f, end = 1.0f;
    Handle dragging = Handle::None;

    std::vector<std::pair<float, float>> peaks;   // min/max per colonna
    bool peaksDirty = true;
};
