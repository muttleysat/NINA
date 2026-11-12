#include "ModulesBar.h"

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

void ModulesBar::addKnob (int module, const juce::String& title, double min, double max, double step, double def, double skewMid,
                          std::function<float (const sl::CellParams&)> get,
                          std::function<void (sl::CellParams&, float)> set,
                          const juce::String& suffix)
{
    Param p;
    p.box = std::make_unique<KnobBox> (title);
    p.module = module;
    p.get = std::move (get);
    p.set = std::move (set);

    auto& s = p.box->slider;
    s.setRange (min, max, step);
    if (skewMid > 0.0)
        s.setSkewFactorFromMidPoint (skewMid);
    s.setDoubleClickReturnValue (true, def);
    s.setTextValueSuffix (suffix);
    s.setNumDecimalPlacesToDisplay (step >= 1.0 ? 0 : (step >= 0.1 ? 1 : 2));

    const int index = (int) params.size();
    s.onValueChange = [this, index]
    {
        if (updating) return;
        auto& prm = params[(size_t) index];
        prm.set (processor.getKit().cells[(size_t) cell], (float) prm.box->slider.getValue());
        edited();
    };

    addAndMakeVisible (*p.box);
    params.push_back (std::move (p));
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
    for (auto& p : params)
        p.box->slider.setValue (p.get (c), juce::dontSendNotification);
    masterKnob.slider.setValue (processor.getKit().masterDb, juce::dontSendNotification);
    updating = false;
    updateEnvelope();
    repaint();
}

void ModulesBar::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const char* titles[numModules] = { "Amp", "Pitch", "Master" };
    for (int i = 0; i < numModules; ++i)
        widgets::drawModule (g, moduleBounds[i].toFloat(), titles[i], s);
}

void ModulesBar::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds();
    const int gap = juce::jmax (6, (int) (8 * s));

    // Larghezze proporzionali al numero di manopole di ogni modulo
    const float units[numModules] = { 7.0f, 2.4f, 2.6f };
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

    for (int m = 0; m < Master; ++m)
    {
        std::vector<KnobBox*> list;
        for (auto& p : params)
            if (p.module == m)
                list.push_back (p.box.get());

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
