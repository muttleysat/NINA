#include "ModulesBar.h"

ModulesBar::KnobBox::KnobBox (const juce::String& title)
{
    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, theme::textDim);
    label.setInterceptsMouseClicks (false, false);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);

    addAndMakeVisible (label);
    addAndMakeVisible (slider);
}

void ModulesBar::KnobBox::resized()
{
    const float s = theme::scaleFor (*this);
    auto b = getLocalBounds();
    label.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    const int th = (int) (15 * s);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, b.getWidth(), th);
    label.setBounds (b.removeFromBottom (th));     // etichetta sotto il valore, come nel riferimento
    slider.setBounds (b);
}

ModulesBar::ModulesBar (SampleLabProcessor& p)
    : processor (p), meter ([this] { return processor.getMasterLevels(); })
{
    using P = sl::CellParams;
    addKnob (Amp, "Attack", 0, 2, 0.001, 0.0005, 0.1, [] (const P& c) { return c.attack; }, [] (P& c, float v) { c.attack = v; }, " s");
    addKnob (Amp, "Hold", 0, 2, 0.001, 0, 0.1, [] (const P& c) { return c.hold; }, [] (P& c, float v) { c.hold = v; }, " s");
    addKnob (Amp, "Decay", 0, 5, 0.001, 0.1, 0.5, [] (const P& c) { return c.decay; }, [] (P& c, float v) { c.decay = v; }, " s");
    addKnob (Amp, "Sustain", 0, 1, 0.01, 1, -1, [] (const P& c) { return c.sustain; }, [] (P& c, float v) { c.sustain = v; });
    addKnob (Amp, "Release", 0, 5, 0.001, 0.02, 0.5, [] (const P& c) { return c.release; }, [] (P& c, float v) { c.release = v; }, " s");
    addKnob (Amp, "Level", -60, 12, 0.1, 0, -1, [] (const P& c) { return c.levelDb; }, [] (P& c, float v) { c.levelDb = v; }, " dB");
    addKnob (Amp, "Pan", -1, 1, 0.01, 0, -1, [] (const P& c) { return c.pan; }, [] (P& c, float v) { c.pan = v; });

    addKnob (Pitch, "Tune", -100, 100, 1, 0, -1, [] (const P& c) { return c.tuneCents; }, [] (P& c, float v) { c.tuneCents = v; }, " ct");
    addKnob (Pitch, "Pitch", -24, 24, 1, 0, -1, [] (const P& c) { return c.pitchSemis; }, [] (P& c, float v) { c.pitchSemis = v; }, " st");

    auto& root = addKnob (Mapping, "Root", 0, 127, 1, 36, -1, [] (const P& c) { return (float) c.rootNote; }, [] (P& c, float v) { c.rootNote = (int) v; });
    auto& low  = addKnob (Mapping, "Low", 0, 127, 1, 36, -1, [] (const P& c) { return (float) c.lowNote; }, [] (P& c, float v) { c.lowNote = (int) v; });
    auto& high = addKnob (Mapping, "High", 0, 127, 1, 36, -1, [] (const P& c) { return (float) c.highNote; }, [] (P& c, float v) { c.highNote = (int) v; });
    for (auto* k : { &root, &low, &high })
    {
        k->slider.textFromValueFunction = [] (double v) { return theme::noteName ((int) v); };
        k->slider.valueFromTextFunction = [] (const juce::String& t) { return (double) t.getIntValue(); };
        k->slider.updateText();
    }

    addAndMakeVisible (envelope);

    // Master (volume del kit, non legato alla cella selezionata)
    masterKnob.slider.setRange (-60.0, 6.0, 0.1);
    masterKnob.slider.setDoubleClickReturnValue (true, 0.0);
    masterKnob.slider.setTextValueSuffix (" dB");
    masterKnob.slider.setNumDecimalPlacesToDisplay (1);
    masterKnob.slider.onValueChange = [this]
    {
        if (updating) return;
        processor.setMasterDb ((float) masterKnob.slider.getValue());
        if (onEdited) onEdited();
    };
    addAndMakeVisible (masterKnob);
    addAndMakeVisible (meter);

    setCell (0);
}

ModulesBar::KnobBox& ModulesBar::addKnob (int module, const juce::String& title, double min, double max, double step, double def, double skewMid,
                                          std::function<float (const sl::CellParams&)> get,
                                          std::function<void (sl::CellParams&, float)> set,
                                          const juce::String& suffix)
{
    auto k = std::make_unique<KnobBox> (title);
    k->module = module;
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

void ModulesBar::edited()
{
    processor.cellEdited (cell);
    updateEnvelope();
    if (onEdited)
        onEdited();
}

void ModulesBar::updateEnvelope()
{
    const auto& c = processor.getKit().cells[(size_t) cell];
    envelope.setParams (c.attack, c.hold, c.decay, c.sustain, c.release);
}

void ModulesBar::setCell (int c)
{
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), c);
    refresh();
}

void ModulesBar::refresh()
{
    if (cell >= processor.getKit().numCells())
        cell = 0;

    const auto& c = processor.getKit().cells[(size_t) cell];
    updating = true;
    for (auto& k : knobs)
        k->slider.setValue (k->get (c), juce::dontSendNotification);
    masterKnob.slider.setValue (processor.getKit().masterDb, juce::dontSendNotification);
    updating = false;
    updateEnvelope();
    repaint();
}

void ModulesBar::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const char* titles[numModules] = { "Amp", "Pitch", "Mapping", "Master" };
    for (int i = 0; i < numModules; ++i)
        widgets::drawModule (g, moduleBounds[i].toFloat(), titles[i], s);
}

void ModulesBar::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds();
    const int gap = juce::jmax (6, (int) (8 * s));

    // Larghezze proporzionali al numero di manopole di ogni modulo
    const float units[numModules] = { 7.0f, 2.3f, 3.0f, 2.7f };
    float total = 0.0f;
    for (float u : units) total += u;
    const float perUnit = (float) (area.getWidth() - gap * (numModules - 1)) / total;

    int x = area.getX();
    for (int i = 0; i < numModules; ++i)
    {
        const int w = i == numModules - 1 ? area.getRight() - x : (int) (units[i] * perUnit);
        moduleBounds[i] = { x, area.getY(), w, area.getHeight() };
        x += w + gap;
    }

    const int titleH = (int) (24 * s);
    auto inner = [&] (int m) { return moduleBounds[m].reduced ((int) (6 * s), 0).withTrimmedTop (titleH).withTrimmedBottom ((int) (6 * s)); };

    // Knob di ogni modulo in fila
    for (int m = 0; m < numModules - 1; ++m)
    {
        std::vector<KnobBox*> list;
        for (auto& k : knobs)
            if (k->module == m)
                list.push_back (k.get());

        auto r = inner (m);
        if (m == Amp)
        {
            // Sotto le manopole, il disegno dell'inviluppo
            const int graphH = juce::jmax (0, (int) (r.getHeight() * 0.30f));
            envelope.setBounds (r.removeFromBottom (graphH).reduced ((int) (6 * s), (int) (2 * s)));
        }
        const int w = r.getWidth() / (int) list.size();
        for (auto* k : list)
            k->setBounds (r.removeFromLeft (w));
    }

    // Master: manopola + meter
    auto mr = inner (Master);
    meter.setBounds (mr.removeFromRight (juce::jmax (18, (int) (30 * s))).reduced (0, (int) (6 * s)));
    masterKnob.setBounds (mr);
}
