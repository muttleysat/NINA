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

juce::Label* SampleLabLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setMinimumHorizontalScale (0.55f);     // i valori lunghi si stringono invece di essere troncati
    l->setBorderSize ({ 0, 0, 0, 0 });
    return l;
}

void SampleLabLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                             float startAngle, float endAngle, juce::Slider& slider)
{
    using namespace theme;
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f * 0.8f;   // manopole piu' compatte
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
    // Proprieta' "dim": valore non bloccato (segue la cella), disegnato spento; quello bloccato e' arancione
    const bool dim = (bool) slider.getProperties().getWithDefault ("dim", false);
    g.setColour (! enabled ? textDim.withAlpha (0.4f) : (dim ? textDim.withAlpha (0.65f) : accent));
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

// Testo centrato con margini minimi: i pulsanti piccoli (M, S, FX, +, -) mostrano la lettera invece di "..."
void SampleLabLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const float fontH = juce::jlimit (9.0f, 15.0f, (float) b.getHeight() * 0.5f);
    g.setFont (juce::FontOptions (fontH, juce::Font::bold));
    g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                     .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
    g.drawText (b.getButtonText(), b.getLocalBounds().reduced (1, 0), juce::Justification::centred, false);
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

// Fader verticale del mixer (senza casella di testo): lascia sopra e sotto lo spazio per meta' del cursore, cosi' non viene tagliato
juce::Slider::SliderLayout SampleLabLookAndFeel::getSliderLayout (juce::Slider& sl)
{
    if (sl.getSliderStyle() == juce::Slider::LinearVertical && sl.getTextBoxPosition() == juce::Slider::NoTextBox)
    {
        juce::Slider::SliderLayout layout;
        const float k = juce::jmax (0.6f, (float) sl.getWidth() / 44.0f);
        layout.sliderBounds = sl.getLocalBounds().reduced (0, (int) (10.0f * k));
        return layout;
    }
    return LookAndFeel_V4::getSliderLayout (sl);
}

void SampleLabLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                             float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    using namespace theme;

    if (style == juce::Slider::LinearVertical)
    {
        // Fader: scala in dB a sinistra, guida al centro, cursore con la tacca centrale
        const float k = juce::jmax (0.6f, (float) s.getWidth() / 44.0f);
        const float cx = (float) x + (float) width * 0.64f;
        const double mn = s.getMinimum(), mx = s.getMaximum();
        auto yOf = [&] (double v) { return (float) y + (float) height * (1.0f - (float) ((v - mn) / (mx - mn))); };

        g.setFont (juce::FontOptions (juce::jmax (7.5f, 8.5f * k)));
        for (const int db : { 12, 6, 0, -6, -12, -24, -36, -60 })
        {
            if (db < mn || db > mx)
                continue;
            const float ty = yOf ((double) db);
            g.setColour (db == 0 ? accent : textDim.withAlpha (0.7f));
            g.drawHorizontalLine ((int) ty, cx - 8.0f * k, cx - 5.0f * k);
            g.drawText (db > 0 ? "+" + juce::String (db) : juce::String (db), juce::Rectangle<float> ((float) x - 2.0f * k, ty - 6.0f * k, cx - 9.0f * k - (float) x + 2.0f * k, 12.0f * k),
                        juce::Justification::centredRight, false);
        }

        const float tw = 5.0f * k;
        g.setColour (background);
        g.fillRoundedRectangle (cx - tw * 0.5f, (float) y, tw, (float) height, tw * 0.5f);
        g.setColour (accent.withAlpha (0.55f));
        g.fillRoundedRectangle (cx - tw * 0.5f, sliderPos, tw, (float) y + (float) height - sliderPos, tw * 0.5f);

        const float cw = 26.0f * k, ch = 17.0f * k;
        const auto cap = juce::Rectangle<float> (cx - cw * 0.5f, sliderPos - ch * 0.5f, cw, ch);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a505c), 0.0f, cap.getY(), juce::Colour (0xff2a2e36), 0.0f, cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 3.0f * k);
        g.setColour (s.isMouseOverOrDragging() ? accentSoft : border.brighter (0.35f));
        g.drawRoundedRectangle (cap.reduced (0.5f), 3.0f * k, 1.0f);
        g.setColour (text);
        g.fillRect (cap.getX() + 3.0f * k, cap.getCentreY() - 0.8f, cap.getWidth() - 6.0f * k, 1.6f);
        return;
    }

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
    if (s.getProperties().getWithDefault ("bipolar", false))          // dal centro (pan)
    {
        const float mid = (float) x + (float) width * 0.5f;
        g.fillRoundedRectangle (juce::jmin (mid, sliderPos), cy - th * 0.5f, std::abs (sliderPos - mid), th, th * 0.5f);
        g.setColour (textDim);
        g.fillRect (mid - 0.5f, cy - 5.0f, 1.0f, 10.0f);
    }
    else
        g.fillRoundedRectangle ((float) x, cy - th * 0.5f, sliderPos - (float) x, th, th * 0.5f);
    g.setColour (text);
    g.fillEllipse (sliderPos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
}
