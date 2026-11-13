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
            // Gesto completo verso la DAW (registrabile in automazione)
            if (modeAttachment != nullptr)
                modeAttachment->setValueAsCompleteGesture ((float) i);
            for (const int c : targets())
                if (c != cell)
                {
                    auto* p = processor.getParams().cell (c).mode;
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
                }
        };
        addAndMakeVisible (b);
    }

    reverseButton.setColour (juce::ToggleButton::textColourId, theme::text);
    reverseButton.onClick = [this]
    {
        for (const int c : targets())
            if (c != cell)
                processor.getParams().cell (c).reverse->setValueNotifyingHost (reverseButton.getToggleState() ? 1.0f : 0.0f);
    };
    addAndMakeVisible (reverseButton);

    // Normalize: regola il Level della cella perche' il picco arrivi a -1 dBFS (il file audio non cambia)
    normalizeButton.setTooltip ("Set this cell's Level so the loudest peak reaches -1 dBFS");
    normalizeButton.onClick = [this]
    {
        forTargets ([this] (int c) { processor.normalizeCell (c); });
    };
    addAndMakeVisible (normalizeButton);

    // MIDI learn su Reverse e sulla modalita' di trigger (parametri della cella selezionata)
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, reverseButton,
        [this] { return (juce::RangedAudioParameter*) processor.getParams().cell (cell).reverse; }));
    for (auto& b : modeButtons)
        learnMenus.push_back (std::make_unique<LearnMenu> (processor, b,
            [this] { return (juce::RangedAudioParameter*) processor.getParams().cell (cell).mode; }));

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

    // Loop e tempo: Sync allunga/accorcia il campione al BPM della DAW (o interno); Key lock mantiene l'intonazione
    syncButton.setTooltip ("Make this sample follow the project tempo (host BPM, or the BPM box in the top bar). The original file is not changed.");
    keyLockButton.setTooltip ("On: tempo changes keep the pitch (time-stretch). Off: tempo changes also change the pitch, like a turntable.");
    for (auto* b : { &syncButton, &keyLockButton })
    {
        b->setColour (juce::ToggleButton::textColourId, theme::text);
        addAndMakeVisible (*b);
    }
    syncButton.onClick = [this]
    {
        if (updating) return;
        const bool on = syncButton.getToggleState();
        forTargets ([this, on] (int c) { processor.getKit().cells[(size_t) c].tempoSync = on; processor.cellEdited (c); });
    };
    keyLockButton.onClick = [this]
    {
        if (updating) return;
        const bool on = keyLockButton.getToggleState();
        forTargets ([this, on] (int c) { processor.getKit().cells[(size_t) c].keyLock = on; processor.cellEdited (c); });
    };

    bpmSlider.setSliderStyle (juce::Slider::IncDecButtons);
    bpmSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
    bpmSlider.setRange (0.0, 300.0, 0.1);
    bpmSlider.setNumDecimalPlacesToDisplay (1);
    bpmSlider.setTooltip ("Original tempo of this loop (BPM). 0 = unknown. Use Detect to estimate it from the length.");
    bpmSlider.textFromValueFunction = [] (double v) { return v <= 0.0 ? juce::String ("--") : juce::String (v, 1); };
    bpmSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    bpmSlider.onValueChange = [this]
    {
        if (updating) return;
        const float v = (float) bpmSlider.getValue();
        forTargets ([this, v] (int c) { processor.getKit().cells[(size_t) c].loopBpm = v; processor.cellEdited (c); });
    };
    addAndMakeVisible (bpmSlider);

    detectButton.setTooltip ("Estimate the loop's BPM from its length (assumes a whole number of bars between Start and End)");
    detectButton.onClick = [this]
    {
        for (const int c : targets())
            processor.detectLoopBpm (c);
        refresh();
        if (onEdited) onEdited();
    };
    addAndMakeVisible (detectButton);

    // Uscita verso la DAW: Main (mix) oppure una delle uscite aggiuntive (nel plugin vanno attivate nella DAW)
    outputBox.addItem ("Main (mix)", 1);
    for (int i = 2; i <= sl::Engine::kMaxOutputs; ++i)
        outputBox.addItem ("Out " + juce::String (i), i);
    outputBox.setTooltip ("Which output of the plugin this cell plays on. In your DAW, enable the extra outputs (Out 2...) to get it on its own track; "
                          "if that output is off the cell plays in the Main mix. Single outputs skip the master effects and master volume.");
    outputBox.onChange = [this]
    {
        if (updating) return;
        const int out = outputBox.getSelectedId() - 1;
        forTargets ([this, out] (int c) { processor.getKit().cells[(size_t) c].output = out; processor.cellEdited (c); });
    };
    addAndMakeVisible (outputBox);

    setCell (0);
}

