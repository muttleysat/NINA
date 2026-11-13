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
        centre = 0.5f * (owner.viewLo() + owner.viewHi());
        hover = -1;
        peaksDirty = true;
        repaint();
    }

    void touch() { peaksDirty = true; repaint(); }

    // Durante l'ascolto con lo zoom la vista segue il cursore (a pagine); se muovi la vista a mano si ferma per qualche secondo
    void follow (float p)
    {
        if (zoom <= 1.001f || juce::Time::getMillisecondCounterHiRes() < holdUntil)
            return;
        const auto [a, b] = view();
        const float span = b - a;
        if (p < a + span * 0.05f || p > b - span * 0.15f)
        {
            centre = p + span * 0.35f;
            clampCentre();
            peaksDirty = true;
        }
    }

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
            g.drawFittedText (owner.source == Source::File ? "No file loaded.\nDrop an audio file here, drag one from the browser, or press Load file..."
                                                           : "This cell has no sample.\nSelect a cell with a sample in the Main tab, or switch to File to slice an external audio file.",
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

        // Sorgente File: la parte fuori dalla zona scelta e' scurita
        if (owner.source == Source::File)
        {
            g.setColour (juce::Colour (0xff0d0e11).withAlpha (0.62f));
            const float xs = juce::jlimit (area.getX(), area.getRight(), xOf (rs)), xe = juce::jlimit (area.getX(), area.getRight(), xOf (re));
            g.fillRect (juce::Rectangle<float> (area.getX(), area.getY(), xs - area.getX(), area.getHeight()));
            g.fillRect (juce::Rectangle<float> (xe, area.getY(), area.getRight() - xe, area.getHeight()));
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
        if (owner.cursorActive())
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
        g.drawText (owner.source == Source::File ? "Drag the brackets to choose the zone - click a slice to hear it - double-click adds a marker - right-click removes"
                                                 : "Click a slice to hear it - drag markers - double-click to add - right-click to remove",
                    juce::Rectangle<float> (area.getX() + 4.0f * s, area.getBottom() + 6.0f * s, area.getWidth() - 140.0f * s, 18.0f * s),
                    juce::Justification::centredLeft);
        layoutButtons();
    }

    void resized() override { layoutButtons(); peaksDirty = true; }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int near = markerNear (e.x);
        const bool onBracket = bracketNear (e.x) >= 0;
        setMouseCursor (near >= 0 || onBracket ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
        const int h = (near >= 0 || onBracket) ? -1 : sliceAtX ((float) e.x, (float) e.y);
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

        if (const int br = bracketNear (e.x); br >= 0)        // sorgente File: le parentesi scelgono la zona
        {
            dragBracket = br;
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
        if (dragBracket >= 0)
        {
            const float p = pos ((float) e.x), minLen = 0.0005f;
            if (dragBracket == 0)
            {
                const float old = owner.extStart;
                owner.extStart = juce::jlimit (0.0f, owner.extEnd - minLen, p);
                if (! owner.markers.empty() && std::abs (owner.markers.front() - old) < 1.0e-6f)
                    owner.markers.front() = owner.extStart;                // il primo marcatore segue l'inizio della zona
            }
            else
                owner.extEnd = juce::jlimit (owner.extStart + minLen, 1.0f, p);
            owner.regionChanged();
            repaint();
            return;
        }
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
        if (dragBracket >= 0)
        {
            dragBracket = -1;
            owner.regionChanged();
            repaint();
            return;
        }
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

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (juce::ModifierKeys::currentModifiers.isCtrlDown())
        {
            setZoom (zoom * (w.deltaY > 0 ? 1.25f : 0.8f), (float) e.x);
            return;
        }
        holdUntil = juce::Time::getMillisecondCounterHiRes() + 2500.0;
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
        const float rs = owner.viewLo(), re = owner.viewHi();
        const float span = (re - rs) / zoom;
        const float a = juce::jlimit (rs, juce::jmax (rs, re - span), centre - span * 0.5f);
        return { a, a + span };
    }

    void clampCentre()
    {
        const float rs = owner.viewLo(), re = owner.viewHi();
        const float span = (re - rs) / zoom;
        centre = juce::jlimit (rs + span * 0.5f, re - span * 0.5f, centre);
    }

    // Zoom attorno a un punto: sotto il mouse (anchorX >= 0), sul cursore se sta suonando, altrimenti al centro
    void setZoom (float z, float anchorX = -1.0f)
    {
        const auto [a, b] = view();
        float anchorPos = centre, frac = 0.5f;
        if (anchorX >= 0.0f)
        {
            anchorPos = pos (anchorX);
            frac = (anchorPos - a) / juce::jmax (1.0e-9f, b - a);
        }
        else if (owner.cursorActive())
            anchorPos = (float) owner.chopPosition (juce::Time::getMillisecondCounterHiRes());
        zoom = juce::jlimit (1.0f, 64.0f, z);
        const float span = (owner.viewHi() - owner.viewLo()) / zoom;
        centre = anchorPos + (0.5f - frac) * span;
        clampCentre();
        holdUntil = juce::Time::getMillisecondCounterHiRes() + 2500.0;
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

    int bracketNear (int x) const                       // 0 = inizio, 1 = fine della zona (solo sorgente File)
    {
        if (owner.source != Source::File)
            return -1;
        const float s = theme::scaleFor (*this);
        const float d0 = std::abs (xOf (owner.regionStart()) - (float) x), d1 = std::abs (xOf (owner.regionEnd()) - (float) x);
        if (d0 <= 8.0f * s && d0 <= d1)
            return 0;
        return d1 <= 8.0f * s ? 1 : -1;
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
    int hover = -1, dragIndex = -1, dragBracket = -1;
    double holdUntil = 0.0;
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
            const bool original = i == owner.cell && owner.source == Source::Cell;
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

    // Sorgente: la cella selezionata oppure un file esterno
    for (auto* b : { &cellSrcButton, &fileSrcButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (4713);
        addAndMakeVisible (*b);
    }
    cellSrcButton.setTooltip ("Slice the sample of the cell selected in the Main tab");
    fileSrcButton.setTooltip ("Slice an external audio file: drop one here, drag it from the browser, or press Load file...");
    cellSrcButton.onClick = [this] { setSource (Source::Cell); };
    fileSrcButton.onClick = [this] { setSource (Source::File); };

    playButton.setTooltip ("Play the zone (Space). Press again, or Esc, to stop. Clicking a slice plays just that slice.");
    playButton.onClick = [this] { togglePlay(); };
    addAndMakeVisible (playButton);

    loadButton.setTooltip ("Choose an audio file to slice");
    loadButton.onClick = [this] { chooseFile(); };
    addAndMakeVisible (loadButton);

    // Tempo della sorgente: serve al Sync delle slice (stretch dal tempo originale al BPM del progetto)
    bpmSlider.setSliderStyle (juce::Slider::IncDecButtons);
    bpmSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
    bpmSlider.setRange (0.0, 300.0, 0.1);
    bpmSlider.setNumDecimalPlacesToDisplay (1);
    bpmSlider.setTooltip ("Original tempo of the source (BPM). The slices inherit it, and Sync stretches them from it to the project tempo.");
    bpmSlider.textFromValueFunction = [] (double v) { return v <= 0.0 ? juce::String ("--") : juce::String (v, 1); };
    bpmSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    bpmSlider.onValueChange = [this] { setSrcBpm ((float) bpmSlider.getValue()); };
    addAndMakeVisible (bpmSlider);

    bpmDetectButton.setTooltip ("Estimate the tempo from the regularity of the hits inside the zone (not from its length)");
    bpmDetectButton.onClick = [this] { doDetectBpm(); };
    halfButton.setTooltip ("Halve the tempo (fixes a detection that is twice too fast)");
    halfButton.onClick = [this] { scaleBpm (0.5); };
    doubleButton.setTooltip ("Double the tempo (fixes a detection that is half too slow)");
    doubleButton.onClick = [this] { scaleBpm (2.0); };
    tapButton.setTooltip ("Tap the beat while listening to set the tempo");
    tapButton.onClick = [this] { doTap(); };
    for (auto* b : { &bpmDetectButton, &halfButton, &doubleButton, &tapButton })
        addAndMakeVisible (*b);

    clickButton.setTooltip ("Play the zone with a click at the source BPM (accent every 4 beats, starting at the first marker): if the click stays on the beat, the tempo is right");
    clickButton.onClick = [this] { if (testPlaying) stopTest(); else doClickTest(); };
    addAndMakeVisible (clickButton);

    syncToggle.setToggleState (false, juce::dontSendNotification);
    syncToggle.setColour (juce::ToggleButton::textColourId, theme::text);
    syncToggle.setTooltip ("On: the new cells follow the project tempo (Sync to tempo). The source BPM is stored in each of them.");
    addAndMakeVisible (syncToggle);

    updateSourceControls();
    theme::noButtonFocus (*this);
    setCell (0);
}

SlicePanel::~SlicePanel() { processor.setChopCapture (false); }

float SlicePanel::regionStart() const
{
    if (source == Source::File)
        return juce::jlimit (0.0f, 1.0f, extStart);
    const auto& refs = processor.getParams().cell (cell);
    return juce::jlimit (0.0f, 1.0f, refs.at (sl::CellF::Start)->get());
}

float SlicePanel::regionEnd() const
{
    if (source == Source::File)
        return juce::jlimit (regionStart(), 1.0f, extEnd);
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
    if (source == Source::File)
        return targetStart >= 0 ? targetStart : cell;       // dalla cella selezionata in Main
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
    stopAll();
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), c);
    targetStart = -1;
    if (source == Source::Cell)
        markers = processor.getKit().cells[(size_t) cell].slices;
    refresh();
    if (source == Source::Cell)
        wave->reset();
}

bool SlicePanel::hasSource() const
{
    return data() != nullptr && (source == Source::File || ! processor.getKit().cells[(size_t) cell].missing);
}

juce::String SlicePanel::sourceName() const
{
    if (source == Source::File)
        return extFile.getFullPathName().isNotEmpty() ? extFile.getFileName() : juce::String ("No file loaded");
    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];
    return theme::cellId (cell, kit.cols) + (c.hasSample() ? " - " + c.name : juce::String (" - empty"));
}

void SlicePanel::clampMarkers()
{
    const float rs = regionStart(), re = regionEnd();
    std::vector<float> m;
    for (const float v : markers)
        if (v >= rs - 1.0e-6f && v < re - 1.0e-6f)
            m.push_back (v);
    if (m.empty() || m.front() > rs + 1.0e-6f)
        m.insert (m.begin(), rs);
    markers = m;
    if (source == Source::File)
        extMarkers = markers;
}

void SlicePanel::refresh()
{
    if (cell >= processor.getKit().numCells())
        cell = 0;

    clampMarkers();                                            // Start/End possono essere cambiati nella scheda Main

    const bool has = hasSource();
    for (auto* c : std::initializer_list<juce::Component*> { &detectButton, &sensitivity, &evenBox, &clearButton, &chopButton, &distributeButton,
                                                             &bpmSlider, &bpmDetectButton, &halfButton, &doubleButton, &tapButton, &clickButton, &playButton })
        c->setEnabled (has);
    keepToggle.setButtonText ("Keep original in " + theme::cellId (cell, processor.getKit().cols));
    bpmSlider.setValue ((double) srcBpm(), juce::dontSendNotification);
    wave->touch();
    cellsStrip->repaint();
    repaint();
}

void SlicePanel::regionChanged()
{
    clampMarkers();
    cellsStrip->repaint();
    repaint();
}

void SlicePanel::updateSourceControls()
{
    const bool file = source == Source::File;
    cellSrcButton.setToggleState (! file, juce::dontSendNotification);
    fileSrcButton.setToggleState (file, juce::dontSendNotification);
    loadButton.setVisible (file);
    syncToggle.setVisible (file);
    keepToggle.setVisible (! file);
}

void SlicePanel::setSource (Source s)
{
    stopAll();
    if (s != source)
    {
        if (source == Source::File)
            extMarkers = markers;
        source = s;
        targetStart = -1;
        markers = (s == Source::File) ? extMarkers : processor.getKit().cells[(size_t) cell].slices;
    }
    updateSourceControls();
    resized();
    refresh();
    wave->reset();
}

void SlicePanel::loadFile (const juce::File& f)
{
    if (! f.existsAsFile())
        return;
    auto d = processor.loadAudioFile (f);
    if (d == nullptr)
    {
        showStatus ("Could not read " + f.getFileName());
        return;
    }
    stopAll();
    extData = d;
    extFile = f;
    extStart = 0.0f;
    extEnd = 1.0f;
    extBpm = 0.0f;
    extMarkers = { 0.0f };
    targetStart = -1;
    source = Source::File;
    markers = extMarkers;
    updateSourceControls();
    resized();
    refresh();
    wave->reset();
    showStatus ("Loaded " + f.getFileName() + " - drag the brackets to choose the zone");
}

void SlicePanel::chooseFile()
{
    const auto start = extFile.existsAsFile() ? extFile.getParentDirectory()
                                              : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    chooser = std::make_unique<juce::FileChooser> ("Choose an audio file to slice", start, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    juce::Component::SafePointer<SlicePanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe != nullptr && fc.getResult().existsAsFile())
                                  safe->loadFile (fc.getResult());
                          });
}

