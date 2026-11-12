#include "FxPanel.h"

FxPanel::FxPanel (SampleLabProcessor& p) : processor (p)
{
    for (auto* b : { &cellButton, &masterButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (4712);
        addAndMakeVisible (*b);
    }
    cellButton.setToggleState (true, juce::dontSendNotification);
    cellButton.onClick = [this] { masterTarget = false; refresh(); };
    masterButton.onClick = [this] { masterTarget = true; refresh(); };

    using F = sl::FxParams;

    auto& sat = addCard ("Saturation", 2.4f, [] (const F& f) { return f.satOn; }, [] (F& f, bool v) { f.satOn = v; });
    addKnob (sat, "Drive", 0, 36, 0.1, 6, -1, [] (const F& f) { return f.drive; }, [] (F& f, float v) { f.drive = v; }, " dB");
    addKnob (sat, "Mix", 0, 1, 0.01, 1, -1, [] (const F& f) { return f.satMix; }, [] (F& f, float v) { f.satMix = v; });

    auto& lofi = addCard ("LoFi", 3.2f, [] (const F& f) { return f.lofiOn; }, [] (F& f, bool v) { f.lofiOn = v; });
    addKnob (lofi, "Bits", 1, 16, 1, 12, -1, [] (const F& f) { return f.bits; }, [] (F& f, float v) { f.bits = v; });
    addKnob (lofi, "Rate", 1, 40, 0.1, 4, 8, [] (const F& f) { return f.downsample; }, [] (F& f, float v) { f.downsample = v; }, "x");
    addKnob (lofi, "Mix", 0, 1, 0.01, 1, -1, [] (const F& f) { return f.lofiMix; }, [] (F& f, float v) { f.lofiMix = v; });

    auto& filter = addCard ("Filter", 3.2f, [] (const F& f) { return f.filterOn; }, [] (F& f, bool v) { f.filterOn = v; });
    filter.combo = std::make_unique<juce::ComboBox>();
    filter.combo->addItem ("Lowpass", 1);
    filter.combo->addItem ("Highpass", 2);
    filter.combo->addItem ("Bandpass", 3);
    filter.combo->onChange = [this]
    {
        if (updating) return;
        target().filterType = cards[2]->combo->getSelectedId() - 1;
        edited();
    };
    addAndMakeVisible (*filter.combo);
    addKnob (filter, "Cutoff", 20, 20000, 1, 8000, 1000, [] (const F& f) { return f.cutoff; }, [] (F& f, float v) { f.cutoff = v; }, " Hz");
    addKnob (filter, "Reso", 0.3, 10, 0.01, 0.707, 1.0, [] (const F& f) { return f.resonance; }, [] (F& f, float v) { f.resonance = v; });

    auto& comp = addCard ("Compressor", 5.0f, [] (const F& f) { return f.compOn; }, [] (F& f, bool v) { f.compOn = v; });
    addKnob (comp, "Thresh", -60, 0, 0.1, -18, -1, [] (const F& f) { return f.compThreshold; }, [] (F& f, float v) { f.compThreshold = v; }, " dB");
    addKnob (comp, "Ratio", 1, 20, 0.1, 4, 4, [] (const F& f) { return f.compRatio; }, [] (F& f, float v) { f.compRatio = v; }, ":1");
    addKnob (comp, "Attack", 0.1, 100, 0.1, 10, 10, [] (const F& f) { return f.compAttack; }, [] (F& f, float v) { f.compAttack = v; }, " ms");
    addKnob (comp, "Release", 10, 1000, 1, 120, 150, [] (const F& f) { return f.compRelease; }, [] (F& f, float v) { f.compRelease = v; }, " ms");
    addKnob (comp, "Makeup", 0, 24, 0.1, 0, -1, [] (const F& f) { return f.compMakeup; }, [] (F& f, float v) { f.compMakeup = v; }, " dB");

    auto& delay = addCard ("Delay", 3.2f, [] (const F& f) { return f.delayOn; }, [] (F& f, bool v) { f.delayOn = v; });
    addKnob (delay, "Time", 1, 750, 1, 250, 200, [] (const F& f) { return f.delayTime; }, [] (F& f, float v) { f.delayTime = v; }, " ms");
    addKnob (delay, "Feedback", 0, 0.95, 0.01, 0.35, -1, [] (const F& f) { return f.delayFeedback; }, [] (F& f, float v) { f.delayFeedback = v; });
    addKnob (delay, "Mix", 0, 1, 0.01, 0.3, -1, [] (const F& f) { return f.delayMix; }, [] (F& f, float v) { f.delayMix = v; });

    auto& reverb = addCard ("Reverb", 3.2f, [] (const F& f) { return f.reverbOn; }, [] (F& f, bool v) { f.reverbOn = v; });
    addKnob (reverb, "Size", 0, 1, 0.01, 0.5, -1, [] (const F& f) { return f.reverbSize; }, [] (F& f, float v) { f.reverbSize = v; });
    addKnob (reverb, "Damp", 0, 1, 0.01, 0.5, -1, [] (const F& f) { return f.reverbDamp; }, [] (F& f, float v) { f.reverbDamp = v; });
    addKnob (reverb, "Mix", 0, 1, 0.01, 0.25, -1, [] (const F& f) { return f.reverbMix; }, [] (F& f, float v) { f.reverbMix = v; });

    setCell (0);
}

FxPanel::Card& FxPanel::addCard (const juce::String& title, float units,
                                 std::function<bool (const sl::FxParams&)> getOn, std::function<void (sl::FxParams&, bool)> setOn)
{
    auto c = std::make_unique<Card>();
    c->title = title;
    c->units = units;
    c->getOn = std::move (getOn);
    c->setOn = std::move (setOn);
    c->power = std::make_unique<juce::ToggleButton>();

    auto* raw = c.get();
    raw->power->onClick = [this, raw]
    {
        if (updating) return;
        raw->setOn (target(), raw->power->getToggleState());
        edited();
        repaint();
    };
    addAndMakeVisible (*raw->power);
    cards.push_back (std::move (c));
    return *raw;
}

void FxPanel::addKnob (Card& card, const juce::String& title, double min, double max, double step, double def, double skewMid,
                       Getter get, Setter set, const juce::String& suffix)
{
    Knob k;
    k.box = std::make_unique<KnobBox> (title);
    k.get = std::move (get);
    k.set = std::move (set);

    auto& s = k.box->slider;
    s.setRange (min, max, step);
    if (skewMid > 0.0)
        s.setSkewFactorFromMidPoint (skewMid);
    s.setDoubleClickReturnValue (true, def);
    s.setTextValueSuffix (suffix);
    s.setNumDecimalPlacesToDisplay (step >= 1.0 ? 0 : (step >= 0.1 ? 1 : 2));

    // La lambda cattura setter e manopola per valore/puntatore: resta valida anche se il vettore si riallocherà
    auto* box = k.box.get();
    s.onValueChange = [this, set = k.set, box]
    {
        if (updating) return;
        set (target(), (float) box->slider.getValue());
        edited();
    };
    addAndMakeVisible (*box);
    card.knobs.push_back (std::move (k));
}

sl::FxParams& FxPanel::target()
{
    return masterTarget ? processor.getKit().masterFx
                        : processor.getKit().cells[(size_t) juce::jlimit (0, processor.getKit().numCells() - 1, cell)].fx;
}

void FxPanel::edited()
{
    processor.cellEdited (masterTarget ? -1 : cell);
    if (onEdited)
        onEdited();
}

void FxPanel::setCell (int c)
{
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), c);
    refresh();
}