std::vector<int> CellPanel::targets() const
{
    std::vector<int> t;
    const auto& kit = processor.getKit();
    if (selectedCells)
        for (const int c : selectedCells())
            if (c >= 0 && c < kit.numCells() && kit.cells[(size_t) c].hasSample() && ! kit.cells[(size_t) c].missing)
                t.push_back (c);
    if (std::find (t.begin(), t.end(), cell) == t.end())
        t.push_back (cell);
    return t;
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

    // Modalita' di trigger e reverse seguono i parametri della cella (automazione inclusa)
    const auto& refs = processor.getParams().cell (cell);
    reverseAttachment.reset();
    reverseAttachment = std::make_unique<juce::ButtonParameterAttachment> (*refs.reverse, reverseButton);
    modeAttachment.reset();
    modeAttachment = std::make_unique<juce::ParameterAttachment> (*refs.mode, [this] (float v)
    {
        for (int i = 0; i < 3; ++i)
            modeButtons[i].setToggleState ((int) std::lround (v) == i, juce::dontSendNotification);
    });
    modeAttachment->sendInitialUpdate();

    updating = true;
    rootSlider.setValue (c.rootNote, juce::dontSendNotification);
    lowSlider.setValue (c.lowNote, juce::dontSendNotification);
    highSlider.setValue (c.highNote, juce::dontSendNotification);
    syncButton.setToggleState (c.tempoSync, juce::dontSendNotification);
    keyLockButton.setToggleState (c.keyLock, juce::dontSendNotification);
    bpmSlider.setValue ((double) c.loopBpm, juce::dontSendNotification);
    outputBox.setSelectedId (juce::jlimit (0, 15, c.output) + 1, juce::dontSendNotification);
    updating = false;

    const bool has = c.hasSample();
    for (auto& b : modeButtons) b.setEnabled (has);
    reverseButton.setEnabled (has);
    normalizeButton.setEnabled (has && ! c.missing);
    for (auto* b : { &syncButton, &keyLockButton })
        b->setEnabled (has && ! c.missing);
    bpmSlider.setEnabled (has && ! c.missing);
    detectButton.setEnabled (has && ! c.missing);
    outputBox.setEnabled (has);
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
    const int nTargets = (int) targets().size();
    g.drawText (theme::cellId (cell, kit.cols) + (c.hasSample() ? juce::String (" - ") + c.name : juce::String())
                    + (nTargets > 1 ? "  (+" + juce::String (nTargets - 1) + ")" : juce::String()),
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
    sectionLabel (g, loopLabel, "Loop / tempo", s);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    g.drawText ("Loop BPM", bpmRow.withWidth (bpmRow.getWidth() / 3), juce::Justification::centredLeft);
    g.drawText ("Output", outputRow.withWidth (outputRow.getWidth() / 3), juce::Justification::centredLeft);
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

    auto reverseRow = area.removeFromTop ((int) (26 * s));
    normalizeButton.setBounds (reverseRow.removeFromRight ((int) (92 * s)));
    reverseButton.setBounds (reverseRow);
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

    loopLabel = area.removeFromTop ((int) (20 * s));
    loopToggleRow = area.removeFromTop (rh);
    syncButton.setBounds (loopToggleRow.removeFromLeft (loopToggleRow.getWidth() * 55 / 100));
    keyLockButton.setBounds (loopToggleRow);
    bpmRow = area.removeFromTop (rh);
    {
        auto r = bpmRow.withTrimmedLeft (bpmRow.getWidth() / 3).reduced (0, 1);
        detectButton.setBounds (r.removeFromRight ((int) (58 * s)));
        r.removeFromRight ((int) (4 * s));
        bpmSlider.setBounds (r);
        bpmSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, (int) ((float) r.getWidth() * 0.5f), r.getHeight());
    }
    outputRow = area.removeFromTop (rh);
    outputBox.setBounds (outputRow.withTrimmedLeft (outputRow.getWidth() / 3).reduced (0, 1));
    area.removeFromTop (gap);

    infoRect = area;
}