static bool isAudioFileName (const juce::String& path)
{
    const auto ext = juce::File (path).getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".ogg" || ext == ".mp3";
}

bool SlicePanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (isAudioFileName (f))
            return true;
    return false;
}

void SlicePanel::filesDropped (const juce::StringArray& files, int, int)
{
    setDragOver (false);
    for (const auto& f : files)
        if (isAudioFileName (f))
        {
            loadFile (juce::File (f));
            return;
        }
}

void SlicePanel::itemDropped (const SourceDetails&)
{
    setDragOver (false);
    if (browserFile)
        loadFile (browserFile());
}

void SlicePanel::paintOverChildren (juce::Graphics& g)
{
    if (! dragOver)
        return;
    const float s = theme::scaleFor (*this);
    const auto r = waveRect.toFloat();
    g.setColour (theme::accent.withAlpha (0.12f));
    g.fillRoundedRectangle (r, 10.0f * s);
    g.setColour (theme::accent);
    g.drawRoundedRectangle (r.reduced (1.0f), 10.0f * s, 2.0f);
    g.setFont (juce::FontOptions (juce::jmax (12.0f, 18.0f * s), juce::Font::bold));
    g.drawText ("Drop an audio file to slice it", waveRect, juce::Justification::centred);
}

void SlicePanel::cellsStripRepaint()
{
    if (cellsStrip != nullptr)
        cellsStrip->repaint();
}

