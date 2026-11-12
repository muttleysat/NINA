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

    auto& sat = addCard ("Saturation", 2.4f, B::Sat);
    addKnob (sat, "Drive", F::Drive);
    addKnob (sat, "Mix", F::SatMix);

    auto& lofi = addCard ("LoFi", 3.2f, B::Lofi);
    addKnob (lofi, "Bits", F::Bits);
    addKnob (lofi, "Rate", F::Downsample);
    addKnob (lofi, "Mix", F::LofiMix);

    auto& filter = addCard ("Filter", 3.2f, B::Filter);
    filter.combo = std::make_unique<juce::ComboBox>();
    addAndMakeVisible (*filter.combo);
    addKnob (filter, "Cutoff", F::Cutoff);
    addKnob (filter, "Reso", F::Resonance);

    auto& comp = addCard ("Compressor", 5.0f, B::Comp);
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
    addKnob (delay, "Time", F::DelayTime);
    addKnob (delay, "Feedback", F::DelayFeedback);
    addKnob (delay, "Mix", F::DelayMix);

    auto& reverb = addCard ("Reverb", 3.2f, B::Reverb);
    addKnob (reverb, "Size", F::ReverbSize);
    addKnob (reverb, "Damp", F::ReverbDamp);
    addKnob (reverb, "Mix", F::ReverbMix);

    setCell (0);
    startTimerHz (10);
}

void FxPanel::timerCallback()
{
    updateSyncState();
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
}

FxPanel::Card& FxPanel::addCard (const juce::String& title, float units, sl::FxB effect)
{
    auto c = std::make_unique<Card>();
    c->title = title;
    c->units = units;
    c->effect = effect;
    c->power = std::make_unique<juce::ToggleButton>();
    addAndMakeVisible (*c->power);
    cards.push_back (std::move (c));
    return *cards.back();
}

void FxPanel::addKnob (Card& card, const juce::String& title, sl::FxF which)
{
    Knob k;
    k.box = std::make_unique<KnobBox> (title);
    k.which = which;
    addAndMakeVisible (*k.box);
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
