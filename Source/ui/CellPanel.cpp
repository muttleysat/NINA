#include "CellPanel.h"

CellPanel::CellPanel (SampleLabProcessor& p) : processor (p)
{
    prevButton.onClick = [this] { if (onStep) onStep (-1); };
    nextButton.onClick = [this] { if (onStep) onStep (+1); };
    loadButton.onClick = [this] { if (onLoadRequested) onLoadRequested (cell); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    addAndMakeVisible (loadButton);

    const char* names[3] = { "One-shot", "Gate", "Loop" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (4711);
        b.onClick = [this, i]
        {
            if (updating) return;
            processor.getKit().cells[(size_t) cell].mode = (sl::TriggerMode) i;
            processor.cellEdited (cell);
            if (onEdited) onEdited();
        };
        addAndMakeVisible (b);
    }

    reverseButton.setColour (juce::ToggleButton::textColourId, theme::text);
    reverseButton.onClick = [this]
    {
        if (updating) return;
        processor.getKit().cells[(size_t) cell].reverse = reverseButton.getToggleState();
        processor.cellEdited (cell);
        if (onEdited) onEdited();
    };
    addAndMakeVisible (reverseButton);

    // Selettori delle note (nome nota in stile Cubase: C3 = 60)
    struct NoteSetup { juce::Slider* slider; int sl::CellParams::*member; const char* tip; };
    for (auto [slider, member, tip] : { NoteSetup { &rootSlider, &sl::CellParams::rootNote, "Note at which the sample plays at its original pitch" },
                                        NoteSetup { &lowSlider,  &sl::CellParams::lowNote,  "Lowest key that triggers this cell" },
                                        NoteSetup { &highSlider, &sl::CellParams::highNote, "Highest key that triggers this cell" } })
    {
        slider->setSliderStyle (juce::Slider::IncDecButtons);
        slider->setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
        slider->setRange (0, 127, 1);
        slider->setTooltip (tip);
        slider->textFromValueFunction = [] (double v) { return theme::noteName ((int) v); };
        slider->valueFromTextFunction = [] (const juce::String& t) { return (double) t.getIntValue(); };
        slider->onValueChange = [this, slider, member]
        {
            if (updating) return;
            processor.getKit().cells[(size_t) cell].*member = (int) slider->getValue();
            processor.cellEdited (cell);
            if (onEdited) onEdited();
        };
        addAndMakeVisible (*slider);
    }

    setCell (0);
}

void CellPanel::setCell (int c)
{
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), c);
    refresh();
}

void CellPanel::refresh()
{
    if (cell >= processor.getKit().numCells())
        cell = 0;

    const auto& c = processor.getKit().cells[(size_t) cell];
    updating = true;
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setToggleState ((int) c.mode == i, juce::dontSendNotification);
    reverseButton.setToggleState (c.reverse, juce::dontSendNotification);
    rootSlider.setValue (c.rootNote, juce::dontSendNotification);
    lowSlider.setValue (c.lowNote, juce::dontSendNotification);
    highSlider.setValue (c.highNote, juce::dontSendNotification);
    updating = false;

    const bool has = c.hasSample();
    for (auto& b : modeButtons) b.setEnabled (has);
    reverseButton.setEnabled (has);
    repaint();
}

void CellPanel::sectionLabel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t, float s)
{
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s), juce::Font::bold));
    g.drawText (t.toUpperCase(), r, juce::Justification::centredLeft);
}

void CellPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];

    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f * s, 1.0f);

    // Titolo: "A1 - Kick 01"
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (12.0f, 17.0f * s), juce::Font::bold));
    g.drawText (theme::cellId (cell, kit.cols) + (c.hasSample() ? juce::String (" - ") + c.name : juce::String()),
                titleRect, juce::Justification::centredLeft, true);

    // Scheda del campione
    g.setColour (theme::panelHi);
    g.fillRoundedRectangle (sampleCard.toFloat(), 8.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (sampleCard.toFloat().reduced (0.5f), 8.0f * s, 1.0f);

    auto card = sampleCard.reduced ((int) (12 * s), (int) (8 * s));
    const float dot = 10.0f * s;
    g.setColour (c.hasSample() ? (c.missing ? theme::danger : juce::Colour (c.colour)) : theme::textDim.withAlpha (0.5f));
    g.fillEllipse ((float) card.getX(), (float) card.getY() + 3.0f * s, dot, dot);

    auto text = card.withTrimmedLeft ((int) (dot + 8 * s)).withTrimmedRight ((int) (64 * s));
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
    g.drawText ("Sample", text.removeFromTop (text.getHeight() / 2), juce::Justification::centredLeft);
    g.setColour (c.missing ? theme::danger : theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    g.drawText (c.hasSample() ? juce::File (c.samplePath).getFileName() + (c.missing ? "  (missing)" : "") : juce::String ("No sample - drop a file here"),
                text, juce::Justification::centredLeft, true);

    sectionLabel (g, modeLabel, "Trigger mode", s);
    sectionLabel (g, mapLabel, "MIDI mapping", s);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    g.drawText ("Root note", rootRow.withWidth (rootRow.getWidth() / 2), juce::Justification::centredLeft);
    g.drawText ("Key low", lowRow.withWidth (lowRow.getWidth() / 2), juce::Justification::centredLeft);
    g.drawText ("Key high", highRow.withWidth (highRow.getWidth() / 2), juce::Justification::centredLeft);

    // Informazioni sul campione
    if (const auto data = processor.getCellData (cell))
    {
        const double secs = (double) data->numSamples() / data->sampleRate;
        const juce::String rows[2][2] = {
            { "Format", juce::String (data->sampleRate / 1000.0, 1) + " kHz - " + (data->numChannels() > 1 ? "stereo" : "mono") },
            { "Length", juce::String (secs, 3) + " s  (" + juce::String (data->numSamples()) + " smp)" },
        };

        auto r = infoRect;
        const int rh = juce::jmax (14, (int) (20 * s));
        sectionLabel (g, r.removeFromTop (rh), "Sample info", s);
        for (auto& row : rows)
        {
            auto line = r.removeFromTop (rh);
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
            g.drawText (row[0], line.removeFromLeft (line.getWidth() / 3), juce::Justification::centredLeft);
            g.setColour (theme::text);
            g.drawText (row[1], line, juce::Justification::centredLeft, true);
        }
    }
}

void CellPanel::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (12 * s));
    const int gap = (int) (10 * s);

    auto top = area.removeFromTop ((int) (32 * s));
    const int bw = (int) (30 * s);
    nextButton.setBounds (top.removeFromRight (bw));
    top.removeFromRight ((int) (6 * s));
    prevButton.setBounds (top.removeFromRight (bw));
    titleRect = top;
    area.removeFromTop (gap);

    sampleCard = area.removeFromTop ((int) (54 * s));
    loadButton.setBounds (sampleCard.reduced ((int) (8 * s)).removeFromRight ((int) (54 * s)).withSizeKeepingCentre ((int) (54 * s), (int) (26 * s)));
    area.removeFromTop (gap);

    modeLabel = area.removeFromTop ((int) (20 * s));
    auto row = area.removeFromTop ((int) (30 * s));
    const int w = row.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
        modeButtons[i].setBounds (row.removeFromLeft (w).reduced ((int) (2 * s), 0));
    area.removeFromTop (gap);

    reverseButton.setBounds (area.removeFromTop ((int) (26 * s)));
    area.removeFromTop (gap);

    mapLabel = area.removeFromTop ((int) (20 * s));
    const int rh = juce::jmax (22, (int) (26 * s));
    rootRow = area.removeFromTop (rh);
    lowRow = area.removeFromTop (rh);
    highRow = area.removeFromTop (rh);
    for (auto [mapRow, slider] : { std::pair { rootRow, &rootSlider }, std::pair { lowRow, &lowSlider }, std::pair { highRow, &highSlider } })
    {
        const auto r = mapRow.withTrimmedLeft (mapRow.getWidth() / 2).reduced (0, 1);
        slider->setBounds (r);
        // Casella del nome nota a sinistra, pulsanti - e + a destra con spazio sufficiente
        slider->setTextBoxStyle (juce::Slider::TextBoxLeft, false, (int) ((float) r.getWidth() * 0.52f), r.getHeight());
    }
    area.removeFromTop (gap);

    infoRect = area;
}
