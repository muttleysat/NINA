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

    using F = sl::FxF;
    using B = sl::FxB;

    auto& sat = addCard ("Saturation", 2.8f, B::Sat);
    addKnob (sat, "Drive", F::Drive);
    addKnob (sat, "Mix", F::SatMix);

    auto& lofi = addCard ("LoFi", 2.9f, B::Lofi);
    addKnob (lofi, "Bits", F::Bits);
    addKnob (lofi, "Rate", F::Downsample);
    addKnob (lofi, "Mix", F::LofiMix);

    auto& filter = addCard ("Filter", 3.0f, B::Filter);
    filter.combo = std::make_unique<juce::ComboBox>();
    addAndMakeVisible (*filter.combo);
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *filter.combo,
        [this] { return (juce::RangedAudioParameter*) refs().filterType; }));
    addKnob (filter, "Cutoff", F::Cutoff);
    addKnob (filter, "Reso", F::Resonance);

    auto& eq = addCard ("EQ", 5.5f, B::Eq);
    addKnob (eq, "Low", F::EqLow);
    addKnob (eq, "Mid Hz", F::EqMidFreq);
    addKnob (eq, "Mid dB", F::EqMidGain);
    addKnob (eq, "Mid Q", F::EqMidQ);
    addKnob (eq, "High", F::EqHigh);

    auto& comp = addCard ("Compressor", 5.4f, B::Comp);
    addKnob (comp, "Thresh", F::CompThreshold);
    addKnob (comp, "Ratio", F::CompRatio);
    addKnob (comp, "Attack", F::CompAttack);
    addKnob (comp, "Release", F::CompRelease);
    addKnob (comp, "Makeup", F::CompMakeup);

    auto& delay = addCard ("Delay", 3.6f, B::Delay);
    delay.syncButton = std::make_unique<juce::TextButton> ("Sync");
    delay.syncButton->setTooltip ("Lock the delay time to the project tempo");
    delay.divisionCombo = std::make_unique<juce::ComboBox>();
    addAndMakeVisible (*delay.syncButton);
    addAndMakeVisible (*delay.divisionCombo);
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *delay.syncButton,
        [this] { return (juce::RangedAudioParameter*) refs().delaySync; }));
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *delay.divisionCombo,
        [this] { return (juce::RangedAudioParameter*) refs().delayDivision; }));
    addKnob (delay, "Time", F::DelayTime);
    addKnob (delay, "Feedback", F::DelayFeedback);
    addKnob (delay, "Mix", F::DelayMix);

    auto& reverb = addCard ("Reverb", 2.9f, B::Reverb);
    addKnob (reverb, "Size", F::ReverbSize);
    addKnob (reverb, "Damp", F::ReverbDamp);
    addKnob (reverb, "Mix", F::ReverbMix);

    auto& stut = addCard ("Stutter", 2.8f, B::Stutter);
    stut.gridCombo = std::make_unique<juce::ComboBox>();
    stut.gridCombo->setTooltip ("Length of the repeated slice (musical value, follows the tempo)");
    addAndMakeVisible (*stut.gridCombo);
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *stut.gridCombo,
        [this] { return (juce::RangedAudioParameter*) refs().stutterGrid; }));
    addKnob (stut, "Decay", F::StutterDecay);
    addKnob (stut, "Mix", F::StutterMix);

    setCell (0);
    startTimerHz (10);
}

void FxPanel::timerCallback()
{
    updateSyncState();
    for (auto& c : cards)
        for (auto& k : c->knobs)
            k.box->refreshIndicator();
    unsigned mask = 0;
    const auto& r = refs();
    for (size_t i = 0; i < cards.size(); ++i)
        if (r.power (cards[i]->effect)->get())
            mask |= 1u << i;

    if (mask != lastPowerMask)
    {
        lastPowerMask = mask;
        repaint();
    }

    // La curva dell'EQ segue i valori (anche se cambiano dall'automazione)
    {
        using F = sl::FxF;
        const float key = r.at (F::EqLow)->get() * 1.0f + r.at (F::EqMidFreq)->get() * 0.013f + r.at (F::EqMidGain)->get() * 3.1f
                          + r.at (F::EqMidQ)->get() * 7.7f + r.at (F::EqHigh)->get() * 11.3f;
        if (key != lastEqKey)
        {
            lastEqKey = key;
            for (auto& c : cards)
                if (c->effect == sl::FxB::Eq)
                    repaint (c->curve);
        }
    }
}

