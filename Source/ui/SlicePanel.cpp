#include "SlicePanel.h"
#include "../engine/SliceDetect.h"
#include "../engine/LoopStretch.h"
#include <algorithm>

namespace
{
juce::String timeLabel (double seconds, double step)
{
    return juce::String (seconds, step >= 1.0 ? 0 : (step >= 0.1 ? 1 : (step >= 0.01 ? 2 : 3)));
}
}

// =============================================================== Forma d'onda con i marcatori

class SlicePanel::WaveView final : public juce::Component
{
public:
    explicit WaveView (SlicePanel& o) : owner (o)
    {
        for (auto* b : { &zoomOut, &zoomFit, &zoomIn })
        {
            b->setWantsKeyboardFocus (false);
            addAndMakeVisible (*b);
        }
        zoomOut.onClick = [this] { setZoom (zoom / 1.6f); };
        zoomIn.onClick = [this] { setZoom (zoom * 1.6f); };
        zoomFit.onClick = [this] { setZoom (1.0f); };
        zoomOut.setTooltip ("Zoom out");
        zoomIn.setTooltip ("Zoom in (mouse wheel pans, Ctrl + wheel zooms)");
        zoomFit.setTooltip ("Show the whole region");
    }

    void reset()
    {
        zoom = 1.0f;
        centre = 0.5f * (owner.regionStart() + owner.regionEnd());
        hover = -1;
        peaksDirty = true;
        repaint();
    }

    void touch() { peaksDirty = true; repaint(); }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto all = getLocalBounds().toFloat();
        g.setColour (theme::panel);
        g.fillRoundedRectangle (all, 10.0f * s);
        g.setColour (theme::border);
        g.drawRoundedRectangle (all.reduced (0.5f), 10.0f * s, 1.0f);