void FxPanel::updateHeader()
{
    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];
    cellButton.setButtonText ("Cell " + theme::cellId (cell, kit.cols) + (c.hasSample() ? "  " + c.name : juce::String()));
    cellButton.setToggleState (! masterTarget, juce::dontSendNotification);
    masterButton.setToggleState (masterTarget, juce::dontSendNotification);
}

void FxPanel::refresh()
{
    if (cell >= processor.getKit().numCells())
        cell = 0;

    updateHeader();
    const auto& fx = target();
    updating = true;
    for (auto& c : cards)
    {
        c->power->setToggleState (c->getOn (fx), juce::dontSendNotification);
        for (auto& k : c->knobs)
            k.box->slider.setValue (k.get (fx), juce::dontSendNotification);
        if (c->combo != nullptr)
            c->combo->setSelectedId (fx.filterType + 1, juce::dontSendNotification);
    }
    updating = false;
    repaint();
}

void FxPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const auto& fx = target();
    for (auto& c : cards)
    {
        widgets::drawModule (g, c->bounds.toFloat(), c->title, s);
        if (c->getOn (fx))
        {
            g.setColour (theme::accent);
            g.drawRoundedRectangle (c->bounds.toFloat().reduced (0.5f), 10.0f * s, 1.2f);
        }
    }

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    g.drawText (masterTarget ? "Effects on the whole kit (after all cells)" : "Effects on the selected cell",
                header.withTrimmedLeft ((int) (300 * s)), juce::Justification::centredLeft);
}

void FxPanel::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds();
    const int gap = juce::jmax (6, (int) (8 * s));

    header = area.removeFromTop ((int) (26 * s));
    auto hb = header;
    cellButton.setBounds (hb.removeFromLeft ((int) (190 * s)));
    hb.removeFromLeft ((int) (6 * s));
    masterButton.setBounds (hb.removeFromLeft ((int) (90 * s)));
    area.removeFromTop ((int) (6 * s));

    float total = 0.0f;
    for (auto& c : cards) total += c->units;
    const float perUnit = (float) (area.getWidth() - gap * ((int) cards.size() - 1)) / total;

    int x = area.getX();
    for (size_t i = 0; i < cards.size(); ++i)
    {
        auto& c = *cards[i];
        const int w = i + 1 == cards.size() ? area.getRight() - x : (int) (c.units * perUnit);
        c.bounds = { x, area.getY(), w, area.getHeight() };
        x += w + gap;

        auto inner = c.bounds.reduced ((int) (6 * s), 0).withTrimmedTop ((int) (24 * s)).withTrimmedBottom ((int) (6 * s));
        const int pw = (int) (36 * s);
        c.power->setBounds (juce::Rectangle<int> (c.bounds.getRight() - pw - (int) (10 * s), c.bounds.getY() + (int) (5 * s), pw, (int) (18 * s)));

        if (c.combo != nullptr)
            c.combo->setBounds (inner.removeFromTop ((int) (24 * s)).reduced ((int) (4 * s), 0));

        const int n = (int) c.knobs.size();
        const int kw = inner.getWidth() / juce::jmax (1, n);
        for (auto& k : c.knobs)
            k.box->setBounds (inner.removeFromLeft (kw));
    }
}
