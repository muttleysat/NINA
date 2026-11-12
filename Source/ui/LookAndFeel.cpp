#include "LookAndFeel.h"

SampleLabLookAndFeel::SampleLabLookAndFeel()
{
    using namespace theme;
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::TextButton::buttonColourId, panelHi);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1a1006));
    setColour (juce::ComboBox::backgroundColourId, panelHi);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, border);
    setColour (juce::ComboBox::arrowColourId, textDim);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff1a1006));
    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::textBoxTextColourId, accentSoft);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, panelHi);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::ScrollBar::thumbColourId, border.brighter (0.2f));
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::TextEditor::backgroundColourId, panelHi);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, border);
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
}

void SampleLabLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                             float startAngle, float endAngle, juce::Slider& slider)
{
    using namespace theme;
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    if (radius < 6.0f)
        return;

    const auto centre = bounds.getCentre();
    const float arcW = juce::jmax (2.5f, radius * 0.15f);
    const float arcR = radius - arcW * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool enabled = slider.isEnabled();

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (panelHi.brighter (0.12f));
    g.strokePath (track, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arco del valore: dallo zero per i parametri bipolari (pan, tune, pitch), dall'inizio per gli altri
    float from = startAngle;
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
        from = startAngle + (float) slider.valueToProportionOfLength (0.0) * (endAngle - startAngle);

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (enabled ? accent : textDim.withAlpha (0.4f));
    g.strokePath (value, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Corpo
    const float bodyR = arcR - arcW * 1.1f;
    juce::ColourGradient grad (juce::Colour (0xff343841), centre.x, centre.y - bodyR,
                               juce::Colour (0xff15171b), centre.x, centre.y + bodyR, false);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (border.brighter (0.2f));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Indicatore
    const float s = std::sin (angle), c = -std::cos (angle);
    g.setColour (enabled ? text : textDim);
    g.drawLine (centre.x + s * bodyR * 0.3f, centre.y + c * bodyR * 0.3f,
                centre.x + s * bodyR * 0.88f, centre.y + c * bodyR * 0.88f, juce::jmax (1.8f, radius * 0.07f));
}

void SampleLabLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    using namespace theme;
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto c = findColour (on ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);
    if (down) c = c.darker (0.2f);
    else if (highlighted) c = c.brighter (0.12f);

    g.setColour (c);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? accent.brighter (0.2f) : border);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void SampleLabLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    using namespace theme;
    const float h = juce::jmin (18.0f, (float) b.getHeight() - 4.0f);
    const float w = h * 1.8f;
    const auto pill = juce::Rectangle<float> (2.0f, ((float) b.getHeight() - h) * 0.5f, w, h);
    const bool on = b.getToggleState();

    g.setColour (on ? accent : panelHi.brighter (0.1f));
    g.fillRoundedRectangle (pill, h * 0.5f);
    g.setColour (highlighted ? text : (on ? accent.brighter (0.2f) : border));
    g.drawRoundedRectangle (pill, h * 0.5f, 1.0f);
    const float d = h - 6.0f;
    g.setColour (on ? juce::Colour (0xff1a1006) : textDim);
    g.fillEllipse (on ? pill.getRight() - d - 3.0f : pill.getX() + 3.0f, pill.getCentreY() - d * 0.5f, d, d);

    g.setColour (b.findColour (juce::ToggleButton::textColourId));
    g.setFont (juce::FontOptions (juce::jmax (10.0f, (float) b.getHeight() * 0.6f)));
    g.drawText (b.getButtonText(), b.getLocalBounds().withTrimmedLeft ((int) w + 10), juce::Justification::centredLeft);
}

void SampleLabLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                             float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    using namespace theme;
    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0, 0, style, s);
        return;
    }

    const float cy = (float) y + (float) height * 0.5f;
    const float th = 4.0f;
    g.setColour (panelHi.brighter (0.12f));
    g.fillRoundedRectangle ((float) x, cy - th * 0.5f, (float) width, th, th * 0.5f);
    g.setColour (accent);
    g.fillRoundedRectangle ((float) x, cy - th * 0.5f, sliderPos - (float) x, th, th * 0.5f);
    g.setColour (text);
    g.fillEllipse (sliderPos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
}
