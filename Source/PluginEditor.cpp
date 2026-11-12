#include "PluginEditor.h"

namespace
{
const juce::Colour bgColour      { 0xff14161a };
const juce::Colour panelColour   { 0xff1e2127 };
const juce::Colour accentColour  { 0xff3ec1d3 };
const juce::Colour textColour    { 0xffd8dde3 };
}

SampleLabEditor::SampleLabEditor (SampleLabProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setResizable (true, true);
    setResizeLimits (1000, 640, 3840, 2160);

    sizeButton.onClick = [this] { showSizeMenu(); };
    addAndMakeVisible (sizeButton);

    setSize (processor.uiWidth, processor.uiHeight);
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (processor.uiZoom * 100.0f)) + "%");
}

void SampleLabEditor::showSizeMenu()
{
    // Fallback ridimensionamento a percentuali (UI-12b)
    juce::PopupMenu menu;
    const float zooms[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    for (int i = 0; i < 5; ++i)
        menu.addItem (i + 1, juce::String (juce::roundToInt (zooms[i] * 100.0f)) + "%",
                      true, std::abs (processor.uiZoom - zooms[i]) < 0.01f);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (sizeButton),
                        [this, zooms] (int result)
                        {
                            if (result > 0)
                                applyScale (zooms[result - 1]);
                        });
}

void SampleLabEditor::applyScale (float zoom)
{
    // Limita alla dimensione dello schermo
    if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()))
    {
        const auto area = display->userArea;
        zoom = juce::jmin (zoom, (float) area.getWidth() / (float) baseWidth,
                                 (float) area.getHeight() / (float) baseHeight);
    }

    processor.uiZoom = zoom;
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (zoom * 100.0f)) + "%");
    setSize (juce::roundToInt (baseWidth * zoom), juce::roundToInt (baseHeight * zoom));
}

void SampleLabEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgColour);

    const float s = (float) getWidth() / (float) baseWidth;

    // Barra superiore
    g.setColour (panelColour);
    g.fillRect (topBar);
    g.setColour (accentColour);
    g.setFont (juce::FontOptions (22.0f * s, juce::Font::bold));
    g.drawText ("SAMPLE LAB", topBar.reduced (16).withTrimmedRight (200), juce::Justification::centredLeft);

    // Browser
    g.setColour (panelColour);
    g.fillRoundedRectangle (browser.toFloat(), 6.0f * s);
    g.setColour (textColour.withAlpha (0.6f));
    g.setFont (juce::FontOptions (14.0f * s));
    g.drawText ("Browser", browser.reduced (12), juce::Justification::topLeft);

    // Griglia 4x12
    const float gap = 6.0f * s;
    const float cw = ((float) grid.getWidth()  - gap * (cols - 1)) / cols;
    const float ch = ((float) grid.getHeight() - gap * (rows - 1)) / rows;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            auto cell = juce::Rectangle<float> ((float) grid.getX() + c * (cw + gap),
                                                (float) grid.getY() + r * (ch + gap), cw, ch);
            g.setColour (panelColour.brighter (0.08f));
            g.fillRoundedRectangle (cell, 5.0f * s);
            g.setColour (accentColour.withAlpha (0.35f));
            g.drawRoundedRectangle (cell, 5.0f * s, 1.0f);
            g.setColour (textColour.withAlpha (0.5f));
            g.setFont (juce::FontOptions (12.0f * s));
            g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + r)) + juce::String (c + 1),
                        cell.toNearestInt(), juce::Justification::centred);
        }

    // Pannello inferiore
    g.setColour (panelColour);
    g.fillRoundedRectangle (bottomPanel.toFloat(), 6.0f * s);
    g.setColour (textColour.withAlpha (0.6f));
    g.setFont (juce::FontOptions (14.0f * s));
    g.drawText ("Editor / Effects / Sequencer", bottomPanel.reduced (12), juce::Justification::topLeft);
}

void SampleLabEditor::resized()
{
    processor.uiWidth  = getWidth();
    processor.uiHeight = getHeight();

    auto area = getLocalBounds();
    const int m = juce::jmax (6, getWidth() / 160);

    topBar = area.removeFromTop (juce::jmax (36, getHeight() / 14));
    sizeButton.setBounds (topBar.removeFromRight (juce::jmax (110, getWidth() / 10)).reduced (6));

    area.reduce (m, m);
    browser = area.removeFromLeft (juce::jlimit (180, 420, getWidth() / 5));
    area.removeFromLeft (m);

    bottomPanel = area.removeFromBottom (area.getHeight() * 40 / 100);
    area.removeFromBottom (m);
    grid = area;
}
