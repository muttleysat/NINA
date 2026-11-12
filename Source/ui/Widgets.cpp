#include "Widgets.h"

namespace widgets
{
void drawModule (juce::Graphics& g, juce::Rectangle<float> b, const juce::String& title, float s)
{
    g.setColour (theme::panel);
    g.fillRoundedRectangle (b, 10.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f * s, 1.0f);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.5f * s), juce::Font::bold));
    g.drawText (title.toUpperCase(), b.reduced (12.0f * s, 6.0f * s).withHeight (18.0f * s), juce::Justification::centredLeft);
}
}

// ------------------------------------------------------------------ LevelMeter

LevelMeter::LevelMeter (std::function<std::pair<float, float>()> getLevels) : levels (std::move (getLevels))
{
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    const auto [nl, nr] = levels();
    // Salita immediata, discesa graduale
    l = nl > l ? nl : l * 0.86f;
    r = nr > r ? nr : r * 0.86f;
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float gap = 3.0f;
    const float bw = (b.getWidth() - gap) * 0.5f;

    auto drawBar = [&] (float x, float level)
    {
        const auto bar = juce::Rectangle<float> (x, b.getY(), bw, b.getHeight());
        g.setColour (theme::background);
        g.fillRoundedRectangle (bar, 2.0f);

        // Scala in dB: -60 .. 0
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        const int segs = 24;
        const float sh = bar.getHeight() / (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const float pos = (float) (i + 1) / (float) segs;
            if (pos > frac)
                break;
            g.setColour (pos > 0.92f ? theme::danger : (pos > 0.75f ? theme::accentSoft : theme::accent));
            g.fillRect (bar.getX() + 1.0f, bar.getBottom() - (float) (i + 1) * sh + 1.0f, bar.getWidth() - 2.0f, sh - 1.5f);
        }
    };
    drawBar (b.getX(), l);
    drawBar (b.getX() + bw + gap, r);
}

// ---------------------------------------------------------------- EnvelopeView

void EnvelopeView::setParams (float attack, float hold, float decay, float sustain, float release)
{
    a = attack; h = hold; d = decay; s = sustain; r = release;
    repaint();
}

void EnvelopeView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    if (b.getWidth() < 10.0f || b.getHeight() < 6.0f)
        return;

    // Asse del tempo non lineare: ogni fase ha una larghezza minima per restare leggibile
    auto seg = [] (float t) { return 0.12f + std::sqrt (juce::jmax (0.0f, t)); };
    const float wa = seg (a), wh = h > 0.0f ? seg (h) * 0.6f : 0.0f, wd = seg (d), wsus = 0.8f, wr = seg (r);
    const float total = wa + wh + wd + wsus + wr;
    const float k = b.getWidth() / total;

    const float yTop = b.getY(), yBot = b.getBottom();
    const float ySus = yBot - s * (yBot - yTop);

    juce::Path p;
    float x = b.getX();
    p.startNewSubPath (x, yBot);
    x += wa * k; p.lineTo (x, yTop);
    if (wh > 0.0f) { x += wh * k; p.lineTo (x, yTop); }
    x += wd * k; p.lineTo (x, ySus);
    x += wsus * k; p.lineTo (x, ySus);
    x += wr * k; p.lineTo (x, yBot);

    juce::Path fill (p);
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (theme::accent.withAlpha (0.35f), 0.0f, yTop,
                                             theme::accent.withAlpha (0.02f), 0.0f, yBot, false));
    g.fillPath (fill);
    g.setColour (theme::accent);
    g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
