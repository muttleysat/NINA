#include "KnobBox.h"

KnobBox::KnobBox (const juce::String& title)
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

void KnobBox::refreshIndicator()
{
    const int cc = ccQuery ? ccQuery() : -1;
    const bool learning = learningQuery ? learningQuery() : false;
    if (cc != shownCc || learning != shownLearning)
    {
        shownCc = cc;
        shownLearning = learning;
        repaint();
    }
}

void KnobBox::paintOverChildren (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);

    if (shownLearning)
    {
        g.setColour (theme::accent);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f * s, 1.6f);
        g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.5f * s), juce::Font::bold));
        g.drawText ("MOVE CC...", getLocalBounds().removeFromTop ((int) (14 * s)), juce::Justification::centred);
    }
    else if (shownCc >= 0)
    {
        const juce::String tag = "CC" + juce::String (shownCc);
        g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.5f * s), juce::Font::bold));
        const float w = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), tag) + 8.0f * s;
        const auto pill = juce::Rectangle<float> ((float) getWidth() - w - 2.0f, 1.0f, w, 13.0f * s);
        g.setColour (theme::accent.withAlpha (0.22f));
        g.fillRoundedRectangle (pill, 5.0f * s);
        g.setColour (theme::accentSoft);
        g.drawText (tag, pill, juce::Justification::centred);
    }
}

void KnobBox::resized()
{
    const float s = theme::scaleFor (*this);
    auto b = getLocalBounds();
    label.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    const int th = (int) (15 * s);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, b.getWidth(), th);
    label.setBounds (b.removeFromBottom (th));     // etichetta sotto il valore, come nel riferimento
    slider.setBounds (b);
}
