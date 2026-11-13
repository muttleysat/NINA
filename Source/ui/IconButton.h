#pragma once

#include "Theme.h"

// Pulsante con una piccola icona disegnata in vettoriale a sinistra del testo (o al centro se non c'e' testo).
class IconButton : public juce::TextButton
{
public:
    enum class Icon { None, Midi, Wave, Layers, Reset, Grid, More };

    explicit IconButton (const juce::String& text = {}, Icon i = Icon::None) : juce::TextButton (text), icon (i) {}
    void setIcon (Icon i) { icon = i; repaint(); }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto& lf = getLookAndFeel();
        lf.drawButtonBackground (g, *this, findColour (getToggleState() ? buttonOnColourId : buttonColourId), highlighted, down);

        const float s = theme::scaleFor (*this);
        auto b = getLocalBounds().toFloat();
        const float isz = juce::jmin (b.getHeight() * 0.52f, 15.0f * s);
        const auto label = getButtonText();

        juce::Rectangle<float> iconRect;
        auto textRect = b;
        if (icon != Icon::None)
        {
            if (label.isEmpty())
                iconRect = juce::Rectangle<float> (isz * 1.2f, isz).withCentre (b.getCentre());
            else
            {
                iconRect = juce::Rectangle<float> (isz, isz).withCentre ({ b.getX() + 10.0f * s + isz * 0.5f, b.getCentreY() });
                textRect = b.withTrimmedLeft (iconRect.getRight() - b.getX() + 2.0f * s);
            }
            drawIcon (g, icon, iconRect, isEnabled() ? theme::accent : theme::textDim, s);
        }

        if (label.isNotEmpty())
        {
            g.setColour (isEnabled() ? theme::text : theme::textDim);
            g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
            g.drawText (label, textRect.toNearestInt(), juce::Justification::centred, true);
        }
    }

    static void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> r, juce::Colour c, float s)
    {
        g.setColour (c);
        const float lw = juce::jmax (1.3f, 1.5f * s);
        switch (icon)
        {
            case Icon::Wave:            // forma d'onda: barrette di altezza diversa
            {
                const float hs[] = { 0.35f, 0.7f, 1.0f, 0.6f, 0.85f, 0.45f, 0.25f };
                const float step = r.getWidth() / 7.0f;
                for (int i = 0; i < 7; ++i)
                {
                    const float h = r.getHeight() * hs[i];
                    g.fillRoundedRectangle (r.getX() + (float) i * step + step * 0.2f, r.getCentreY() - h * 0.5f, step * 0.55f, h, 0.8f);
                }
                break;
            }
            case Icon::Layers:          // tre rombi sovrapposti
            {
                for (int k = 0; k < 3; ++k)
                {
                    const float y = r.getY() + r.getHeight() * (0.28f + 0.22f * (float) k);
                    juce::Path p;
                    p.startNewSubPath (r.getX(), y);
                    p.lineTo (r.getCentreX(), y - r.getHeight() * 0.26f);
                    p.lineTo (r.getRight(), y);
                    p.lineTo (r.getCentreX(), y + r.getHeight() * 0.26f);
                    p.closeSubPath();
                    if (k == 0)
                        g.strokePath (p, juce::PathStrokeType (lw, juce::PathStrokeType::curved));
                    else
                    {
                        juce::Path lower;                               // solo la meta' bassa: sembrano pile
                        lower.startNewSubPath (r.getX(), y);
                        lower.lineTo (r.getCentreX(), y + r.getHeight() * 0.26f);
                        lower.lineTo (r.getRight(), y);
                        g.strokePath (lower, juce::PathStrokeType (lw, juce::PathStrokeType::curved));
                    }
                }
                break;
            }
            case Icon::Midi:            // connettore DIN: cerchio con puntini
            {
                const auto e = r.withSizeKeepingCentre (r.getHeight(), r.getHeight()).reduced (1.0f);
                g.drawEllipse (e, lw);
                for (int k = 0; k < 5; ++k)
                {
                    const float a = juce::MathConstants<float>::pi * (1.15f + 0.175f * (float) k);
                    g.fillEllipse (e.getCentreX() + std::cos (a) * e.getWidth() * 0.28f - 1.1f,
                                   e.getCentreY() + std::sin (a) * e.getHeight() * 0.28f - 1.1f, 2.2f, 2.2f);
                }
                break;
            }
            case Icon::Reset:           // freccia circolare
            {
                const auto e = r.withSizeKeepingCentre (r.getHeight(), r.getHeight()).reduced (1.5f);
                juce::Path arc;
                arc.addCentredArc (e.getCentreX(), e.getCentreY(), e.getWidth() * 0.5f, e.getHeight() * 0.5f, 0.0f,
                                   juce::MathConstants<float>::pi * 0.35f, juce::MathConstants<float>::pi * 1.85f, true);
                g.strokePath (arc, juce::PathStrokeType (lw));
                juce::Path head;
                head.addTriangle (e.getRight() - 1.0f, e.getCentreY() - 3.5f, e.getRight() + 3.0f, e.getCentreY() + 1.0f, e.getRight() - 5.0f, e.getCentreY() + 1.5f);
                g.fillPath (head);
                break;
            }
            case Icon::Grid:            // quattro quadrati (uno pieno): "aggiungi tutte"
            {
                const float q = r.getHeight() * 0.42f;
                const auto o = juce::Rectangle<float> (q * 2.0f + 2.0f, q * 2.0f + 2.0f).withCentre (r.getCentre());
                g.fillRoundedRectangle (o.getX(), o.getY(), q, q, 1.5f);
                g.fillRoundedRectangle (o.getX() + q + 2.0f, o.getY(), q, q, 1.5f);
                g.fillRoundedRectangle (o.getX(), o.getY() + q + 2.0f, q, q, 1.5f);
                g.drawRoundedRectangle (o.getX() + q + 2.0f, o.getY() + q + 2.0f, q, q, 1.5f, lw * 0.8f);
                break;
            }
            case Icon::More:            // tre puntini
            {
                for (int k = -1; k <= 1; ++k)
                    g.fillEllipse (r.getCentreX() + (float) k * 5.0f * s - 1.8f * s, r.getCentreY() - 1.8f * s, 3.6f * s, 3.6f * s);
                break;
            }
            case Icon::None: break;
        }
    }

private:
    Icon icon;
};
