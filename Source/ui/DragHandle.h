#pragma once

#include "Theme.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Maniglia "Drag to DAW": trascinandola fuori dalla finestra si lascia il file su una traccia della DAW.
// I file si generano al momento del trascinamento (prepareFiles).
class DragHandle final : public juce::Component, public juce::SettableTooltipClient
{
public:
    DragHandle (juce::String text, juce::String tooltip, std::function<juce::StringArray()> prepareFiles)
        : label (std::move (text)), prepare (std::move (prepareFiles))
    {
        setTooltip (tooltip);
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hover ? theme::panelHi.brighter (0.18f) : theme::panelHi);
        g.fillRoundedRectangle (b, 7.0f * s);
        g.setColour (hover ? theme::accent : theme::border);
        g.drawRoundedRectangle (b, 7.0f * s, 1.2f);

        // Impugnatura a puntini
        g.setColour (theme::accent);
        const float cx = b.getX() + 12.0f * s, cy = b.getCentreY();
        for (int r = -1; r <= 1; ++r)
            for (int c = 0; c < 2; ++c)
                g.fillEllipse (cx + (float) c * 5.0f * s - 1.5f, cy + (float) r * 5.0f * s - 1.5f, 3.0f, 3.0f);

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
        g.drawText (label, getLocalBounds().withTrimmedLeft ((int) (26 * s)), juce::Justification::centredLeft, true);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override { started = false; }
    void mouseUp (const juce::MouseEvent&) override { started = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (started || e.getDistanceFromDragStart() < 6)
            return;
        started = true;

        const auto files = prepare();
        if (! files.isEmpty())
            juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
    }

private:
    juce::String label;
    std::function<juce::StringArray()> prepare;
    bool hover = false, started = false;
};