        const auto d = owner.data();
        if (d == nullptr)
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * s)));
            g.drawFittedText ("This cell has no sample.\nSelect a cell with a sample in the Main tab, or drop one on it.",
                              getLocalBounds().reduced (20), juce::Justification::centred, 3);
            layoutButtons();
            return;
        }

        const auto area = waveArea();
        const auto cellColour = juce::Colour (owner.processor.getKit().cells[(size_t) owner.cell].colour);
        const float rs = owner.regionStart(), re = owner.regionEnd();
        const auto [a, b] = view();

        // Righello del tempo
        const double n = (double) d->numSamples(), sr = d->sampleRate;
        const double secondsPerPixel = (double) (b - a) * n / sr / juce::jmax (1.0f, area.getWidth());
        double step = 0.01;
        for (const double c : { 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0 })
        {
            step = c;
            if (c / secondsPerPixel >= 70.0 * s)
                break;
        }
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
        const double t0 = (double) a * n / sr, t1 = (double) b * n / sr;
        for (double t = std::ceil (t0 / step) * step; t <= t1 + 1.0e-9; t += step)
        {
            const float x = xOf ((float) (t * sr / n));
            g.setColour (theme::textDim.withAlpha (0.8f));
            g.drawText (timeLabel (t, step), juce::Rectangle<float> (x - 30.0f * s, 6.0f * s, 60.0f * s, 14.0f * s), juce::Justification::centred);
            g.setColour (theme::border.brighter (0.4f));
            g.drawLine (x, area.getY() - 12.0f * s - 18.0f * s, x, area.getY() - 18.0f * s, 1.0f);
        }

        // Sfondo della forma d'onda
        g.setColour (juce::Colour (0xff111317));
        g.fillRoundedRectangle (area, 6.0f * s);

        rebuildPeaks (*d, a, b, juce::jmax (1, (int) area.getWidth()));

        // Slice: tinta alternata, evidenziazione sotto il mouse
        const auto bnds = owner.bounds();
        for (int i = 0; i + 1 < (int) bnds.size(); ++i)
        {
            const float x0 = juce::jlimit (area.getX(), area.getRight(), xOf (bnds[(size_t) i]));
            const float x1 = juce::jlimit (area.getX(), area.getRight(), xOf (bnds[(size_t) i + 1]));
            if (x1 <= x0)
                continue;
            if (i % 2 == 1)
            {
                g.setColour (juce::Colours::white.withAlpha (0.035f));
                g.fillRect (juce::Rectangle<float> (x0, area.getY(), x1 - x0, area.getHeight()));
            }
        }

        // Forma d'onda (un canale per metà altezza se stereo)
        const int chans = (int) peaks.size();
        for (int ch = 0; ch < chans; ++ch)
        {
            const float laneH = area.getHeight() / (float) chans;
            const float mid = area.getY() + laneH * ((float) ch + 0.5f), half = laneH * 0.46f;
            const auto& pk = peaks[(size_t) ch];
            if (pk.empty())
                continue;
            juce::Path p;
            p.startNewSubPath (area.getX(), mid);
            for (int x = 0; x < (int) pk.size(); ++x)
                p.lineTo (area.getX() + (float) x, mid - pk[(size_t) x].second * half);
            for (int x = (int) pk.size() - 1; x >= 0; --x)
                p.lineTo (area.getX() + (float) x, mid - pk[(size_t) x].first * half);
            p.closeSubPath();
            g.setColour (cellColour.withAlpha (0.92f));
            g.fillPath (p);
        }

        // Slice sotto il mouse: tinta arancione, icona play e durata
        if (hover >= 0 && hover + 1 < (int) bnds.size())
        {
            const float x0 = juce::jlimit (area.getX(), area.getRight(), xOf (bnds[(size_t) hover]));
            const float x1 = juce::jlimit (area.getX(), area.getRight(), xOf (bnds[(size_t) hover + 1]));
            const auto sr2 = juce::Rectangle<float> (x0, area.getY(), x1 - x0, area.getHeight());
            g.setColour (theme::accent.withAlpha (0.28f));
            g.fillRect (sr2);
            g.setColour (theme::accent.withAlpha (0.9f));
            g.drawRect (sr2, 1.2f);

            if (sr2.getWidth() > 34.0f * s)
            {
                const auto c = sr2.getCentre();
                const float R = 18.0f * s;
                g.setColour (juce::Colours::white.withAlpha (0.92f));
                g.drawEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f, 2.0f);
                juce::Path tri;
                tri.addTriangle (c.x - R * 0.3f, c.y - R * 0.45f, c.x - R * 0.3f, c.y + R * 0.45f, c.x + R * 0.5f, c.y);
                g.fillPath (tri);
            }
            const double ms = (double) (bnds[(size_t) hover + 1] - bnds[(size_t) hover]) * n / sr * 1000.0;
            const juce::String label = juce::String (juce::roundToInt (ms)) + " ms";
            const float lw = juce::jmin (sr2.getWidth() - 4.0f, 64.0f * s);
            if (lw > 24.0f)
            {
                const auto pill = juce::Rectangle<float> (lw, 20.0f * s).withCentre ({ sr2.getCentreX(), sr2.getBottom() - 20.0f * s });
                g.setColour (juce::Colour (0xff0d0e11).withAlpha (0.85f));
                g.fillRoundedRectangle (pill, 6.0f * s);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
                g.drawText (label, pill, juce::Justification::centred);
            }
        }

        // Marcatori: linea arancione e bandierina numerata
        for (int i = 0; i < (int) owner.markers.size(); ++i)
        {
            const float x = xOf (owner.markers[(size_t) i]);
            if (x < area.getX() - 1.0f || x > area.getRight() + 1.0f)
                continue;
            g.setColour (theme::accent);
            g.drawLine (x, area.getY() - 4.0f * s, x, area.getBottom(), i == dragIndex ? 2.4f : 1.6f);
            const auto flag = juce::Rectangle<float> (22.0f * s, 20.0f * s).withCentre ({ x, area.getY() - 14.0f * s });
            g.setColour (i == dragIndex ? theme::accentSoft : theme::accent);
            g.fillRoundedRectangle (flag, 5.0f * s);
            g.setColour (juce::Colour (0xff1a1006));
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
            g.drawText (juce::String (i + 1), flag, juce::Justification::centred);
            g.setColour (theme::accent);
            g.fillEllipse (x - 3.0f * s, area.getY() + 2.0f * s, 6.0f * s, 6.0f * s);
        }

        // Inizio e fine della regione della cella: parentesi arancioni
        for (const float p : { rs, re })
        {
            const float x = xOf (p);
            if (x < area.getX() - 2.0f || x > area.getRight() + 2.0f)
                continue;
            const bool left = p == rs;
            g.setColour (theme::accent);
            g.drawLine (x, area.getY(), x, area.getBottom(), 2.2f);
            const float tick = 8.0f * s * (left ? 1.0f : -1.0f);
            g.drawLine (x, area.getY(), x + tick, area.getY(), 2.2f);
            g.drawLine (x, area.getBottom(), x + tick, area.getBottom(), 2.2f);
        }

        // Chop: cursore di lettura
        if (owner.chopping)
        {
            const float x = xOf ((float) owner.chopPosition (juce::Time::getMillisecondCounterHiRes()));
            if (x >= area.getX() && x <= area.getRight())
            {
                g.setColour (juce::Colours::white.withAlpha (0.95f));
                g.drawLine (x, area.getY() - 6.0f * s, x, area.getBottom(), 1.8f);
            }
        }

        // Suggerimento
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
        g.drawText ("Click a slice to hear it - drag markers - double-click to add - right-click to remove",
                    juce::Rectangle<float> (area.getX() + 4.0f * s, area.getBottom() + 6.0f * s, area.getWidth() - 140.0f * s, 18.0f * s),
                    juce::Justification::centredLeft);
        layoutButtons();
    }

    void resized() override { layoutButtons(); peaksDirty = true; }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int near = markerNear (e.x);
        setMouseCursor (near >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
        const int h = near >= 0 ? -1 : sliceAtX ((float) e.x, (float) e.y);
        if (h != hover)
        {
            hover = h;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        if (hover != -1)
        {
            hover = -1;
            repaint();
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (owner.data() == nullptr || ! waveArea().expanded (0.0f, 30.0f).contains (e.position))
            return;

        const int near = markerNear (e.x);
        if (e.mods.isPopupMenu())
        {
            if (near >= 0 && owner.markers.size() > 1)
            {
                owner.markers.erase (owner.markers.begin() + near);
                owner.markersChanged();
                hover = -1;
                repaint();
            }
            return;
        }

        if (near >= 0)
        {
            dragIndex = near;
            repaint();
            return;
        }

        const int slice = sliceAtX ((float) e.x, (float) e.y);
        if (slice >= 0)
            owner.auditionSlice (slice);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragIndex < 0)
            return;
        auto& m = owner.markers;
        const auto d = owner.data();
        const float minGap = d != nullptr ? (float) (0.004 * d->sampleRate / (double) d->numSamples()) : 0.001f;
        const float lo = dragIndex > 0 ? m[(size_t) dragIndex - 1] + minGap : owner.regionStart();
        const float hi = dragIndex + 1 < (int) m.size() ? m[(size_t) dragIndex + 1] - minGap : owner.regionEnd() - minGap;
        m[(size_t) dragIndex] = juce::jlimit (lo, juce::jmax (lo, hi), pos ((float) e.x));
        repaint();
        owner.cellsStripRepaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragIndex >= 0)
        {
            dragIndex = -1;
            owner.markersChanged();
        }
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (owner.data() == nullptr || e.mods.isPopupMenu() || markerNear (e.x) >= 0 || ! waveArea().contains (e.position))
            return;
        const float p = pos ((float) e.x);
        if (p <= owner.regionStart() || p >= owner.regionEnd())
            return;
        owner.markers.push_back (p);
        std::sort (owner.markers.begin(), owner.markers.end());
        owner.markersChanged();
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (juce::ModifierKeys::currentModifiers.isCtrlDown())
        {
            setZoom (zoom * (w.deltaY > 0 ? 1.25f : 0.8f));
            return;
        }
        const auto [a, b] = view();
        centre += (b - a) * (w.deltaY != 0.0f ? -w.deltaY : -w.deltaX) * 0.5f;
        clampCentre();
        peaksDirty = true;
        repaint();
    }

