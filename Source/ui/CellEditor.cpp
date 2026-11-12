#include "CellEditor.h"

CellEditor::KnobBox::KnobBox (const juce::String& title)
{
    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, theme::textDim);
    label.setInterceptsMouseClicks (false, false);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setColour (juce::Slider::rotarySliderFillColourId, theme::accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, theme::panelHi);
    slider.setColour (juce::Slider::thumbColourId, theme::text);
    slider.setColour (juce::Slider::textBoxTextColourId, theme::text);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    addAndMakeVisible (label);
    addAndMakeVisible (slider);
}

void CellEditor::KnobBox::resized()
{
    const float s = theme::scaleFor (*this);
    auto b = getLocalBounds();
    label.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s)));
    label.setBounds (b.removeFromTop ((int) (15 * s)));
    const int tb = (int) (16 * s);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, b.getWidth(), tb);
    slider.setBounds (b);
}

CellEditor::CellEditor (SampleLabProcessor& p) : processor (p)
{
    titleLabel.setColour (juce::Label::textColourId, theme::text);
    addAndMakeVisible (titleLabel);

    modeBox.addItem ("One-shot", 1);
    modeBox.addItem ("Gate", 2);
    modeBox.addItem ("Loop", 3);
    modeBox.setSelectedId (1, juce::dontSendNotification);
    modeBox.onChange = [this]
    {
        if (updating) return;
        processor.getKit().cells[(size_t) cell].mode = (sl::TriggerMode) (modeBox.getSelectedId() - 1);
        edited();
    };
    addAndMakeVisible (modeBox);

    reverseButton.onClick = [this]
    {
        if (updating) return;
        processor.getKit().cells[(size_t) cell].reverse = reverseButton.getToggleState();
        edited();
    };
    reverseButton.setColour (juce::ToggleButton::textColourId, theme::text);
    reverseButton.setColour (juce::ToggleButton::tickColourId, theme::accent);
    addAndMakeVisible (reverseButton);

    addAndMakeVisible (waveform);
    waveform.onRegionChanged = [this] (float s, float e)
    {
        auto& c = processor.getKit().cells[(size_t) cell];
        c.start = s;
        c.end = e;
        edited();
    };

    using P = sl::CellParams;
    addKnob ("Level", -60, 12, 0.1, 0, -1, [] (const P& c) { return c.levelDb; }, [] (P& c, float v) { c.levelDb = v; }, " dB");
    addKnob ("Pan", -1, 1, 0.01, 0, -1, [] (const P& c) { return c.pan; }, [] (P& c, float v) { c.pan = v; });
    addKnob ("Tune", -100, 100, 1, 0, -1, [] (const P& c) { return c.tuneCents; }, [] (P& c, float v) { c.tuneCents = v; }, " ct");
    addKnob ("Pitch", -24, 24, 1, 0, -1, [] (const P& c) { return c.pitchSemis; }, [] (P& c, float v) { c.pitchSemis = v; }, " st");
    addKnob ("Attack", 0, 2, 0.001, 0.0005, 0.1, [] (const P& c) { return c.attack; }, [] (P& c, float v) { c.attack = v; }, " s");
    addKnob ("Hold", 0, 2, 0.001, 0, 0.1, [] (const P& c) { return c.hold; }, [] (P& c, float v) { c.hold = v; }, " s");
    addKnob ("Decay", 0, 5, 0.001, 0.1, 0.5, [] (const P& c) { return c.decay; }, [] (P& c, float v) { c.decay = v; }, " s");
    addKnob ("Sustain", 0, 1, 0.01, 1, -1, [] (const P& c) { return c.sustain; }, [] (P& c, float v) { c.sustain = v; });
    addKnob ("Release", 0, 5, 0.001, 0.02, 0.5, [] (const P& c) { return c.release; }, [] (P& c, float v) { c.release = v; }, " s");

    auto& root = addKnob ("Root", 0, 127, 1, 36, -1, [] (const P& c) { return (float) c.rootNote; }, [] (P& c, float v) { c.rootNote = (int) v; });
    auto& low  = addKnob ("Low", 0, 127, 1, 36, -1, [] (const P& c) { return (float) c.lowNote; }, [] (P& c, float v) { c.lowNote = (int) v; });
    auto& high = addKnob ("High", 0, 127, 1, 36, -1, [] (const P& c) { return (float) c.highNote; }, [] (P& c, float v) { c.highNote = (int) v; });
    for (auto* k : { &root, &low, &high })
    {
        k->slider.textFromValueFunction = [] (double v) { return theme::noteName ((int) v); };
        k->slider.valueFromTextFunction = [] (const juce::String& t) { return (double) t.getIntValue(); };
        k->slider.updateText();
    }

    setCell (0);
}

