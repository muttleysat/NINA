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