private:
    juce::Rectangle<float> waveArea() const
    {
        const float s = theme::scaleFor (*this);
        return getLocalBounds().toFloat().withTrimmedTop (44.0f * s).withTrimmedBottom (34.0f * s).reduced (12.0f * s, 0.0f);
    }

    std::pair<float, float> view() const
    {
        const float rs = owner.regionStart(), re = owner.regionEnd();
        const float span = (re - rs) / zoom;
        const float a = juce::jlimit (rs, juce::jmax (rs, re - span), centre - span * 0.5f);
        return { a, a + span };
    }

    void clampCentre()
    {
        const float rs = owner.regionStart(), re = owner.regionEnd();
        const float span = (re - rs) / zoom;
        centre = juce::jlimit (rs + span * 0.5f, re - span * 0.5f, centre);
    }

    void setZoom (float z)
    {
        zoom = juce::jlimit (1.0f, 64.0f, z);
        clampCentre();
        peaksDirty = true;
        repaint();
    }

    float xOf (float p) const
    {
        const auto area = waveArea();
        const auto [a, b] = view();
        return area.getX() + (p - a) / juce::jmax (1.0e-9f, b - a) * area.getWidth();
    }

    float pos (float x) const
    {
        const auto area = waveArea();
        const auto [a, b] = view();
        return juce::jlimit (0.0f, 1.0f, a + (x - area.getX()) / juce::jmax (1.0f, area.getWidth()) * (b - a));
    }

    int markerNear (int x) const
    {
        const float s = theme::scaleFor (*this);
        for (int i = 0; i < (int) owner.markers.size(); ++i)
            if (std::abs (xOf (owner.markers[(size_t) i]) - (float) x) <= 7.0f * s)
                return i;
        return -1;
    }

    int sliceAtX (float x, float y) const
    {
        const auto area = waveArea();
        if (y < area.getY() - 4.0f || y > area.getBottom() || x < area.getX() || x > area.getRight())
            return -1;
        const float p = pos (x);
        const auto bnds = owner.bounds();
        for (int i = 0; i + 1 < (int) bnds.size(); ++i)
            if (p >= bnds[(size_t) i] && p < bnds[(size_t) i + 1])
                return i;
        return -1;
    }

    void layoutButtons()
    {
        const float s = theme::scaleFor (*this);
        const int bw = (int) (34 * s), bh = (int) (26 * s);
        auto r = getLocalBounds().withTrimmedRight ((int) (14 * s)).withTrimmedBottom ((int) (6 * s)).removeFromBottom (bh).removeFromRight (bw * 3 + (int) (8 * s));
        zoomOut.setBounds (r.removeFromLeft (bw));
        r.removeFromLeft ((int) (4 * s));
        zoomFit.setBounds (r.removeFromLeft (bw));
        r.removeFromLeft ((int) (4 * s));
        zoomIn.setBounds (r.removeFromLeft (bw));
    }

    // Picchi min/max per colonna e per canale nella finestra visibile
    void rebuildPeaks (const sl::SampleData& d, float a, float b, int width)
    {
        if (! peaksDirty && lastData == &d && width == lastWidth && a == lastA && b == lastB)
            return;
        peaksDirty = false;
        lastData = &d;
        lastWidth = width;
        lastA = a;
        lastB = b;

        const int n = d.numSamples();
        const int chans = juce::jmin (2, d.numChannels());
        peaks.assign ((size_t) chans, std::vector<std::pair<float, float>> ((size_t) width, { 0.0f, 0.0f }));
        for (int ch = 0; ch < chans; ++ch)
        {
            const float* src = d.audio.getReadPointer (ch);
            for (int x = 0; x < width; ++x)
            {
                const int i0 = juce::jlimit (0, n, (int) std::floor (((double) a + (double) (b - a) * x / width) * (double) n));
                const int i1 = juce::jlimit (i0 + 1, n, (int) std::floor (((double) a + (double) (b - a) * (x + 1) / width) * (double) n));
                float lo = 0.0f, hi = 0.0f;
                for (int i = i0; i < i1 && i < n; ++i)
                {
                    lo = std::min (lo, src[i]);
                    hi = std::max (hi, src[i]);
                }
                peaks[(size_t) ch][(size_t) x] = { juce::jlimit (-1.0f, 1.0f, lo), juce::jlimit (-1.0f, 1.0f, hi) };
            }
        }
    }

    SlicePanel& owner;
    float zoom = 1.0f, centre = 0.5f;
    int hover = -1, dragIndex = -1;
    bool peaksDirty = true;
    const sl::SampleData* lastData = nullptr;
    int lastWidth = 0;
    float lastA = 0.0f, lastB = 0.0f;
    std::vector<std::vector<std::pair<float, float>>> peaks;
    juce::TextButton zoomOut { "-" }, zoomFit { "[ ]" }, zoomIn { "+" };
};