void SlicePanel::markersChanged()
{
    if (source == Source::File)
        extMarkers = markers;
    else
    {
        processor.getKit().cells[(size_t) cell].slices = markers;
        processor.markDirty();
    }
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
    if (chopping || testPlaying)
        return;
    startPreview (b[(size_t) index], b[(size_t) index + 1]);
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
    if (testPlaying)
    {
        if (now - chopStartMs >= chopDurMs + 60.0)
            stopTest (true);
        else
            wave->repaint();
    }
    if (previewing)
    {
        if (now - chopStartMs >= chopDurMs + 60.0)
            stopPreview (true);
        else
            wave->repaint();
    }
    if (cursorActive())
        wave->follow ((float) chopPosition (now));               // con lo zoom la forma d'onda segue il cursore
    if (status.isNotEmpty() && now >= statusUntil)
    {
        status.clear();
        repaint();
    }
    if (! cursorActive() && status.isEmpty())
        stopTimer();
}

// ---------------------------------------------------------------- Tempo della sorgente

float SlicePanel::srcBpm() const
{
    return source == Source::File ? extBpm : processor.getKit().cells[(size_t) cell].loopBpm;
}

void SlicePanel::setSrcBpm (float bpm)
{
    bpm = juce::jlimit (0.0f, 300.0f, bpm);
    if (source == Source::File)
        extBpm = bpm;
    else
    {
        processor.getKit().cells[(size_t) cell].loopBpm = bpm;
        processor.cellEdited (cell);
    }
    bpmSlider.setValue ((double) bpm, juce::dontSendNotification);
}