CellEditor::KnobBox& CellEditor::addKnob (const juce::String& title, double min, double max, double step, double def, double skewMid,
                                          std::function<float (const sl::CellParams&)> get,
                                          std::function<void (sl::CellParams&, float)> set,
                                          const juce::String& suffix)
{
    auto k = std::make_unique<KnobBox> (title);
    k->get = std::move (get);
    k->set = std::move (set);
    k->slider.setRange (min, max, step);
    if (skewMid > 0.0)
        k->slider.setSkewFactorFromMidPoint (skewMid);
    k->slider.setDoubleClickReturnValue (true, def);
    k->slider.setTextValueSuffix (suffix);
    k->slider.setNumDecimalPlacesToDisplay (step >= 1.0 ? 0 : (step >= 0.1 ? 1 : 2));

    auto* raw = k.get();
    raw->slider.onValueChange = [this, raw]
    {
        if (updating) return;
        raw->set (processor.getKit().cells[(size_t) cell], (float) raw->slider.getValue());
        edited();
    };

    addAndMakeVisible (*k);
    knobs.push_back (std::move (k));
    return *raw;
}

void CellEditor::edited()
{
    processor.cellEdited (cell);
    if (onEdited)
        onEdited();
}

void CellEditor::setCell (int c)
{
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), c);
    refresh();
}

void CellEditor::refresh()
{
    if (cell >= processor.getKit().numCells())
        cell = 0;

    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];
    updating = true;

    const juce::String id = juce::String::charToString ((juce::juce_wchar) ('A' + cell / kit.cols)) + juce::String (cell % kit.cols + 1);
    titleLabel.setText (id + "   " + (c.hasSample() ? c.name : juce::String ("(empty)")), juce::dontSendNotification);
    modeBox.setSelectedId ((int) c.mode + 1, juce::dontSendNotification);
    reverseButton.setToggleState (c.reverse, juce::dontSendNotification);
    for (auto& k : knobs)
        k->slider.setValue (k->get (c), juce::dontSendNotification);

    waveform.setSample (processor.getCellData (cell), juce::Colour (c.colour));
    waveform.setRegion (c.start, c.end);

    const bool enabled = c.hasSample() && ! c.missing;
    waveform.setEnabled (enabled);
    updating = false;
    repaint();
}

void CellEditor::paint (juce::Graphics& g)
{
    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f * theme::scaleFor (*this));
}

void CellEditor::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (8 * s));

    strip = area.removeFromTop ((int) (26 * s));
    titleLabel.setFont (juce::FontOptions (juce::jmax (11.0f, 15.0f * s), juce::Font::bold));
    titleLabel.setBounds (strip.removeFromLeft (strip.getWidth() / 2));
    reverseButton.setBounds (strip.removeFromRight ((int) (90 * s)));
    modeBox.setBounds (strip.removeFromRight ((int) (110 * s)).reduced (2));

    area.removeFromTop ((int) (4 * s));
    waveform.setBounds (area.removeFromTop (juce::jmax (40, area.getHeight() * 42 / 100)));
    area.removeFromTop ((int) (6 * s));

    // Manopole: flusso a capo, larghezza proporzionale
    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.alignContent = juce::FlexBox::AlignContent::flexStart;

    const int perRow = juce::jmax (1, area.getWidth() / juce::jmax (40, (int) (68 * s)));
    const int rowsNeeded = ((int) knobs.size() + perRow - 1) / perRow;
    const float w = (float) area.getWidth() / (float) juce::jmin ((int) knobs.size(), perRow);
    const float h = (float) area.getHeight() / (float) rowsNeeded;

    for (auto& k : knobs)
        fb.items.add (juce::FlexItem (*k).withWidth (w).withHeight (h));
    fb.performLayout (area.toFloat());
}