// =============================================================== Celle di destinazione

class SlicePanel::CellsStrip final : public juce::Component
{
public:
    explicit CellsStrip (SlicePanel& o) : owner (o) {}

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto& kit = owner.processor.getKit();
        g.setColour (theme::panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f * s);
        g.setColour (theme::border);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f * s, 1.0f);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
        g.drawText ("CELLS", juce::Rectangle<int> (getLocalBounds().getX() + (int) (16 * s), (int) (8 * s), (int) (200 * s), (int) (18 * s)),
                    juce::Justification::centredLeft);

        const auto targets = owner.targetCells();
        for (int i = 0; i < kit.numCells(); ++i)
        {
            const auto b = cellRect (i);
            const int slot = (int) (std::find (targets.begin(), targets.end(), i) - targets.begin());
            const bool isTarget = slot < (int) targets.size();
            const bool original = i == owner.cell;
            const bool occupied = isTarget && kit.cells[(size_t) i].hasSample();

            if (isTarget && ! occupied)
            {
                g.setGradientFill (juce::ColourGradient (theme::accentSoft, 0.0f, b.getY(), theme::accent, 0.0f, b.getBottom(), false));
                g.fillRoundedRectangle (b, 7.0f * s);
            }
            else
            {
                g.setColour (theme::panelHi);
                g.fillRoundedRectangle (b, 7.0f * s);
            }
            if (occupied)
            {
                g.setColour (theme::danger.withAlpha (0.18f));
                g.fillRoundedRectangle (b, 7.0f * s);
                g.setColour (theme::danger);
                g.drawRoundedRectangle (b.reduced (0.7f), 7.0f * s, 1.5f);
            }
            else if (original)
            {
                g.setColour (theme::text);
                g.drawRoundedRectangle (b.reduced (0.7f), 7.0f * s, 1.8f);
            }
            else if (! isTarget)
            {
                g.setColour (theme::border);
                g.drawRoundedRectangle (b.reduced (0.5f), 7.0f * s, 1.0f);
            }

            const auto id = theme::cellId (i, kit.cols);
            const juce::Colour ink = (isTarget && ! occupied) ? juce::Colour (0xff1a1006) : theme::text;
            g.setColour (ink);
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 13.0f * s), juce::Font::bold));
            if (original || isTarget)
            {
                g.drawText (id, b.withTrimmedBottom (b.getHeight() * 0.42f), juce::Justification::centred);
                g.setFont (juce::FontOptions (juce::jmax (7.0f, 9.5f * s)));
                const juce::String sub = occupied ? juce::String ("occupied") : (isTarget ? juce::String (slot + 1) : juce::String ("original"));
                g.drawText (sub, b.withTrimmedTop (b.getHeight() * 0.50f), juce::Justification::centred);
            }
            else
                g.drawText (id, b, juce::Justification::centred);

            if (occupied)
            {
                juce::Path tri;
                const float tx = b.getX() + 6.0f * s, ty = b.getY() + 5.0f * s, tw = 10.0f * s;
                tri.addTriangle (tx + tw * 0.5f, ty, tx + tw, ty + tw * 0.9f, tx, ty + tw * 0.9f);
                g.setColour (theme::danger);
                g.fillPath (tri);
            }
        }

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
        for (int r = 0; r < kit.rows; ++r)
            g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + r)), labelRect (r), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < owner.processor.getKit().numCells(); ++i)
            if (cellRect (i).contains (e.position))
            {
                owner.setTargetStart (i);
                return;
            }
    }