void SlicePanel::doDetectBpm()
{
    const auto d = data();
    if (d == nullptr)
        return;
    const double bpm = sl::estimateBpmFromOnsets (*d, regionStart(), regionEnd());
    if (bpm <= 0.0)
    {
        showStatus ("Not enough rhythm in this zone to estimate the tempo - use Tap or type the BPM");
        return;
    }
    setSrcBpm ((float) (std::round (bpm * 10.0) / 10.0));
    showStatus ("Estimated tempo: " + juce::String (srcBpm(), 1) + " BPM - check it with Click test");
}

void SlicePanel::doTap()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (! taps.empty() && now - taps.back() > 2000.0)
        taps.clear();
    taps.push_back (now);
    if (taps.size() > 9)
        taps.erase (taps.begin());
    if (taps.size() >= 2)
    {
        const double avg = (taps.back() - taps.front()) / (double) (taps.size() - 1);
        setSrcBpm ((float) (std::round (60000.0 / avg * 10.0) / 10.0));
    }
}

void SlicePanel::scaleBpm (double factor)
{
    const float v = srcBpm();
    if (v > 0.0f)
        setSrcBpm (juce::jlimit (20.0f, 300.0f, v * (float) factor));
}

void SlicePanel::doClickTest()
{
    const auto d = data();
    if (d == nullptr)
        return;
    const double bpm = (double) srcBpm();
    if (bpm < 20.0)
    {
        showStatus ("Set the source BPM first: Detect, Tap or type it");
        return;
    }
    stopChop();
    stopPreview();

    const float rs = regionStart(), re = regionEnd();
    cursorLo = rs;
    cursorHi = re;
    const int n = d->numSamples();
    const int a = juce::jlimit (0, n, (int) std::floor ((double) rs * (double) n)), b = juce::jlimit (a, n, (int) std::floor ((double) re * (double) n));
    const int len = b - a;
    if (len < 1024)
        return;

    // Copia della zona con un click a ogni battito, a partire dal primo marcatore; accento ogni 4 battiti
    auto out = std::make_shared<sl::SampleData>();
    out->sampleRate = d->sampleRate;
    out->audio.setSize (d->numChannels(), len);
    for (int ch = 0; ch < d->numChannels(); ++ch)
        out->audio.copyFrom (ch, 0, d->audio, ch, a, len);

    const double sr = d->sampleRate;
    const double period = 60.0 / bpm * sr;
    const double first = markers.empty() ? 0.0 : (double) (markers.front() - rs) * (double) n;
    const int clickLen = (int) (0.03 * sr);
    for (int k = 0;; ++k)
    {
        const double pos = first + (double) k * period;
        if (pos >= (double) len)
            break;
        const int p0 = (int) pos;
        const bool accent = (k % 4) == 0;
        const double freq = accent ? 2200.0 : 1500.0;
        const float amp = accent ? 0.7f : 0.45f;
        for (int i = 0; i < clickLen && p0 + i < len; ++i)
        {
            const float v = amp * (float) (std::sin (juce::MathConstants<double>::twoPi * freq * (double) i / sr) * std::exp (-(double) i / (0.006 * sr)));
            for (int ch = 0; ch < out->audio.getNumChannels(); ++ch)
                out->audio.addSample (ch, p0 + i, v);
        }
    }

    processor.auditionData (out, 0.0f, 1.0f, (float) auditionVolume.getValue());
    chopStartMs = juce::Time::getMillisecondCounterHiRes();
    chopDurMs = (double) len / sr * 1000.0;
    testPlaying = true;
    clickButton.setButtonText ("Stop test");
    clickButton.setPrimary (true);
    startTimerHz (60);
    wave->repaint();
}