FxPanel::Card& FxPanel::addCard (const juce::String& title, float units, sl::FxB effect)
{
    auto c = std::make_unique<Card>();
    c->title = title;
    c->units = units;
    c->effect = effect;
    c->power = std::make_unique<juce::ToggleButton>();
    addAndMakeVisible (*c->power);

    // MIDI learn sull'interruttore dell'effetto
    const auto effect_ = effect;
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *c->power,
        [this, effect_] { return (juce::RangedAudioParameter*) refs().power (effect_); }));

    cards.push_back (std::move (c));
    return *cards.back();
}

void FxPanel::addKnob (Card& card, const juce::String& title, sl::FxF which)
{
    Knob k;
    k.box = std::make_unique<KnobBox> (title);
    k.which = which;
    addAndMakeVisible (*k.box);

    auto param = [this, which] { return (juce::RangedAudioParameter*) refs().at (which); };
    k.box->ccQuery = [this, param] { return processor.getMidiLearn().ccFor (param()); };
    k.box->learningQuery = [this, param] { return processor.getMidiLearn().isLearning (param()); };
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *k.box, param));

    card.knobs.push_back (std::move (k));
}

const sl::FxRefs& FxPanel::refs() const
{
    return masterTarget ? processor.getParams().masterFx() : processor.getParams().cell (cell).fx;
}

void FxPanel::setCell (int c)
{
    cell = juce::jlimit (0, sl::kNumCells - 1, c);
    refresh();
}

void FxPanel::updateHeader()
{
    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];
    cellButton.setButtonText ("Cell " + sl::cellLabel (cell, kit.cols) + (c.hasSample() ? "  " + c.name : juce::String()));
    cellButton.setToggleState (! masterTarget, juce::dontSendNotification);
    masterButton.setToggleState (masterTarget, juce::dontSendNotification);
}

void FxPanel::refresh()
{
    updateHeader();

    // Ricollega ogni controllo al parametro giusto (cella selezionata o master)
    const auto& r = refs();
    for (auto& c : cards)
    {
        c->powerAttachment.reset();
        c->powerAttachment = std::make_unique<juce::ButtonParameterAttachment> (*r.power (c->effect), *c->power);

        if (c->combo != nullptr)
        {
            c->comboAttachment.reset();
            c->combo->clear (juce::dontSendNotification);
            c->combo->addItemList (r.filterType->choices, 1);
            c->comboAttachment = std::make_unique<juce::ComboBoxParameterAttachment> (*r.filterType, *c->combo);
        }

        for (auto& k : c->knobs)
        {
            k.attachment.reset();
            k.attachment = std::make_unique<juce::SliderParameterAttachment> (*r.at (k.which), k.box->slider);
        }

        if (c->gridCombo != nullptr)
        {
            c->gridAttachment.reset();
            c->gridCombo->clear (juce::dontSendNotification);
            c->gridCombo->addItemList (r.stutterGrid->choices, 1);
            c->gridAttachment = std::make_unique<juce::ComboBoxParameterAttachment> (*r.stutterGrid, *c->gridCombo);
        }

        if (c->syncButton != nullptr)
        {
            c->syncAttachment.reset();
            c->syncAttachment = std::make_unique<juce::ButtonParameterAttachment> (*r.delaySync, *c->syncButton);
            c->divisionAttachment.reset();
            c->divisionCombo->clear (juce::dontSendNotification);
            c->divisionCombo->addItemList (r.delayDivision->choices, 1);
            c->divisionAttachment = std::make_unique<juce::ComboBoxParameterAttachment> (*r.delayDivision, *c->divisionCombo);
        }
    }
    updateSyncState();
    repaint();
}

// Con il sync acceso il tempo libero (ms) non ha effetto: la manopola Time si disattiva
void FxPanel::updateSyncState()
{
    const bool sync = refs().delaySync->get();
    for (auto& c : cards)
        if (c->syncButton != nullptr)
            for (auto& k : c->knobs)
                if (k.which == sl::FxF::DelayTime)
                {
                    k.box->setEnabled (! sync);
                    k.box->setAlpha (sync ? 0.4f : 1.0f);
                    c->divisionCombo->setEnabled (sync);
                    c->divisionCombo->setAlpha (sync ? 1.0f : 0.5f);
                }
}

void FxPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const auto& r = refs();
    for (auto& c : cards)
    {
        widgets::drawModule (g, c->bounds.toFloat(), c->title, s);
        if (r.power (c->effect)->get())
        {
            g.setColour (theme::accent);
            g.drawRoundedRectangle (c->bounds.toFloat().reduced (0.5f), 10.0f * s, 1.2f);
        }

        if (c->effect == sl::FxB::Eq && ! c->curve.isEmpty())
        {
            // Curva di risposta in frequenza (20 Hz - 20 kHz, scala logaritmica) e guadagno (+-18 dB)
            const auto cr = c->curve.toFloat();
            const bool on = r.power (sl::FxB::Eq)->get();
            g.setColour (theme::background);
            g.fillRoundedRectangle (cr, 5.0f * s);
            g.setColour (theme::border);
            g.drawRoundedRectangle (cr, 5.0f * s, 1.0f);

            auto xOf = [&] (double f) { return cr.getX() + cr.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
            auto yOf = [&] (float db) { return cr.getCentreY() - juce::jlimit (-18.0f, 18.0f, db) / 18.0f * cr.getHeight() * 0.5f; };
            g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.0f * s)));
            for (const double f : { 100.0, 1000.0, 10000.0 })
            {
                g.setColour (theme::border.withAlpha (0.8f));
                g.drawVerticalLine ((int) xOf (f), cr.getY() + 1.0f, cr.getBottom() - 1.0f);
                g.setColour (theme::textDim);
                g.drawText (f >= 1000.0 ? juce::String ((int) (f / 1000.0)) + "k" : juce::String ((int) f),
                            juce::Rectangle<float> (xOf (f) + 2.0f, cr.getBottom() - 12.0f * s, 28.0f * s, 11.0f * s), juce::Justification::centredLeft);
            }
            for (const float db : { -12.0f, 12.0f })
            {
                g.setColour (theme::border.withAlpha (0.5f));
                g.drawHorizontalLine ((int) yOf (db), cr.getX() + 1.0f, cr.getRight() - 1.0f);
            }
            g.setColour (theme::textDim.withAlpha (0.8f));
            g.drawHorizontalLine ((int) yOf (0.0f), cr.getX() + 1.0f, cr.getRight() - 1.0f);

            sl::FxParams fx;
            fx.eqOn = true;
            fx.eqLow = r.at (sl::FxF::EqLow)->get();
            fx.eqMidFreq = r.at (sl::FxF::EqMidFreq)->get();
            fx.eqMidGain = r.at (sl::FxF::EqMidGain)->get();
            fx.eqMidQ = r.at (sl::FxF::EqMidQ)->get();
            fx.eqHigh = r.at (sl::FxF::EqHigh)->get();

            juce::Path line, fill;
            const int steps = juce::jmax (16, (int) cr.getWidth() / 2);
            for (int i = 0; i <= steps; ++i)
            {
                const double f = 20.0 * std::pow (1000.0, (double) i / (double) steps);
                const float x = cr.getX() + cr.getWidth() * (float) i / (float) steps;
                const float y = yOf (sl::FxChain::eqResponseDb (fx, f));
                if (i == 0) { line.startNewSubPath (x, y); fill.startNewSubPath (x, yOf (0.0f)); fill.lineTo (x, y); }
                else { line.lineTo (x, y); fill.lineTo (x, y); }
            }
            fill.lineTo (cr.getRight(), yOf (0.0f));
            fill.closeSubPath();
            const auto col = on ? theme::accent : theme::textDim;
            g.setColour (col.withAlpha (0.18f));
            g.fillPath (fill);
            g.setColour (col);
            g.strokePath (line, juce::PathStrokeType (1.8f * s));
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
        if (c.gridCombo != nullptr)
            c.gridCombo->setBounds (inner.removeFromTop ((int) (24 * s)).reduced ((int) (4 * s), 0));
        if (c.effect == sl::FxB::Eq)
            c.curve = inner.removeFromTop (inner.getHeight() * 40 / 100).reduced ((int) (4 * s), (int) (2 * s));

        if (c.syncButton != nullptr)
        {
            auto row = inner.removeFromTop ((int) (24 * s)).reduced ((int) (4 * s), 0);
            c.syncButton->setBounds (row.removeFromLeft (row.getWidth() * 38 / 100));
            row.removeFromLeft ((int) (4 * s));
            c.divisionCombo->setBounds (row);
        }

        const int n = (int) c.knobs.size();
        const int kw = inner.getWidth() / juce::jmax (1, n);
        for (auto& k : c.knobs)
            k.box->setBounds (inner.removeFromLeft (kw));
    }
}