private:
    juce::Rectangle<float> cellRect (int i) const
    {
        const float s = theme::scaleFor (*this);
        const auto& kit = owner.processor.getKit();
        const float labelW = 34.0f * s, top = 30.0f * s, bottom = 10.0f * s, left = 12.0f * s, right = 14.0f * s, gap = 7.0f * s;
        const float cw = (getWidth() - labelW - left - right - gap * (float) (kit.cols - 1)) / (float) kit.cols;
        const float ch = (getHeight() - top - bottom - gap * (float) (kit.rows - 1)) / (float) kit.rows;
        const int r = i / kit.cols, c = i % kit.cols;
        return { left + labelW + (float) c * (cw + gap), top + (float) r * (ch + gap), cw, ch };
    }

    juce::Rectangle<int> labelRect (int row) const
    {
        const auto first = cellRect (row * owner.processor.getKit().cols);
        return juce::Rectangle<float> (first.getX() - 34.0f * theme::scaleFor (*this), first.getY(), 30.0f * theme::scaleFor (*this), first.getHeight()).toNearestInt();
    }

    SlicePanel& owner;
};

// =============================================================== SlicePanel

SlicePanel::SlicePanel (SampleLabProcessor& p) : processor (p)
{
    wave = std::make_unique<WaveView> (*this);
    cellsStrip = std::make_unique<CellsStrip> (*this);
    addAndMakeVisible (*wave);
    addAndMakeVisible (*cellsStrip);

    detectButton.setPrimary (true);
    detectButton.setTooltip ("Find the hits in the sample and put a marker on each one. Use Sensitivity to find more or fewer.");
    detectButton.onClick = [this] { doDetect(); };
    addAndMakeVisible (detectButton);

    sensitivity.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    sensitivity.setRange (0.0, 100.0, 1.0);
    sensitivity.setValue (60.0, juce::dontSendNotification);
    sensitivity.setTextValueSuffix (" %");
    sensitivity.setNumDecimalPlacesToDisplay (0);
    sensitivity.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
    sensitivity.setTooltip ("Detect sensitivity: higher finds more (and softer) hits");
    sensitivity.onDragEnd = [this] { if (data() != nullptr) doDetect(); };
    sensitivity.onValueChange = [this] { if (! sensitivity.isMouseButtonDown() && data() != nullptr) doDetect(); };
    addAndMakeVisible (sensitivity);

    int id = 1;
    for (const int n : { 2, 4, 8, 16, 32 })
        evenBox.addItem ("Even: 1/" + juce::String (n), id++);
    evenBox.setTextWhenNothingSelected ("Even: 1/16");
    evenBox.setTooltip ("Cut the region into equal slices");
    evenBox.onChange = [this]
    {
        const int sel = evenBox.getSelectedId();
        if (sel > 0)
            doEven (std::vector<int> { 2, 4, 8, 16, 32 }[(size_t) sel - 1]);
        evenBox.setSelectedId (0, juce::dontSendNotification);
        evenBox.setText ("Even: 1/" + juce::String (std::vector<int> { 2, 4, 8, 16, 32 }[(size_t) juce::jmax (1, sel) - 1]), juce::dontSendNotification);
    };
    addAndMakeVisible (evenBox);

    chopButton.setTooltip ("Play the region and press M (or a MIDI note) to drop a marker at the playing position. Click again to stop.");
    chopButton.onClick = [this] { if (chopping) stopChop(); else startChop(); };
    addAndMakeVisible (chopButton);

    clearButton.setTooltip ("Remove all markers");
    clearButton.onClick = [this] { doClear(); };
    addAndMakeVisible (clearButton);

    auditionToggle.setToggleState (true, juce::dontSendNotification);
    auditionToggle.setColour (juce::ToggleButton::textColourId, theme::text);
    auditionToggle.setTooltip ("Play a slice when you click it");
    addAndMakeVisible (auditionToggle);
    auditionVolume.setSliderStyle (juce::Slider::LinearHorizontal);
    auditionVolume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    auditionVolume.setRange (-24.0, 6.0, 0.5);
    auditionVolume.setValue (0.0, juce::dontSendNotification);
    auditionVolume.setTooltip ("Audition volume");
    addAndMakeVisible (auditionVolume);

    keepToggle.setToggleState (true, juce::dontSendNotification);
    keepToggle.setColour (juce::ToggleButton::textColourId, theme::text);
    keepToggle.setTooltip ("On: the slices go to the cells after the original, which stays as it is. Off: they start in the original's own cell.");
    keepToggle.onClick = [this]
    {
        keepOriginal = keepToggle.getToggleState();
        targetStart = -1;
        cellsStrip->repaint();
        repaint();
    };
    addAndMakeVisible (keepToggle);

    distributeButton.setPrimary (true);
    distributeButton.setIconOnRight (true);
    distributeButton.setTooltip ("Fill the highlighted cells with the slices: each cell plays its own part of the same sample file");
    distributeButton.onClick = [this] { doDistribute(); };
    addAndMakeVisible (distributeButton);

    theme::noButtonFocus (*this);
    setCell (0);
}

SlicePanel::~SlicePanel() { processor.setChopCapture (false); }

float SlicePanel::regionStart() const
{
    const auto& refs = processor.getParams().cell (cell);
    return juce::jlimit (0.0f, 1.0f, refs.at (sl::CellF::Start)->get());
}