void SlicePanel::stopTest (bool finished)
{
    if (! testPlaying)
        return;
    testPlaying = false;
    if (! finished)
        processor.stopAudition();
    clickButton.setButtonText ("Click test");
    clickButton.setPrimary (false);
    wave->repaint();
}

double SlicePanel::chopPosition (double nowMs) const
{
    const double t = chopDurMs > 0.0 ? juce::jlimit (0.0, 1.0, (nowMs - chopStartMs) / chopDurMs) : 0.0;
    return cursorLo + t * (cursorHi - cursorLo);
}

void SlicePanel::startPreview (float lo, float hi)
{
    const auto d = data();
    if (d == nullptr || hi <= lo)
        return;
    stopAll();
    cursorLo = lo;
    cursorHi = hi;
    chopDurMs = (double) (hi - lo) * (double) d->numSamples() / d->sampleRate * 1000.0;
    processor.auditionData (d, lo, hi, (float) auditionVolume.getValue());
    chopStartMs = juce::Time::getMillisecondCounterHiRes();
    previewing = true;
    playButton.setButtonText ("Stop");
    playButton.setPrimary (true);
    startTimerHz (60);
    wave->repaint();
}

void SlicePanel::stopPreview (bool finished)
{
    if (! previewing)
        return;
    previewing = false;
    if (! finished)
        processor.stopAudition();
    playButton.setButtonText ("Play zone");
    playButton.setPrimary (false);
    wave->repaint();
}

