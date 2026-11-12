#include "WaveformView.h"

void WaveformView::setSample (sl::SampleDataPtr data, juce::Colour c)
{
    sample = std::move (data);
    colour = c;
    peaksDirty = true;
    repaint();
}

void WaveformView::setRegion (float s, float e)
{
    start = s;
    end = e;
    repaint();
}

void WaveformView::rebuildPeaks()
{
    peaks.clear();
    peaksDirty = false;
    const int w = getWidth();
    if (sample == nullptr || w <= 0)
        return;

    const int n = sample->numSamples();
    peaks.resize ((size_t) w);
    for (int x = 0; x < w; ++x)
    {
        const int a = (int) ((int64_t) x * n / w);
        const int b = juce::jmax (a + 1, (int) ((int64_t) (x + 1) * n / w));
        float lo = 0.0f, hi = 0.0f;
        for (int ch = 0; ch < sample->numChannels(); ++ch)
        {
            const float* d = sample->audio.getReadPointer (ch);
            for (int i = a; i < juce::jmin (b, n); ++i)
            {
                lo = juce::jmin (lo, d[i]);
                hi = juce::jmax (hi, d[i]);
            }
        }
        peaks[(size_t) x] = { lo, hi };
    }
}

float WaveformView::xToNorm (int x) const
{
    return juce::jlimit (0.0f, 1.0f, (float) x / (float) juce::jmax (1, getWidth() - 1));
}

WaveformView::Handle WaveformView::handleAt (int x) const
{
    const float sx = start * (float) (getWidth() - 1), ex = end * (float) (getWidth() - 1);
    const float tol = 10.0f;
    const bool nearS = std::abs ((float) x - sx) <= tol, nearE = std::abs ((float) x - ex) <= tol;
    if (nearS && nearE)
        return (float) x < (sx + ex) * 0.5f ? Handle::Start : Handle::End;
    return nearS ? Handle::Start : (nearE ? Handle::End : Handle::None);
}

void WaveformView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float s = theme::scaleFor (*this);

    g.setColour (theme::background);
    g.fillRoundedRectangle (b, 5.0f * s);

    if (sample == nullptr)
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (13.0f * s));
        g.drawText ("Drop a sample on the selected cell", getLocalBounds(), juce::Justification::centred);
        return;
    }

    if (peaksDirty)
        rebuildPeaks();

    const float mid = b.getCentreY(), half = b.getHeight() * 0.46f;
    const float sx = start * (float) (getWidth() - 1), ex = end * (float) (getWidth() - 1);

    // Zone escluse attenuate
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (0.0f, 0.0f, sx, b.getHeight());
    g.fillRect (ex, 0.0f, b.getWidth() - ex, b.getHeight());

    g.setColour (theme::textDim.withAlpha (0.25f));
    g.drawHorizontalLine ((int) mid, 0.0f, b.getWidth());

    for (int x = 0; x < (int) peaks.size(); ++x)
    {
        const bool inside = (float) x >= sx && (float) x <= ex;
        g.setColour (colour.withAlpha (inside ? 0.95f : 0.3f));
        const auto [lo, hi] = peaks[(size_t) x];
        g.drawVerticalLine (x, mid - hi * half, juce::jmax (mid - hi * half + 1.0f, mid - lo * half));
    }

    // Marker
    auto drawHandle = [&] (float x, const juce::String& label, bool isStart)
    {
        g.setColour (theme::accent);
        g.drawVerticalLine ((int) x, 0.0f, b.getHeight());
        const float w = 16.0f * s, h = 14.0f * s;
        auto tab = juce::Rectangle<float> (isStart ? x : x - w, 0.0f, w, h);
        g.fillRect (tab);
        g.setColour (theme::background);
        g.setFont (juce::FontOptions (10.0f * s, juce::Font::bold));
        g.drawText (label, tab, juce::Justification::centred);
    };
    drawHandle (sx, "S", true);
    drawHandle (ex, "E", false);
}

void WaveformView::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (sample != nullptr && handleAt (e.x) != Handle::None ? juce::MouseCursor::LeftRightResizeCursor
                                                                         : juce::MouseCursor::NormalCursor);
}

void WaveformView::mouseDown (const juce::MouseEvent& e)
{
    dragging = sample != nullptr ? handleAt (e.x) : Handle::None;
}

void WaveformView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Handle::None)
        return;

    const float minGap = 0.002f;
    const float v = xToNorm (e.x);
    if (dragging == Handle::Start)
        start = juce::jmin (v, end - minGap);
    else
        end = juce::jmax (v, start + minGap);

    start = juce::jlimit (0.0f, 1.0f, start);
    end = juce::jlimit (0.0f, 1.0f, end);
    repaint();
    if (onRegionChanged)
        onRegionChanged (start, end);
}

void WaveformView::mouseUp (const juce::MouseEvent&)
{
    if (dragging != Handle::None && onRegionCommitted)
        onRegionCommitted();
    dragging = Handle::None;
}