float SlicePanel::regionEnd() const
{
    const auto& refs = processor.getParams().cell (cell);
    return juce::jlimit (regionStart(), 1.0f, refs.at (sl::CellF::End)->get());
}

std::vector<float> SlicePanel::bounds() const
{
    auto b = markers;
    b.push_back (regionEnd());
    return b;
}

int SlicePanel::firstTarget() const
{
    return targetStart >= 0 ? targetStart : (keepOriginal ? cell + 1 : cell);
}

std::vector<int> SlicePanel::targetCells() const
{
    std::vector<int> t;
    if (data() == nullptr)
        return t;
    for (int i = 0; i < (int) markers.size(); ++i)
        if (firstTarget() + i < processor.getKit().numCells())
            t.push_back (firstTarget() + i);
    return t;
}

void SlicePanel::setCell (int c)
{
    stopChop();
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), c);
    targetStart = -1;
    markers = processor.getKit().cells[(size_t) cell].slices;
    refresh();
    wave->reset();
}

void SlicePanel::refresh()
{
    if (cell >= processor.getKit().numCells())
        cell = 0;

    // Marcatori sempre dentro la regione (Start/End possono essere cambiati nella scheda Main) e almeno uno: l'inizio
    const float rs = regionStart(), re = regionEnd();
    std::vector<float> m;
    for (const float v : markers)
        if (v >= rs - 1.0e-6f && v < re - 1.0e-6f)
            m.push_back (v);
    if (m.empty() || m.front() > rs + 1.0e-6f)
        m.insert (m.begin(), rs);
    markers = m;

    const bool has = data() != nullptr && ! processor.getKit().cells[(size_t) cell].missing;
    for (auto* c : std::initializer_list<juce::Component*> { &detectButton, &sensitivity, &evenBox, &clearButton, &chopButton, &distributeButton })
        c->setEnabled (has);
    keepToggle.setButtonText ("Keep original in " + theme::cellId (cell, processor.getKit().cols));
    wave->touch();
    cellsStrip->repaint();
    repaint();
}

void SlicePanel::cellsStripRepaint()
{
    if (cellsStrip != nullptr)
        cellsStrip->repaint();
}

void SlicePanel::markersChanged()
{
    processor.getKit().cells[(size_t) cell].slices = markers;
    processor.markDirty();
    cellsStrip->repaint();
    repaint();
}

void SlicePanel::auditionSlice (int index)
{
    if (! auditionToggle.getToggleState())
        return;
    const auto b = bounds();
    if (index < 0 || index + 1 >= (int) b.size())
        return;
    processor.auditionCellRegion (cell, b[(size_t) index], b[(size_t) index + 1], (float) auditionVolume.getValue());
}

void SlicePanel::doDetect()
{
    const auto d = data();
    if (d == nullptr)
        return;
    markers = sl::detectSlices (*d, (float) sensitivity.getValue() / 100.0f, regionStart(), regionEnd());
    markersChanged();
    wave->touch();
}

void SlicePanel::doEven (int count)
{
    if (data() == nullptr)
        return;
    markers = sl::evenSlices (regionStart(), regionEnd(), count);
    markersChanged();
    wave->touch();
}

void SlicePanel::doClear()
{
    markers = { regionStart() };
    markersChanged();
    wave->touch();
}

void SlicePanel::setTargetStart (int cellIndex)
{
    targetStart = cellIndex;
    cellsStrip->repaint();
    repaint();
}

void SlicePanel::showStatus (const juce::String& text)
{
    status = text;
    statusUntil = juce::Time::getMillisecondCounterHiRes() + 4000.0;
    startTimerHz (60);
    repaint();
}

void SlicePanel::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (chopping)
    {
        const auto head = processor.chopNoteCount();
        for (auto i = juce::jmax (chopSeen, head > 32 ? head - 32 : 0u); i < head; ++i)
            addChopMarker (processor.chopNoteTime (i));
        chopSeen = head;

        if (now - chopStartMs >= chopDurMs + 60.0)
            stopChop (true);
        else
            wave->repaint();
    }
    if (status.isNotEmpty() && now >= statusUntil)
    {
        status.clear();
        repaint();
    }
    if (! chopping && status.isEmpty())
        stopTimer();
}

double SlicePanel::chopPosition (double nowMs) const
{
    const float rs = regionStart(), re = regionEnd();
    const double t = chopDurMs > 0.0 ? juce::jlimit (0.0, 1.0, (nowMs - chopStartMs) / chopDurMs) : 0.0;
    return (double) rs + t * (double) (re - rs);
}

void SlicePanel::startChop()
{
    const auto d = data();
    if (d == nullptr || processor.getKit().cells[(size_t) cell].missing)
        return;
    const float rs = regionStart(), re = regionEnd();
    if (re <= rs)
        return;

    markers = { rs };                                          // si riparte da zero: il primo marcatore e' l'inizio
    markersChanged();
    chopDurMs = (double) (re - rs) * (double) d->numSamples() / d->sampleRate * 1000.0;
    chopSeen = processor.chopNoteCount();
    processor.setChopCapture (true);
    processor.auditionCellRegion (cell, rs, re, (float) auditionVolume.getValue());
    chopStartMs = juce::Time::getMillisecondCounterHiRes();
    chopping = true;
    chopButton.setButtonText ("Stop chop");
    chopButton.setPrimary (true);
    startTimerHz (60);
    wave->touch();
}