void SlicePanel::stopAll()
{
    stopChop();
    stopTest();
    stopPreview();
}

void SlicePanel::togglePlay()
{
    if (cursorActive())
        stopAll();
    else if (hasSource())
        startPreview (regionStart(), regionEnd());
}

void SlicePanel::startChop()
{
    stopTest();
    stopPreview();
    const auto d = data();
    if (d == nullptr || ! hasSource())
        return;
    const float rs = regionStart(), re = regionEnd();
    if (re <= rs)
        return;

    cursorLo = rs;
    cursorHi = re;
    markers = { rs };                                          // si riparte da zero: il primo marcatore e' l'inizio
    markersChanged();
    chopDurMs = (double) (re - rs) * (double) d->numSamples() / d->sampleRate * 1000.0;
    chopSeen = processor.chopNoteCount();
    processor.setChopCapture (true);
    processor.auditionData (d, rs, re, (float) auditionVolume.getValue());
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

int SlicePanel::distributeFromFile (const std::vector<float>& b, int first)
{
    const auto& kit = processor.getKit();
    const auto base = kit.file.getFullPathName().isNotEmpty() ? kit.file.getSiblingFile ("Slices")
                                                              : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Sample Lab Slices");
    const auto stem = sl::KitIO::sanitiseName (extFile.getFileNameWithoutExtension());
    auto dir = base.getChildFile (stem);
    for (int i = 2; dir.exists() && i < 1000; ++i)             // mai sovrascrivere i pezzi di una distribuzione precedente
        dir = base.getChildFile (stem + " " + juce::String (i));
    return processor.distributeSliceData (extData, extFile.getFileNameWithoutExtension(), b, first, srcBpm(), syncToggle.getToggleState(), dir);
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
        const int n = safe->source == Source::File ? safe->distributeFromFile (b, first)
                                                   : safe->processor.distributeSlices (safe->cell, b, first);
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

    auto drawCard = [&] (juce::Rectangle<int> r)
    {
        g.setColour (theme::panel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f * s);
        g.setColour (theme::border);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f * s, 1.0f);
    };
    drawCard (sourceRect);
    drawCard (toolbarRect);
    drawCard (tempoRect);
    drawCard (actionRect);
    if (! infoRect.isEmpty())
        drawCard (infoRect);

    // Titoli delle tre righe
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (11.0f, 15.0f * s), juce::Font::bold));
    g.drawText ("SOURCE", sourceTitle, juce::Justification::centredLeft);
    g.drawText ("CUT", cutTitle, juce::Justification::centredLeft);
    g.drawText ("TEMPO", tempoTitle, juce::Justification::centredLeft);

    // Nome e dettagli della sorgente
    if (! nameRect.isEmpty())
    {
        auto nr = nameRect;
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 14.0f * s), juce::Font::bold));
        g.drawText (sourceName(), nr.removeFromTop (nr.getHeight() * 55 / 100), juce::Justification::centredLeft, true);
        if (const auto d = data())
        {
            g.setColour (theme::textDim);
            g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
            g.drawText (juce::String ((double) d->numSamples() / d->sampleRate, 1) + " s  -  " + juce::String (d->sampleRate / 1000.0, 1) + " kHz  -  "
                            + (d->numChannels() > 1 ? "stereo" : "mono"),
                        nr, juce::Justification::centredLeft, true);
        }
    }

    g.setColour (theme::border);
    for (const auto& sp : seps)
        g.drawLine ((float) sp.x, (float) sp.row.getY() + 10.0f * s, (float) sp.x, (float) sp.row.getBottom() - 10.0f * s, 1.0f);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
    g.drawText ("Sensitivity", sensLabelRect, juce::Justification::centredLeft);
    g.drawText ("Source BPM", bpmLabelRect, juce::Justification::centredLeft);

    // Riga delle azioni
    const auto targets = targetCells();
    juce::String summary;
    if (data() == nullptr)
        summary = source == Source::File ? "No file loaded" : "No sample in this cell";
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
    g.drawText (status.isNotEmpty() && ! chopping ? status : summary, actionRect.reduced ((int) (18 * s), 0).withTrimmedRight ((int) (470 * s)),
                juce::Justification::centredLeft, true);

    // Scheda informazioni sul campione
    if (! infoRect.isEmpty())
    {
        auto r = infoRect.reduced ((int) (16 * s), (int) (14 * s));
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 16.0f * s), juce::Font::bold));
        g.drawText (sourceName(), r.removeFromTop ((int) (24 * s)),
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

    seps.clear();
    const int titleW = (int) (92 * s), rowH = (int) (54 * s);
    auto beginRow = [&] (juce::Rectangle<int>& card, juce::Rectangle<int>& title)
    {
        card = area.removeFromTop (rowH);
        area.removeFromTop (gap);
        auto r = card.reduced ((int) (14 * s), (int) (9 * s));
        title = r.removeFromLeft (titleW);
        seps.push_back ({ r.getX() - (int) (6 * s), card });
        return r;
    };
    auto sep = [&] (juce::Rectangle<int>& r, const juce::Rectangle<int>& card)
    {
        r.removeFromLeft ((int) (8 * s));
        seps.push_back ({ r.getX(), card });
        r.removeFromLeft ((int) (10 * s));
    };

    // SOURCE: Cell / File, Load, nome della sorgente, anteprima
    {
        auto r = beginRow (sourceRect, sourceTitle);
        auto seg = r.removeFromLeft ((int) (132 * s));
        cellSrcButton.setBounds (seg.removeFromLeft (seg.getWidth() / 2).reduced ((int) (2 * s), 0));
        fileSrcButton.setBounds (seg.reduced ((int) (2 * s), 0));
        sep (r, sourceRect);

        auto right = r.removeFromRight ((int) (230 * s));
        seps.push_back ({ right.getX() - (int) (6 * s), sourceRect });
        playButton.setBounds (r.removeFromRight ((int) (118 * s)));
        r.removeFromRight ((int) (10 * s));
        auditionToggle.setBounds (right.removeFromLeft ((int) (104 * s)).withSizeKeepingCentre ((int) (104 * s), (int) (26 * s)));
        auditionVolume.setBounds (right.reduced (0, (int) (4 * s)));

        if (source == Source::File)
        {
            loadButton.setBounds (r.removeFromLeft ((int) (136 * s)));
            r.removeFromLeft ((int) (12 * s));
        }
        else
            loadButton.setBounds ({});
        nameRect = r.withTrimmedRight ((int) (10 * s));
    }

    // CUT: come trovare i punti di taglio
    {
        auto r = beginRow (toolbarRect, cutTitle);
        detectButton.setBounds (r.removeFromLeft ((int) (104 * s)));
        r.removeFromLeft ((int) (10 * s));
        auto sens = r.removeFromLeft ((int) (146 * s));
        sensLabelRect = juce::Rectangle<int> (sens.getX() + (int) (46 * s), toolbarRect.getY() + (int) (5 * s), (int) (100 * s), (int) (13 * s));
        sensitivity.setBounds (sens.withTrimmedTop ((int) (10 * s)));
        sep (r, toolbarRect);
        evenBox.setBounds (r.removeFromLeft ((int) (128 * s)));
        sep (r, toolbarRect);
        clearButton.setBounds (r.removeFromLeft ((int) (96 * s)));
        sep (r, toolbarRect);
        chopButton.setBounds (r.removeFromLeft ((int) (112 * s)));
    }

    // TEMPO: BPM della sorgente e relativi strumenti
    {
        auto r = beginRow (tempoRect, tempoTitle);
        auto b = r.removeFromLeft ((int) (176 * s));
        bpmLabelRect = juce::Rectangle<int> (b.getX() + (int) (2 * s), tempoRect.getY() + (int) (5 * s), (int) (120 * s), (int) (13 * s));
        const auto bsl = b.withTrimmedTop ((int) (10 * s));
        bpmSlider.setBounds (bsl);
        bpmSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, (int) ((float) bsl.getWidth() * 0.5f), bsl.getHeight());
        r.removeFromLeft ((int) (8 * s));
        bpmDetectButton.setBounds (r.removeFromLeft ((int) (74 * s)));
        r.removeFromLeft ((int) (6 * s));
        halfButton.setBounds (r.removeFromLeft ((int) (42 * s)));
        r.removeFromLeft ((int) (4 * s));
        doubleButton.setBounds (r.removeFromLeft ((int) (42 * s)));
        r.removeFromLeft ((int) (6 * s));
        tapButton.setBounds (r.removeFromLeft ((int) (62 * s)));
        sep (r, tempoRect);
        clickButton.setBounds (r.removeFromLeft ((int) (124 * s)));
    }

    // Azioni in basso
    actionRect = area.removeFromBottom ((int) (62 * s));
    area.removeFromBottom (gap);
    {
        auto r = actionRect.reduced ((int) (14 * s), (int) (10 * s));
        distributeButton.setBounds (r.removeFromRight ((int) (230 * s)));
        r.removeFromRight ((int) (16 * s));
        const auto opt = r.removeFromRight ((int) (230 * s)).withSizeKeepingCentre ((int) (230 * s), (int) (26 * s));
        keepToggle.setBounds (opt);
        syncToggle.setBounds (opt);
    }

    // Celle: quattro righe
    cellsRect = area.removeFromBottom (juce::jmax ((int) (140 * s), juce::jmin ((int) (214 * s), area.getHeight() * 34 / 100)));
    area.removeFromBottom (gap);
    cellsStrip->setBounds (cellsRect);

    waveRect = area;
    wave->setBounds (waveRect);
}
