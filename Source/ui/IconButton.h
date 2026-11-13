#pragma once

#include "Theme.h"

// Pulsante con una piccola icona disegnata in vettoriale a sinistra del testo (o al centro se non c'e' testo).
class IconButton : public juce::TextButton
{
public:
    enum class Icon { None, Midi, Wave, Layers, Reset, Grid, More, TabMain, TabSeq, Mixer, Cube, Scissors, Trash, ArrowRight };

    explicit IconButton (const juce::String& text = {}, Icon i = Icon::None) : juce::TextButton (text), icon (i) {}
    void setIcon (Icon i) { icon = i; repaint(); }
    void setPrimary (bool p) { primary = p; repaint(); }
    void setIconOnRight (bool r) { iconRight = r; repaint(); }        // pulsante d'azione: pieno arancione con testo scuro

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        const float s = theme::scaleFor (*this);
        if (primary)
        {
            const auto r = getLocalBounds().toFloat().reduced (0.5f);
            const auto base = isEnabled() ? theme::accent : theme::panelHi;
            g.setGradientFill (juce::ColourGradient (base.brighter (down ? 0.0f : (highlighted ? 0.25f : 0.12f)), 0.0f, r.getY(),
                                                     base.darker (0.12f), 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 8.0f * s);
        }
        else
        {
            auto& lf = getLookAndFeel();
            lf.drawButtonBackground (g, *this, findColour (getToggleState() ? buttonOnColourId : buttonColourId), highlighted, down);
        }
        const juce::Colour ink = primary ? juce::Colour (0xff1a1006) : theme::text;
        const juce::Colour iconInk = primary ? juce::Colour (0xff1a1006) : theme::accent;
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
                if (iconRight)
                {
                    iconRect = juce::Rectangle<float> (isz * 1.3f, isz).withCentre ({ b.getRight() - 14.0f * s - isz * 0.65f, b.getCentreY() });
                    textRect = b.withTrimmedRight (b.getRight() - iconRect.getX() + 2.0f * s);
                }
                else
                {
                    iconRect = juce::Rectangle<float> (isz, isz).withCentre ({ b.getX() + 10.0f * s + isz * 0.5f, b.getCentreY() });
                    textRect = b.withTrimmedLeft (iconRect.getRight() - b.getX() + 2.0f * s);
                }
            }
            drawIcon (g, icon, iconRect, isEnabled() ? iconInk : theme::textDim, s);
        }

        if (label.isNotEmpty())
        {
            g.setColour (isEnabled() ? ink : theme::textDim);
            g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
            g.drawText (label, textRect.toNearestInt(), juce::Justification::centred, true);
        }
    }

    static void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> r, juce::Colour colour, float s)
    {
        g.setColour (colour);
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
            case Icon::TabMain:         // griglia di 3x3 puntini
            {
                const float cell = r.getHeight() / 3.0f;
                const float dot = cell * 0.52f;
                const auto o = juce::Rectangle<float> (cell * 3.0f, cell * 3.0f).withCentre (r.getCentre());
                for (int row = 0; row < 3; ++row)
                    for (int col = 0; col < 3; ++col)
                        g.fillEllipse (o.getX() + (float) col * cell + (cell - dot) * 0.5f, o.getY() + (float) row * cell + (cell - dot) * 0.5f, dot, dot);
                break;
            }
            case Icon::TabSeq:          // step di un sequencer: 2 righe di 4 quadratini, alcuni pieni
            {
                const float cw = juce::jmin (r.getWidth() * 0.21f, r.getHeight() * 0.42f), gap = cw * 0.28f;
                const auto o = juce::Rectangle<float> (cw * 4.0f + gap * 3.0f, cw * 2.0f + gap).withCentre (r.getCentre());
                const bool on[2][4] = { { true, false, true, false }, { false, true, false, true } };
                for (int row = 0; row < 2; ++row)
                    for (int col = 0; col < 4; ++col)
                    {
                        const auto q = juce::Rectangle<float> (o.getX() + (float) col * (cw + gap), o.getY() + (float) row * (cw + gap), cw, cw);
                        if (on[row][col])
                            g.fillRoundedRectangle (q, 1.5f);
                        else
                            g.drawRoundedRectangle (q.reduced (0.6f), 1.5f, lw * 0.8f);
                    }
                break;
            }
            case Icon::Mixer:           // tre cursori verticali a altezze diverse
            {
                const float ys[] = { 0.62f, 0.32f, 0.5f };
                for (int k = 0; k < 3; ++k)
                {
                    const float x = r.getX() + r.getWidth() * (0.18f + 0.32f * (float) k);
                    g.drawLine (x, r.getY() + 1.0f, x, r.getBottom() - 1.0f, lw);
                    g.fillRoundedRectangle (x - r.getHeight() * 0.18f, r.getY() + r.getHeight() * ys[k] - r.getHeight() * 0.08f,
                                            r.getHeight() * 0.36f, r.getHeight() * 0.18f, 1.5f);
                }
                break;
            }
            case Icon::Cube:            // cubo isometrico
            {
                const auto c = r.getCentre();
                const float R = r.getHeight() * 0.52f;
                juce::Path hex;
                for (int k = 0; k < 6; ++k)
                {
                    const float a = juce::MathConstants<float>::pi / 3.0f * (float) k - juce::MathConstants<float>::halfPi;
                    const juce::Point<float> p (c.x + std::cos (a) * R, c.y + std::sin (a) * R);
                    if (k == 0) hex.startNewSubPath (p); else hex.lineTo (p);
                }
                hex.closeSubPath();
                g.strokePath (hex, juce::PathStrokeType (lw, juce::PathStrokeType::curved));
                g.drawLine (c.x, c.y, c.x, c.y - R, lw);
                g.drawLine (c.x, c.y, c.x + std::cos (juce::MathConstants<float>::pi / 6.0f) * R, c.y + std::sin (juce::MathConstants<float>::pi / 6.0f) * R, lw);
                g.drawLine (c.x, c.y, c.x - std::cos (juce::MathConstants<float>::pi / 6.0f) * R, c.y + std::sin (juce::MathConstants<float>::pi / 6.0f) * R, lw);
                break;
            }
            case Icon::Scissors:        // forbici: due lame incrociate e due anelli
            {
                const float h = r.getHeight();
                const auto c = r.getCentre();
                const float ring = h * 0.17f;
                g.drawEllipse (c.x - h * 0.30f - ring, c.y + h * 0.30f - ring, ring * 2.0f, ring * 2.0f, lw);
                g.drawEllipse (c.x + h * 0.30f - ring, c.y + h * 0.30f - ring, ring * 2.0f, ring * 2.0f, lw);
                g.drawLine (c.x - h * 0.30f + ring * 0.5f, c.y + h * 0.30f - ring * 0.8f, c.x + h * 0.40f, c.y - h * 0.46f, lw * 1.2f);
                g.drawLine (c.x + h * 0.30f - ring * 0.5f, c.y + h * 0.30f - ring * 0.8f, c.x - h * 0.40f, c.y - h * 0.46f, lw * 1.2f);
                break;
            }
            case Icon::Trash:           // cestino
            {
                const float h = r.getHeight();
                const auto c = r.getCentre();
                juce::Path bin;
                bin.startNewSubPath (c.x - h * 0.30f, c.y - h * 0.18f);
                bin.lineTo (c.x - h * 0.24f, c.y + h * 0.46f);
                bin.lineTo (c.x + h * 0.24f, c.y + h * 0.46f);
                bin.lineTo (c.x + h * 0.30f, c.y - h * 0.18f);
                g.strokePath (bin, juce::PathStrokeType (lw, juce::PathStrokeType::curved));
                g.drawLine (c.x - h * 0.42f, c.y - h * 0.26f, c.x + h * 0.42f, c.y - h * 0.26f, lw);
                g.drawLine (c.x - h * 0.14f, c.y - h * 0.26f, c.x - h * 0.10f, c.y - h * 0.44f, lw);
                g.drawLine (c.x + h * 0.14f, c.y - h * 0.26f, c.x + h * 0.10f, c.y - h * 0.44f, lw);
                g.drawLine (c.x - h * 0.08f, c.y - h * 0.44f, c.x + h * 0.08f, c.y - h * 0.44f, lw);
                g.drawLine (c.x, c.y - h * 0.08f, c.x, c.y + h * 0.34f, lw * 0.8f);
                break;
            }
            case Icon::ArrowRight:      // freccia verso destra
            {
                const auto c = r.getCentre();
                const float w = r.getWidth() * 0.5f;
                g.drawLine (c.x - w, c.y, c.x + w, c.y, lw * 1.2f);
                g.drawLine (c.x + w, c.y, c.x + w - r.getHeight() * 0.28f, c.y - r.getHeight() * 0.28f, lw * 1.2f);
                g.drawLine (c.x + w, c.y, c.x + w - r.getHeight() * 0.28f, c.y + r.getHeight() * 0.28f, lw * 1.2f);
                break;
            }
            case Icon::None: break;
        }
    }

private:
    Icon icon;
    bool primary = false, iconRight = false;
};