void SlicePanel::stopChop (bool finished)
{
    if (! chopping)
        return;
    chopping = false;
    processor.setChopCapture (false);
    if (! finished)
        processor.stopAudition();
    chopButton.setButtonText ("Chop");
    chopButton.setPrimary (false);
    showStatus ("Chop done: " + juce::String ((int) markers.size()) + " slice" + (markers.size() == 1 ? "" : "s")
                + " - adjust the markers, then Distribute");
    wave->touch();
}

void SlicePanel::chopMark()
{
    if (chopping)
        addChopMarker (juce::Time::getMillisecondCounterHiRes());
}

void SlicePanel::addChopMarker (double timeMs)
{
    const auto d = data();
    if (! chopping || d == nullptr)
        return;
    const float rs = regionStart(), re = regionEnd();
    const double t = (timeMs - chopStartMs) / chopDurMs;
    if (t <= 0.0 || t >= 1.0)
        return;
    const float p = rs + (float) t * (re - rs);
    const float minGap = (float) (0.004 * d->sampleRate / (double) d->numSamples());
    for (const float m : markers)
        if (std::abs (m - p) < minGap)
            return;
    if (re - p < minGap)
        return;
    markers.push_back (p);
    std::sort (markers.begin(), markers.end());
    markersChanged();
    wave->repaint();
}

void SlicePanel::doDistribute()
{
    const auto targets = targetCells();
    if (targets.empty() || data() == nullptr)
        return;

    int occupied = 0;
    for (const int t : targets)
        occupied += processor.getKit().cells[(size_t) t].hasSample() ? 1 : 0;

    const auto first = firstTarget();
    const auto b = bounds();
    juce::Component::SafePointer<SlicePanel> safe (this);
    auto run = [safe, b, first]
    {
        if (safe == nullptr)
            return;
        const int n = safe->processor.distributeSlices (safe->cell, b, first);
        safe->targetStart = -1;
        if (safe->onKitChanged)
            safe->onKitChanged();
        safe->refresh();
        safe->showStatus ("Distributed " + juce::String (n) + " slice" + (n == 1 ? "" : "s") + " to the cells");
    };

    if (occupied == 0)
    {
        run();
        return;
    }
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle ("Distribute to cells")
                                      .withMessage (juce::String (occupied) + " of the target cells already contain a sample and will be replaced. Continue?")
                                      .withButton ("Replace")
                                      .withButton ("Cancel")
                                      .withAssociatedComponent (this),
                                  [run] (int r) { if (r == 1) run(); });
}

void SlicePanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];

    auto drawCard = [&] (juce::Rectangle<int> r)
    {
        g.setColour (theme::panel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f * s);
        g.setColour (theme::border);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f * s, 1.0f);
    };
    drawCard (toolbarRect);
    drawCard (actionRect);
    if (! infoRect.isEmpty())
        drawCard (infoRect);

    // Barra degli strumenti: titolo, nome del campione, separatori, etichetta della sensibilita'
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (13.0f, 20.0f * s), juce::Font::bold));
    g.drawText ("SLICE", titleRect.withWidth ((int) (74 * s)), juce::Justification::centredLeft);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s)));
    g.drawText (theme::cellId (cell, kit.cols) + (c.hasSample() ? " - " + c.name : juce::String()),
                titleRect.withTrimmedLeft ((int) (82 * s)), juce::Justification::centredLeft, true);
    g.setColour (theme::border);
    for (const int x : toolbarSeps)
        g.drawLine ((float) x, (float) toolbarRect.getY() + 10.0f * s, (float) x, (float) toolbarRect.getBottom() - 10.0f * s, 1.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
    g.drawText ("Sensitivity", sensLabelRect, juce::Justification::centredLeft);

    // Riga delle azioni
    const auto targets = targetCells();
    juce::String summary;
    if (data() == nullptr)
        summary = "No sample in this cell";
    else
    {
        const int slices = (int) markers.size();
        summary = juce::String (slices) + " slice" + (slices == 1 ? "" : "s");
        if (! targets.empty())
        {
            int occupied = 0;
            for (const int t : targets)
                occupied += kit.cells[(size_t) t].hasSample() ? 1 : 0;
            summary += "  -  " + theme::cellId (targets.front(), kit.cols) + " to " + theme::cellId (targets.back(), kit.cols);
            if ((int) targets.size() < slices)
                summary += "  -  only " + juce::String ((int) targets.size()) + " cells available";
            if (occupied > 0)
                summary += "  -  " + juce::String (occupied) + " cell" + (occupied == 1 ? "" : "s") + " will be overwritten";
        }
    }
    g.setColour (status.isNotEmpty() ? theme::accent : theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), status.isNotEmpty() ? juce::Font::bold : juce::Font::plain));
    if (chopping)
    {
        summary = "Chopping: " + juce::String ((int) markers.size() - 1) + " cut" + (markers.size() == 2 ? "" : "s") + " - press M or a MIDI note";
        g.setColour (theme::accent);
    }
    g.drawText (status.isNotEmpty() && ! chopping ? status : summary, actionRect.reduced ((int) (18 * s), 0).withTrimmedLeft ((int) (126 * s)).withTrimmedRight ((int) (470 * s)),
                juce::Justification::centredLeft, true);

    // Scheda informazioni sul campione
    if (! infoRect.isEmpty())
    {
        auto r = infoRect.reduced ((int) (16 * s), (int) (14 * s));
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 16.0f * s), juce::Font::bold));
        g.drawText (theme::cellId (cell, kit.cols) + (c.hasSample() ? " - " + c.name : juce::String()), r.removeFromTop ((int) (24 * s)),
                    juce::Justification::centredLeft, true);
        r.removeFromTop ((int) (10 * s));
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
        g.drawText ("SAMPLE INFO", r.removeFromTop ((int) (20 * s)), juce::Justification::centredLeft);

        if (const auto d = data())
        {
            const double secs = (double) d->numSamples() / d->sampleRate;
            const double bpm = sl::estimateLoopBpm (*d, regionStart(), regionEnd());
            const juce::String rows[4][2] = {
                { "Format", juce::String (d->sampleRate / 1000.0, 1) + " kHz - " + (d->numChannels() > 1 ? "stereo" : "mono") },
                { "Length", juce::String (secs, 3) + " s" },
                { "BPM (est.)", bpm > 0.0 ? juce::String (bpm, 1) : juce::String ("--") },
                { "Slices", juce::String ((int) markers.size()) } };
            for (auto& row : rows)
            {
                auto line = r.removeFromTop ((int) (24 * s));
                g.setColour (theme::textDim);
                g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
                g.drawText (row[0], line.removeFromLeft (line.getWidth() * 40 / 100), juce::Justification::centredLeft);
                g.setColour (theme::text);
                g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.5f * s), juce::Font::bold));
                g.drawText (row[1], line, juce::Justification::centredLeft, true);
            }
        }
    }
}

void SlicePanel::resized()
{
    const float s = theme::scaleFor (*this);
    const int gap = (int) (10 * s);
    auto area = getLocalBounds();

    if (area.getWidth() > (int) (1180 * s))                     // la scheda informazioni solo se c'e' spazio per tutti i comandi
    {
        infoRect = area.removeFromRight ((int) (236 * s));
        area.removeFromRight (gap);
    }
    else
        infoRect = {};

    // Barra degli strumenti
    toolbarRect = area.removeFromTop ((int) (58 * s));
    area.removeFromTop (gap);
    toolbarSeps.clear();
    {
        auto r = toolbarRect.reduced ((int) (14 * s), (int) (9 * s));
        titleRect = r.removeFromLeft ((int) (196 * s));
        r.removeFromLeft ((int) (8 * s));
        toolbarSeps.push_back (r.getX() - (int) (4 * s));

        detectButton.setBounds (r.removeFromLeft ((int) (104 * s)));
        r.removeFromLeft ((int) (10 * s));
        auto sens = r.removeFromLeft ((int) (146 * s));
        sensLabelRect = juce::Rectangle<int> (sens.getX() + (int) (46 * s), toolbarRect.getY() + (int) (5 * s), (int) (100 * s), (int) (13 * s));
        sensitivity.setBounds (sens.withTrimmedTop ((int) (10 * s)));
        r.removeFromLeft ((int) (8 * s));
        toolbarSeps.push_back (r.getX());
        r.removeFromLeft ((int) (10 * s));

        evenBox.setBounds (r.removeFromLeft ((int) (128 * s)));
        r.removeFromLeft ((int) (10 * s));
        toolbarSeps.push_back (r.getX());
        r.removeFromLeft ((int) (10 * s));
        clearButton.setBounds (r.removeFromLeft ((int) (96 * s)));

        auto right = r.removeFromRight ((int) (230 * s));
        toolbarSeps.push_back (right.getX() - (int) (6 * s));
        auditionToggle.setBounds (right.removeFromLeft ((int) (104 * s)).withSizeKeepingCentre ((int) (104 * s), (int) (26 * s)));
        auditionVolume.setBounds (right.reduced (0, (int) (4 * s)));
    }

    // Azioni in basso
    actionRect = area.removeFromBottom ((int) (62 * s));
    area.removeFromBottom (gap);
    {
        auto r = actionRect.reduced ((int) (14 * s), (int) (10 * s));
        chopButton.setBounds (r.removeFromLeft ((int) (112 * s)));
        distributeButton.setBounds (r.removeFromRight ((int) (230 * s)));
        r.removeFromRight ((int) (16 * s));
        keepToggle.setBounds (r.removeFromRight ((int) (230 * s)).withSizeKeepingCentre ((int) (230 * s), (int) (26 * s)));
    }

    // Celle: quattro righe
    cellsRect = area.removeFromBottom (juce::jmax ((int) (150 * s), juce::jmin ((int) (214 * s), area.getHeight() * 36 / 100)));
    area.removeFromBottom (gap);
    cellsStrip->setBounds (cellsRect);

    waveRect = area;
    wave->setBounds (waveRect);
}
