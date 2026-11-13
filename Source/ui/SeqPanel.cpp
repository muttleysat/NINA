#include "SeqPanel.h"
#include "Widgets.h"

namespace
{

// Picchi assoluti (0..1) del campione, uno per colonna: per le mini forme d'onda
std::vector<float> computePeaks (const sl::SampleData* data, int width)
{
    std::vector<float> peaks;
    if (data == nullptr || width <= 0)
        return peaks;

    const int n = data->numSamples();
    peaks.resize ((size_t) width);
    for (int x = 0; x < width; ++x)
    {
        const int a = (int) ((int64_t) x * n / width);
        const int b = juce::jmin (n, juce::jmax (a + 1, (int) ((int64_t) (x + 1) * n / width)));
        float p = 0.0f;
        for (int ch = 0; ch < data->numChannels(); ++ch)
        {
            const float* d = data->audio.getReadPointer (ch);
            for (int i = a; i < b; ++i)
                p = juce::jmax (p, std::abs (d[i]));
        }
        peaks[(size_t) x] = juce::jmin (1.0f, p);
    }
    return peaks;
}

void drawPeaks (juce::Graphics& g, const std::vector<float>& peaks, juce::Rectangle<float> area, juce::Colour colour)
{
    if (peaks.empty() || area.getWidth() < 2.0f)
        return;

    const float mid = area.getCentreY(), half = area.getHeight() * 0.5f;
    juce::Path p;
    const int w = (int) peaks.size();
    const float step = area.getWidth() / (float) w;
    p.startNewSubPath (area.getX(), mid);
    for (int x = 0; x < w; ++x)
        p.lineTo (area.getX() + (float) x * step, mid - peaks[(size_t) x] * half);
    for (int x = w - 1; x >= 0; --x)
        p.lineTo (area.getX() + (float) x * step, mid + peaks[(size_t) x] * half);
    p.closeSubPath();
    g.setColour (colour);
    g.fillPath (p);
}

// Voce del menu "+ Cell": ID, nome e forma d'onda; il campione suona mentre la voce e' evidenziata (si ascolta prima di scegliere)
class CellMenuItem final : public juce::PopupMenu::CustomComponent, private juce::Timer
{
public:
    CellMenuItem (SampleLabProcessor& p, int c) : CustomComponent (true), processor (p), cell (c)
    {
        peaks = computePeaks (processor.getCellData (cell).get(), 120);
        startTimerHz (25);
    }

    ~CellMenuItem() override
    {
        stopTimer();
        stop();
    }

    void getIdealSize (int& w, int& h) override { w = 320; h = 40; }

    void paint (juce::Graphics& g) override
    {
        const auto& kit = processor.getKit();
        const auto& c = kit.cells[(size_t) cell];
        const bool hot = isItemHighlighted();

        g.fillAll (hot ? theme::accent.withAlpha (0.25f) : juce::Colours::transparentBlack);
        auto area = getLocalBounds().reduced (10, 4);

        g.setColour (juce::Colour (c.colour));
        g.fillRoundedRectangle (area.removeFromLeft (4).toFloat(), 2.0f);
        area.removeFromLeft (8);

        auto text = area.removeFromLeft (110);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (sl::cellLabel (cell, kit.cols), text.removeFromTop (16), juce::Justification::centredLeft);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (c.name, text, juce::Justification::centredLeft, true);

        drawPeaks (g, peaks, area.toFloat().reduced (0, 2), juce::Colour (c.colour).withAlpha (hot ? 1.0f : 0.7f));

        if (hot)
        {
            g.setColour (theme::text.withAlpha (0.85f));
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText ("PLAYING", getLocalBounds().reduced (10, 2), juce::Justification::topRight);
        }
    }

private:
    void timerCallback() override
    {
        const bool hot = isItemHighlighted();
        if (hot && ! playing)
            start();
        else if (! hot && playing)
            stop();
        repaint();
    }

    void start()
    {
        const auto& c = processor.getKit().cells[(size_t) cell];
        if (! c.hasSample() || c.missing)
            return;
        note = c.lowNote;
        processor.getKeyboardState().noteOn (1, note, 0.85f);
        playing = true;
    }

    void stop()
    {
        if (playing)
            processor.getKeyboardState().noteOff (1, note, 0.0f);
        playing = false;
    }

    SampleLabProcessor& processor;
    int cell;
    int note = 0;
    bool playing = false;
    std::vector<float> peaks;
};

// Doppio clic su una manopola: toglie il blocco dallo step
struct DoubleClickHandler final : juce::MouseListener
{
    std::function<void()> onDouble;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (onDouble) onDouble(); }
};

const char* lockedParamName (sl::FxF f)
{
    switch (f)
    {
        case sl::FxF::Drive: return "Drive";
        case sl::FxF::SatMix: return "Mix";
        case sl::FxF::Bits: return "Bits";
        case sl::FxF::Downsample: return "Rate";
        case sl::FxF::LofiMix: return "Mix";
        case sl::FxF::Cutoff: return "Cutoff";
        case sl::FxF::Resonance: return "Reso";
        case sl::FxF::DelayMix: return "Send";
        case sl::FxF::ReverbMix: return "Send";
        default: return "";
    }
}
}

// ------------------------------------------------------------------------ Step

class SeqPanel::StepButton final : public juce::Component
{
public:
    StepButton (SeqPanel& o, int laneIndex) : owner (o), lane (laneIndex) {}

    int column = 0;

    struct State
    {
        bool on = false, locked = false, hasProb = false, selected = false, current = false, beat = false, beyond = false;
        int velocity = 100, ratchet = 1;
        bool operator== (const State&) const = default;
    };

    void setState (const State& s)
    {
        if (s == state)
            return;
        state = s;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto b = getLocalBounds().toFloat().reduced (1.6f * s);

        g.setColour (state.beyond ? theme::panel : (state.beat ? theme::panelHi.brighter (0.08f) : theme::panelHi));
        g.fillRoundedRectangle (b, 6.0f * s);
        if (state.beyond)
            return;                                            // oltre la lunghezza della lane

        if (state.on)
        {
            // Quadrato arancione pieno; l'altezza chiara in basso e' la velocity
            g.setColour (theme::accent.withAlpha (0.55f));
            g.fillRoundedRectangle (b, 6.0f * s);
            const float h = b.getHeight() * (0.2f + 0.8f * (float) state.velocity / 127.0f);
            g.setGradientFill (juce::ColourGradient (theme::accentSoft, 0.0f, b.getBottom() - h, theme::accent, 0.0f, b.getBottom(), false));
            g.fillRoundedRectangle (b.withTop (b.getBottom() - h), 6.0f * s);
        }
        if (state.current)
        {
            g.setColour (theme::text.withAlpha (0.95f));
            g.drawRoundedRectangle (b, 6.0f * s, 1.8f);
        }
        if (state.selected)
        {
            g.setColour (theme::text);
            g.drawRoundedRectangle (b.expanded (1.0f), 6.0f * s, 2.2f);
        }

        g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.5f * s), juce::Font::bold));
        g.setColour (juce::Colour (0xff1a1006));
        if (state.on && state.locked)
            g.fillEllipse (b.getRight() - 9.0f * s, b.getY() + 3.0f * s, 6.0f * s, 6.0f * s);
        if (state.on && state.ratchet > 1)
            g.drawText ("x" + juce::String (state.ratchet), b.reduced (3.0f * s), juce::Justification::topLeft);
        if (state.on && state.hasProb)
            g.drawText ("%", b.reduced (3.0f * s), juce::Justification::bottomRight);
    }

    // Clic: accende/seleziona (vedi SeqPanel::stepClicked). Trascinamento verticale su uno step acceso: cambia la velocity.
    // Trascinamento orizzontale col tasto premuto: "pennello". Partendo da uno step vuoto accende tutti gli step toccati
    // (anche su altre righe); partendo da uno acceso li spegne.
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragged = erasing = painting = false;
        if (state.beyond)
            return;
        pressX = e.x;
        pressY = e.y;
        pressVelocity = state.velocity;
        startedOn = state.on;
        rightOrShift = e.mods.isPopupMenu() || e.mods.isShiftDown();
        if (rightOrShift)
            owner.stepClicked (lane, column, true);
        else if (! startedOn)
        {
            owner.stepClicked (lane, column, false);          // step vuoto: si accende subito, e si puo' continuare a "dipingere"
            painting = true;
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (state.beyond || rightOrShift)
            return;

        if (painting)
        {
            owner.paintStepAt (e, true);
            return;
        }

        if (! dragged && ! erasing && std::abs (e.x - pressX) > 4 && std::abs (e.x - pressX) > std::abs (e.y - pressY))
        {
            erasing = true;
            owner.paintStepAt (e.withNewPosition (juce::Point<int> (getWidth() / 2, getHeight() / 2)), false);   // anche lo step di partenza
        }
        if (erasing)
        {
            owner.paintStepAt (e, false);
            return;
        }

        if (std::abs (e.y - pressY) > 3)
            dragged = true;
        if (dragged)
        {
            const int v = juce::jlimit (1, 127, pressVelocity + (int) std::lround ((double) (pressY - e.y) * 127.0 / juce::jmax (20, getHeight() * 2)));
            owner.stepVelocity (lane, column, v);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! state.beyond && ! dragged && ! rightOrShift && ! painting && ! erasing)
            owner.stepClicked (lane, column, false);
        dragged = erasing = painting = false;
    }

private:
    SeqPanel& owner;
    int lane;
    State state;
    int pressX = 0, pressY = 0, pressVelocity = 100;
    bool dragged = false, rightOrShift = false, startedOn = false, erasing = false, painting = false;
};

// ------------------------------------------------------------------------ Lane

class SeqPanel::LaneRow final : public juce::Component
{
public:
    LaneRow (SeqPanel& o, int laneIndex) : owner (o), lane (laneIndex)
    {
        const auto& l = owner.pat().lanes[(size_t) lane];

        for (auto* b : { &muteButton, &soloButton, &removeButton, &fxButton })
            addAndMakeVisible (*b);
        muteButton.setClickingTogglesState (true);
        soloButton.setClickingTogglesState (true);
        muteButton.setToggleState (l.mute, juce::dontSendNotification);
        soloButton.setToggleState (l.solo, juce::dontSendNotification);
        muteButton.setTooltip ("Mute this lane");
        soloButton.setTooltip ("Solo this lane");
        muteButton.onClick = [this] { owner.pat().lanes[(size_t) lane].mute = muteButton.getToggleState(); owner.edited(); };
        soloButton.onClick = [this] { owner.pat().lanes[(size_t) lane].solo = soloButton.getToggleState(); owner.edited(); };
        removeButton.setTooltip ("Remove this lane from the sequencer");
        removeButton.onClick = [this]
        {
            const int cell = owner.pat().lanes[(size_t) lane].cell;
            juce::MessageManager::callAsync ([o = &owner, cell] { o->processor.removeLaneForCell (cell); o->selLane = -1; o->refresh(); });
        };

        // FX: acceso = la riga usa gli effetti della cella (default); spento = la riga suona senza effetti per colpo
        fxButton.setClickingTogglesState (true);
        fxButton.setToggleState (! l.dry, juce::dontSendNotification);
        fxButton.setTooltip ("On: this lane uses the cell's effects. Off: it plays without the filter/saturation/LoFi and delay/reverb sends.");
        fxButton.onClick = [this] { owner.pat().lanes[(size_t) lane].dry = ! fxButton.getToggleState(); owner.edited(); };

        for (int i = 0; i < sl::kNumDivisions; ++i)
            divisionBox.addItem (sl::kDivisionNames[i], i + 1);
        divisionBox.setSelectedId (l.division + 1, juce::dontSendNotification);
        divisionBox.setTooltip ("Step resolution of this lane");
        divisionBox.onChange = [this] { owner.pat().lanes[(size_t) lane].division = divisionBox.getSelectedId() - 1; owner.edited(); };
        addAndMakeVisible (divisionBox);

        lengthSlider.setSliderStyle (juce::Slider::IncDecButtons);
        lengthSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
        lengthSlider.setRange (8, sl::kMaxSteps, 1);
        lengthSlider.setValue (l.length, juce::dontSendNotification);
        lengthSlider.setTooltip ("Number of steps of this lane (each lane can have its own length)");
        lengthSlider.onValueChange = [this] { owner.pat().lanes[(size_t) lane].length = (int) lengthSlider.getValue(); owner.edited(); owner.relayoutLanes(); };
        addAndMakeVisible (lengthSlider);

        for (int i = 0; i < sl::kMaxSteps; ++i)
        {
            auto b = std::make_unique<StepButton> (owner, lane);
            b->column = i;
            addAndMakeVisible (*b);
            steps.push_back (std::move (b));
        }
        muteButton.setButtonText ("M");
        soloButton.setButtonText ("S");
        removeButton.setButtonText ("x");
    }

    ~LaneRow() override
    {
        if (held)
            owner.processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
    }

    void update (int currentStep)
    {
        const auto& l = owner.pat().lanes[(size_t) lane];
        for (int c = 0; c < sl::kMaxSteps; ++c)
        {
            const int idx = c;
            StepButton::State st;
            st.beyond = idx >= l.length;
            if (idx < sl::kMaxSteps)
            {
                const auto& s = l.steps[(size_t) idx];
                st.on = s.on;
                st.velocity = s.velocity;
                st.locked = s.lockMask != 0;
                st.ratchet = s.ratchet;
                st.hasProb = s.probability < 100;
                st.selected = owner.selLane == lane && owner.selStep == idx;
                st.current = idx == currentStep;
                st.beat = ((idx / 4) % 2) == 0;                 // gruppi di 4 step alternati chiaro/scuro
            }
            steps[(size_t) c]->setState (st);
        }
    }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto& kit = owner.processor.getKit();
        const auto& l = owner.pat().lanes[(size_t) lane];
        if (l.cell < 0 || l.cell >= kit.numCells())
            return;

        const auto& c = kit.cells[(size_t) l.cell];

        // Mini forma d'onda dietro il nome: si riconosce il campione a colpo d'occhio
        const int w = labelRect.getWidth();
        if (w != peaksWidth || peaksCell != l.cell)
        {
            peaks = computePeaks (owner.processor.getCellData (l.cell).get(), juce::jmax (1, w));
            peaksWidth = w;
            peaksCell = l.cell;
        }
        g.setColour (theme::panelHi.withAlpha (held ? 0.95f : 0.55f));
        g.fillRoundedRectangle (labelRect.toFloat(), 6.0f * s);
        drawPeaks (g, peaks, labelRect.toFloat().reduced (0.0f, 3.0f * s), juce::Colour (c.colour).withAlpha (held ? 0.5f : 0.28f));

        g.setColour (juce::Colour (c.colour));
        g.fillRoundedRectangle (labelRect.toFloat().removeFromLeft (4.0f * s), 2.0f * s);
        g.setColour (theme::text);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.5f * s), juce::Font::bold));
        g.drawText (sl::cellLabel (l.cell, kit.cols) + "  " + (c.hasSample() ? c.name : juce::String ("(empty)")),
                    labelRect.withTrimmedLeft ((int) (9 * s)), juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        const float s = theme::scaleFor (*this);
        auto full = getLocalBounds().reduced (0, (int) (2 * s));
        const int bw = (int) (24 * s);

        // L'intestazione resta ferma a sinistra anche quando le colonne scorrono (stickyX = posizione di scroll)
        auto header = full.withX (stickyX).withWidth (headerWidth());
        muteButton.setBounds (header.removeFromLeft (bw));
        header.removeFromLeft ((int) (2 * s));
        soloButton.setBounds (header.removeFromLeft (bw));
        header.removeFromLeft ((int) (2 * s));
        fxButton.setBounds (header.removeFromLeft ((int) (28 * s)));
        header.removeFromLeft ((int) (6 * s));
        removeButton.setBounds (header.removeFromRight (bw));
        header.removeFromRight ((int) (4 * s));
        lengthSlider.setBounds (header.removeFromRight ((int) (74 * s)));
        header.removeFromRight ((int) (4 * s));
        divisionBox.setBounds (header.removeFromRight ((int) (88 * s)));
        header.removeFromRight ((int) (6 * s));
        labelRect = header;

        // Colonne di step con uno spazio ogni 4 (si leggono i beat); quelle sotto l'intestazione fissa si nascondono
        const int x0 = headerWidth() + (int) (8 * s);
        const int minX = stickyX + x0;
        for (int i = 0; i < sl::kMaxSteps; ++i)
        {
            const int x = x0 + i * stepW + (i / 4) * stepGap;
            steps[(size_t) i]->setBounds (x, full.getY(), stepW, full.getHeight());
            steps[(size_t) i]->setVisible (i < numSteps && x >= minX - 1);
        }
    }

    // Parametri di disposizione decisi dalla lista (numero di colonne, larghezza, scroll orizzontale)
    void applyLayout (int n, int w, int gap, int sticky)
    {
        numSteps = n;
        stepW = w;
        stepGap = gap;
        stickyX = sticky;
        resized();
        repaint();
    }

    int headerWidth() const { return (int) (368 * theme::scaleFor (*this)); }

    // Un clic sul nome della riga fa suonare la cella (anteprima del campione)
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! labelRect.contains (e.getPosition()))
            return;
        const auto& l = owner.pat().lanes[(size_t) lane];
        if (l.cell < 0 || l.cell >= owner.processor.getKit().numCells())
            return;
        const auto& c = owner.processor.getKit().cells[(size_t) l.cell];
        if (! c.hasSample() || c.missing)
            return;
        heldNote = c.lowNote;
        held = true;
        owner.processor.getKeyboardState().noteOn (1, heldNote, 0.85f);
        repaint (labelRect);
        if (owner.onCellSelected)
            owner.onCellSelected (l.cell);                  // il clic sul nome seleziona anche la cella
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (held)
            owner.processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
        held = false;
        repaint (labelRect);
    }

    juce::Rectangle<int> getStepColumn (int c) const { return steps[(size_t) c]->getBounds(); }
    int stepAt (int x) const
    {
        for (int i = 0; i < numSteps; ++i)
        {
            const auto b = steps[(size_t) i]->getBounds();
            if (steps[(size_t) i]->isVisible() && x >= b.getX() - stepGap / 2 && x < b.getRight() + (stepGap + 1) / 2)
                return i;
        }
        return -1;
    }

    int numSteps = 16, stepW = 40, stepGap = 7, stickyX = 0;

private:
    SeqPanel& owner;
    int lane;
    std::vector<float> peaks;
    int peaksWidth = 0, peaksCell = -2;
    bool held = false;
    int heldNote = 0;
    juce::TextButton muteButton, soloButton, removeButton, fxButton { "FX" };
    juce::ComboBox divisionBox;
    juce::Slider lengthSlider;
    std::vector<std::unique_ptr<StepButton>> steps;
    juce::Rectangle<int> labelRect;
};

class SeqPanel::LaneList final : public juce::Component
{
public:
    explicit LaneList (SeqPanel& o) : owner (o) {}

    void rebuild()
    {
        rows.clear();
        removeAllChildren();
        for (int i = 0; i < (int) owner.pat().lanes.size(); ++i)
        {
            auto r = std::make_unique<LaneRow> (owner, i);
            addAndMakeVisible (*r);
            rows.push_back (std::move (r));
        }
        resized();
    }

    int rowHeight() const { return juce::jmax (28, (int) (42 * theme::scaleFor (*this))); }
    int desiredHeight() const { return (int) rows.size() * rowHeight(); }

    void updateAll()
    {
        const double ppq = owner.processor.getSequencerPpq();
        const bool playing = owner.processor.isSequencerPlaying();
        for (int i = 0; i < (int) rows.size(); ++i)
        {
            int current = -1;
            if (playing && i < (int) owner.pat().lanes.size())
            {
                const auto& l = owner.pat().lanes[(size_t) i];
                const double stepBeats = sl::kDivisionBeats[juce::jlimit (0, sl::kNumDivisions - 1, l.division)];
                current = (int) (((long long) std::floor (ppq / stepBeats)) % juce::jmax (1, l.length));
            }
            rows[(size_t) i]->update (current);
        }
    }

    // Posizione orizzontale della colonna c (nel sistema di coordinate della lista), per il righello dei beat
    int columnX (int c) const
    {
        return headerW() + c * stepW + (c / 4) * stepGap;
    }

    // Colonne visibili = la riga piu' lunga (minimo 8). Si adattano alla larghezza; sotto la larghezza minima compare lo scroll orizzontale.
    int numColumns() const
    {
        int n = 8;
        for (const auto& l : owner.pat().lanes)
            n = juce::jmax (n, juce::jmin (sl::kMaxSteps, l.length));
        return n;
    }

    int headerW() const { return (int) (368 * theme::scaleFor (*this)) + (int) (8 * theme::scaleFor (*this)); }

    void computeLayout (int viewWidth)
    {
        const float s = theme::scaleFor (*this);
        const int n = numColumns();
        stepGap = (int) (7 * s);
        const int groups = (n - 1) / 4;
        const int avail = viewWidth - headerW() - (int) (4 * s);
        stepW = juce::jmax ((int) (16 * s), (avail - groups * stepGap) / n);
        contentW = headerW() + n * stepW + groups * stepGap + (int) (4 * s);
    }

    int desiredWidth (int viewWidth)
    {
        computeLayout (viewWidth);
        return juce::jmax (viewWidth, contentW);
    }

    void setSticky (int x)
    {
        sticky = x;
        for (auto& r : rows)
            r->applyLayout (numColumns(), stepW, stepGap, sticky);
    }

    void resized() override
    {
        const int rh = rowHeight();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            rows[i]->setBounds (0, (int) i * rh, getWidth(), rh);
            rows[i]->applyLayout (numColumns(), stepW, stepGap, sticky);
        }
    }

    bool hitStep (juce::Point<int> p, int& lane, int& step) const
    {
        lane = p.y / juce::jmax (1, rowHeight());
        if (lane < 0 || lane >= (int) rows.size())
            return false;
        step = rows[(size_t) lane]->stepAt (p.x);
        return step >= 0;
    }

    int stepWidth() const { return stepW; }
    int gapWidth() const { return stepGap; }

private:
    SeqPanel& owner;
    std::vector<std::unique_ptr<LaneRow>> rows;
    int stepW = 40, stepGap = 7, contentW = 0, sticky = 0;
};

// ------------------------------------------------------- Drag verso la DAW

// Maniglia da trascinare fuori dalla finestra: al primo movimento prepara i file (clip MIDI o audio renderizzato) in una
// cartella temporanea e avvia il trascinamento di file del sistema, come fa EZdrummer. Si lascia su una traccia di Cubase.
class SeqPanel::DragHandle final : public juce::Component, public juce::SettableTooltipClient
{
public:
    DragHandle (juce::String text, juce::String tooltip, std::function<juce::StringArray()> prepareFiles)
        : label (std::move (text)), prepare (std::move (prepareFiles))
    {
        setTooltip (tooltip);
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto b = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hover ? theme::panelHi.brighter (0.18f) : theme::panelHi);
        g.fillRoundedRectangle (b, 7.0f * s);
        g.setColour (hover ? theme::accent : theme::border);
        g.drawRoundedRectangle (b, 7.0f * s, 1.2f);

        // Impugnatura a puntini
        g.setColour (theme::accent);
        const float cx = b.getX() + 12.0f * s, cy = b.getCentreY();
        for (int r = -1; r <= 1; ++r)
            for (int c = 0; c < 2; ++c)
                g.fillEllipse (cx + (float) c * 5.0f * s - 1.5f, cy + (float) r * 5.0f * s - 1.5f, 3.0f, 3.0f);

        g.setColour (theme::text);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
        g.drawText (label, getLocalBounds().withTrimmedLeft ((int) (26 * s)), juce::Justification::centredLeft, true);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override { started = false; }
    void mouseUp (const juce::MouseEvent&) override { started = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (started || e.getDistanceFromDragStart() < 6)
            return;
        started = true;

        const auto files = prepare();
        if (! files.isEmpty())
            juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
    }

private:
    juce::String label;
    std::function<juce::StringArray()> prepare;
    bool hover = false, started = false;
};

juce::File SeqPanel::dragFolder() const
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("SampleLab_drag");
}

juce::String SeqPanel::dragBaseName() const
{
    const auto& kit = processor.getKit();
    return sl::KitIO::sanitiseName (kit.name + "_" + pat().name + "_" + juce::String (juce::roundToInt (processor.getCurrentBpm()))
                                    + "bpm_" + juce::String (bars()) + "bar");
}

// ----------------------------------------------------------- Carte effetti dello step

struct SeqPanel::FxKnob
{
    std::unique_ptr<KnobBox> box;
    sl::FxF which;
    DoubleClickHandler handler;
};

// Manopola "della cella" dentro la carta di uno step (tempo/feedback del delay, dimensione/smorzamento del riverbero):
// non e' un lock, modifica il parametro della cella (condiviso da tutti gli step) cosi' non serve cambiare scheda.
struct SeqPanel::CellKnob
{
    std::unique_ptr<KnobBox> box;
    sl::FxF which;
    std::unique_ptr<juce::SliderParameterAttachment> attachment;
};

struct SeqPanel::FxCard
{
    juce::String title;
    sl::FxB effect = sl::FxB::Sat;
    juce::ToggleButton power;
    std::vector<std::unique_ptr<FxKnob>> knobs;          // lock per step (colpo per colpo)
    std::vector<std::unique_ptr<CellKnob>> cellKnobs;    // parametri della cella, condivisi
    // Solo per il delay: Sync al tempo e valore musicale (parametri della cella)
    std::unique_ptr<juce::TextButton> syncButton;
    std::unique_ptr<juce::ComboBox> divisionBox;
    std::unique_ptr<juce::ButtonParameterAttachment> syncAttachment;
    std::unique_ptr<juce::ComboBoxParameterAttachment> divisionAttachment;
    int attachedCell = -2;
    juce::Rectangle<int> bounds;

    size_t numKnobs() const { return knobs.size() + cellKnobs.size(); }
};

// ------------------------------------------------------------------- SeqPanel

SeqPanel::SeqPanel (SampleLabProcessor& p) : processor (p)
{
    lanes = std::make_unique<LaneList> (*this);
    viewport.setViewedComponent (lanes.get(), false);
    viewport.setScrollBarsShown (true, true);
    addAndMakeVisible (viewport);

    // Il Play/Stop e' nella barra in alto dell'applicazione (e con la barra spaziatrice)
    for (juce::Button* b : std::initializer_list<juce::Button*> { &addButton, &addAllButton, &patternMenuButton, &midiButton, &wavButton,
                                                                  &stemsButton, &reseedButton, &prevPattern, &nextPattern })
        addAndMakeVisible (*b);
    addAllButton.setTooltip ("Add every cell that has a sample to the sequencer");
    addAllButton.onClick = [this] { addAllLoadedCells(); };
    patternMenuButton.setTooltip ("Pattern actions: duplicate, copy, paste, clear, rename");
    patternMenuButton.onClick = [this] { showPatternMenu(); };
    stemsButton.setTooltip ("Each lane as its own WAV file (same length, so they line up): with the cell's effects but without the master effects. "
                            "Drop them on separate tracks in your DAW.");
    wavButton.setTooltip ("The whole sequence as ONE audio file: cell effects, step locks, tempo-synced delay, tails and master effects included");
    midiButton.setTooltip ("The notes as a MIDI clip (effects are not part of MIDI: use Bounce WAV or Export lanes for those)");
    stemsButton.onClick = [this] { exportStems(); };

    // Drag to DAW: i file si generano al momento del trascinamento (cartella temporanea) con il tempo e la lunghezza correnti
    dragMidi = std::make_unique<DragHandle> ("Drag MIDI", "Drag this onto a MIDI or instrument track in your DAW: the pattern notes as a MIDI clip",
        [this] () -> juce::StringArray
        {
            const auto dir = dragFolder();
            dir.deleteRecursively();
            dir.createDirectory();
            const auto file = dir.getChildFile (dragBaseName() + ".mid");
            juce::String err;
            return processor.exportPatternMidi (file, bars(), err) ? juce::StringArray (file.getFullPathName()) : juce::StringArray();
        });
    dragAudio = std::make_unique<DragHandle> ("Drag audio", "Drag this onto an audio track: the whole sequence as one file, with all effects",
        [this] () -> juce::StringArray
        {
            const auto dir = dragFolder();
            dir.deleteRecursively();
            dir.createDirectory();
            const auto file = dir.getChildFile (dragBaseName() + ".wav");
            juce::String err;
            return processor.bouncePatternWav (file, bars(), err) ? juce::StringArray (file.getFullPathName()) : juce::StringArray();
        });
    dragLanes = std::make_unique<DragHandle> ("Drag lanes", "Drag this into the DAW: one audio file per lane (same length), each with its own effects",
        [this] () -> juce::StringArray
        {
            const auto dir = dragFolder();
            dir.deleteRecursively();
            dir.createDirectory();
            juce::String err;
            if (processor.exportPatternStems (dir, bars(), err) <= 0)
                return {};
            juce::StringArray files;
            for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.wav"))
                files.add (f.getFullPathName());
            files.sort (true);
            return files;
        });
    addAndMakeVisible (*dragMidi);
    addAndMakeVisible (*dragAudio);
    addAndMakeVisible (*dragLanes);

    addButton.onClick = [this] { showAddMenu(); };
    midiButton.onClick = [this] { exportMidi(); };
    wavButton.onClick = [this] { bounceWav(); };
    reseedButton.setTooltip ("New random seed: probability and ratchet choices change but stay repeatable");
    reseedButton.onClick = [this] { pat().seed = (std::uint32_t) juce::Random::getSystemRandom().nextInt(); edited(); };

    for (int i = 0; i < sl::kNumPatterns; ++i)
        patternBox.addItem (processor.getKit().patterns[(size_t) i].name, i + 1);
    patternBox.onChange = [this]
    {
        processor.setActivePattern (patternBox.getSelectedId() - 1);
        selLane = selStep = -1;
        refresh();
    };
    addAndMakeVisible (patternBox);
    prevPattern.onClick = [this] { patternBox.setSelectedId (juce::jmax (1, patternBox.getSelectedId() - 1)); };
    nextPattern.onClick = [this] { patternBox.setSelectedId (juce::jmin (sl::kNumPatterns, patternBox.getSelectedId() + 1)); };

    tailBox.addItem ("Tail: cut", 1);
    tailBox.addItem ("Tail: loop", 2);
    tailBox.addItem ("Tail: keep", 3);
    tailBox.setSelectedId (juce::jlimit (0, 2, (int) processor.getExportTail()) + 1, juce::dontSendNotification);
    tailBox.setTooltip ("What to do with delay/reverb tails in audio exports (Bounce WAV, Export lanes, Drag audio/lanes).\n"
                        "Cut: the file is exactly the chosen number of bars, so it snaps to the grid in your DAW; tails past the end are cut.\n"
                        "Loop: exactly the chosen bars too, but the tail of the end wraps around to the start, so a looped clip sounds seamless.\n"
                        "Keep: the full tail after the end (the file is longer than the bars).");
    tailBox.onChange = [this] { processor.setExportTail ((SampleLabProcessor::ExportTail) (tailBox.getSelectedId() - 1)); };
    addAndMakeVisible (tailBox);

    for (const char* t : { "Export 1 bar", "Export 2 bars", "Export 4 bars", "Export 8 bars" })
        barsBox.addItem (t, barsBox.getNumItems() + 1);
    barsBox.setSelectedId (2, juce::dontSendNotification);
    barsBox.setTooltip ("How many bars the exports contain (Export MIDI, Bounce WAV, Export lanes and the Drag handles). It does not change playback: the pattern simply repeats for that many bars.");
    addAndMakeVisible (barsBox);

    swingSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    swingSlider.setRange (0.0, 50.0, 1.0);
    swingSlider.setTextValueSuffix (" %");
    swingSlider.setNumDecimalPlacesToDisplay (0);
    swingSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 24);
    swingSlider.setTooltip ("Swing: delays the odd steps by a fraction of the step (0-50%)");
    swingSlider.onValueChange = [this] { pat().swing = (float) swingSlider.getValue() / 100.0f; edited(); };
    addAndMakeVisible (swingSlider);

    // ---- Editor dello step: manopole ----
    auto knob = [this] (KnobBox& k, double lo, double hi, const juce::String& suffix, std::function<void (sl::Step&, float)> apply)
    {
        k.slider.setRange (lo, hi, 1.0);
        k.slider.setTextValueSuffix (suffix);
        k.slider.setNumDecimalPlacesToDisplay (0);
        k.slider.onValueChange = [this, apply, &k]
        {
            if (updatingEditor) return;
            if (auto* s = selectedStep())
            {
                apply (*s, (float) k.slider.getValue());
                edited();
                lanes->updateAll();
            }
        };
        addAndMakeVisible (k);
    };
    knob (velocityKnob, 1, 127, "", [] (sl::Step& s, float v) { s.velocity = (std::uint8_t) v; });
    knob (probKnob, 0, 100, " %", [] (sl::Step& s, float v) { s.probability = (std::uint8_t) v; });
    knob (nudgeKnob, -50, 50, " %", [] (sl::Step& s, float v) { s.nudge = v / 100.0f; });
    knob (gateKnob, 5, 100, " %", [] (sl::Step& s, float v) { s.gate = v / 100.0f; });
    knob (pitchKnob, -24, 24, " st", [] (sl::Step& s, float v) { s.pitch = (std::int8_t) v; });
    knob (ratchetKnob, 1, 4, "x", [] (sl::Step& s, float v) { s.ratchet = (std::uint8_t) v; });

    reverseToggle.setTooltip ("Play the cell backwards on this step (combines with the cell's own Reverse)");
    reverseToggle.onClick = [this]
    {
        if (updatingEditor) return;
        if (auto* s = selectedStep()) { s->reverse = reverseToggle.getToggleState(); edited(); }
    };
    addAndMakeVisible (reverseToggle);

    stepModeBox.addItem ("As cell", 1);
    stepModeBox.addItem ("One-shot", 2);
    stepModeBox.addItem ("Gate", 3);
    stepModeBox.addItem ("Loop", 4);
    stepModeBox.setTooltip ("Trigger mode for this step: the cell's own mode, or one-shot / gate / loop");
    stepModeBox.onChange = [this]
    {
        if (updatingEditor) return;
        if (auto* s = selectedStep()) { s->mode = (std::int8_t) (stepModeBox.getSelectedId() - 2); edited(); }
    };
    addAndMakeVisible (stepModeBox);

    clearLocksButton.setTooltip ("Remove every effect lock from this step (the effects go back to the cell's values)");
    clearLocksButton.onClick = [this]
    {
        if (auto* s = selectedStep()) { s->lockMask = 0; edited(); refreshEditor(); }
    };
    addAndMakeVisible (clearLocksButton);

    // ---- Effetti per colpo: saturazione, LoFi, filtro, delay e riverbero (mandate) ----
    auto addCard = [this] (const char* title, sl::FxB effect, std::initializer_list<std::pair<const char*, sl::FxF>> knobList,
                           std::initializer_list<std::pair<const char*, sl::FxF>> cellKnobList = {})
    {
        auto card = std::make_unique<FxCard>();
        card->title = title;
        card->effect = effect;
        card->power.setTooltip ("Effect on/off for this step (grey = follows the cell). Moving a knob switches it on for the step.");
        auto* raw = card.get();
        card->power.onClick = [this, raw] { onFxPower (*raw); };
        addAndMakeVisible (card->power);

        for (const auto& [name, f] : knobList)
        {
            auto k = std::make_unique<FxKnob>();
            k->box = std::make_unique<KnobBox> (name);
            k->which = f;

            const auto* param = processor.getParams().masterFx().at (f);     // stesso intervallo per ogni cella
            const auto range = param->getNormalisableRange();
            auto& s = k->box->slider;
            s.setRange ((double) range.start, (double) range.end, (double) range.interval);
            s.setSkewFactor ((double) range.skew);
            const juce::AudioProcessorParameter* paramBase = param;          // getText e' pubblico nella classe base
            s.textFromValueFunction = [param, paramBase] (double v) { return paramBase->getText (param->convertTo0to1 ((float) v), 0); };
            s.valueFromTextFunction = [] (const juce::String& t) { return (double) t.getFloatValue(); };
            s.setDoubleClickReturnValue (false, 0.0);                        // il doppio clic toglie il blocco (vedi sotto)

            const size_t index = card->knobs.size();
            s.onValueChange = [this, raw, index] { if (! updatingEditor) onFxKnob (*raw, index); };
            k->handler.onDouble = [this, raw, index] { clearKnobLock (*raw, index); };
            s.addMouseListener (&k->handler, false);

            addAndMakeVisible (*k->box);
            card->knobs.push_back (std::move (k));
        }

        // Parametri della cella mostrati nella stessa carta (collegati ai parametri veri, agganciati in refreshEditor)
        for (const auto& [name, f] : cellKnobList)
        {
            auto ck = std::make_unique<CellKnob>();
            ck->box = std::make_unique<KnobBox> (name);
            ck->which = f;
            ck->box->slider.getProperties().set ("dim", true);            // grigio: non e' un lock
            ck->box->slider.setTooltip ("Cell setting shared by all steps (same as in the FX tab)");

            // Intervallo e testo corretti anche prima dell'aggancio al parametro (nessuno step selezionato): stessi del master
            {
                const auto* param = processor.getParams().masterFx().at (f);
                const auto range = param->getNormalisableRange();
                auto& s = ck->box->slider;
                s.setRange ((double) range.start, (double) range.end, (double) range.interval);
                s.setSkewFactor ((double) range.skew);
                const juce::AudioProcessorParameter* paramBase = param;
                s.textFromValueFunction = [param, paramBase] (double v) { return paramBase->getText (param->convertTo0to1 ((float) v), 0); };
                s.setValue ((double) param->get(), juce::dontSendNotification);
                s.setEnabled (false);
            }
            addAndMakeVisible (*ck->box);
            card->cellKnobs.push_back (std::move (ck));
        }
        fxCards.push_back (std::move (card));
    };
    using F = sl::FxF;
    addCard ("Saturation", sl::FxB::Sat, { { "Drive", F::Drive }, { "Mix", F::SatMix } });
    addCard ("LoFi", sl::FxB::Lofi, { { "Bits", F::Bits }, { "Rate", F::Downsample }, { "Mix", F::LofiMix } });
    addCard ("Filter", sl::FxB::Filter, { { "Cutoff", F::Cutoff }, { "Reso", F::Resonance } });
    addCard ("Delay", sl::FxB::Delay, { { "Send", F::DelayMix } }, { { "Time", F::DelayTime }, { "Feedback", F::DelayFeedback } });
    addCard ("Reverb", sl::FxB::Reverb, { { "Send", F::ReverbMix } }, { { "Size", F::ReverbSize }, { "Damp", F::ReverbDamp } });

    // Sync del delay al tempo del progetto (DAW o BPM interno): valore musicale 1/4, 1/8, 1/16T...
    for (auto& card : fxCards)
        if (card->effect == sl::FxB::Delay)
        {
            card->syncButton = std::make_unique<juce::TextButton> ("Sync");
            card->syncButton->setClickingTogglesState (true);          // senza questo il clic non cambia lo stato e il parametro non si muove
            card->syncButton->setTooltip ("Lock the delay time to the project tempo (host or internal BPM)");
            card->divisionBox = std::make_unique<juce::ComboBox>();
            card->divisionBox->setTooltip ("Note value of the delay when Sync is on");
            addAndMakeVisible (*card->syncButton);
            addAndMakeVisible (*card->divisionBox);
        }

    refresh();
    startTimerHz (30);
}

SeqPanel::~SeqPanel()
{
    stopTimer();
    viewport.setViewedComponent (nullptr, false);
}

void SeqPanel::showMessage (const juce::String& text)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Sample Lab", text);
}

void SeqPanel::edited()
{
    processor.patternEdited();
}

sl::Step* SeqPanel::selectedStep()
{
    if (selLane < 0 || selLane >= (int) pat().lanes.size() || selStep < 0 || selStep >= sl::kMaxSteps)
        return nullptr;
    return &pat().lanes[(size_t) selLane].steps[(size_t) selStep];
}

int SeqPanel::selectedCell() const
{
    return selLane >= 0 && selLane < (int) pat().lanes.size() ? pat().lanes[(size_t) selLane].cell : -1;
}

float SeqPanel::cellValue (int cell, sl::FxF f) const
{
    return cell >= 0 ? processor.getParams().cell (cell).fx.at (f)->get() : processor.getParams().masterFx().at (f)->get();
}

// Effetto attivo per lo step: lock esplicito acceso/spento, altrimenti acceso se un suo valore e' bloccato, altrimenti come la cella
bool SeqPanel::effectOn (const sl::Step& s, sl::FxB b, int cell) const
{
    const int sw = sl::lockIndex (b);
    if (s.hasLock (sw))
        return s.lockValues[(size_t) sw] >= 0.5f;
    for (int f = 0; f < (int) sl::FxF::Count; ++f)
        if (sl::fxOwner ((sl::FxF) f) == b && sl::isPerHitLockable ((sl::FxF) f) && s.hasLock (f))
            return true;
    return cell >= 0 ? processor.getParams().cell (cell).fx.power (b)->get() : false;
}

void SeqPanel::onFxKnob (FxCard& card, size_t knob)
{
    auto* s = selectedStep();
    if (s == nullptr)
        return;

    const auto f = card.knobs[knob]->which;
    s->setLock (f, (float) card.knobs[knob]->box->slider.getValue());     // muovere la manopola blocca il valore su questo step

    const int sw = sl::lockIndex (card.effect);
    if (s->hasLock (sw) && s->lockValues[(size_t) sw] < 0.5f)             // se l'effetto era forzato spento, il valore lo riaccende
        s->clearLock (sw);

    edited();
    refreshEditor();
}

void SeqPanel::clearKnobLock (FxCard& card, size_t knob)
{
    if (auto* s = selectedStep())
    {
        s->clearLock (card.knobs[knob]->which);
        edited();
        refreshEditor();
    }
}

void SeqPanel::onFxPower (FxCard& card)
{
    auto* s = selectedStep();
    if (s == nullptr || updatingEditor)
        return;

    const int cell = selectedCell();
    const int sw = sl::lockIndex (card.effect);
    const bool desired = card.power.getToggleState();
    const bool cellOn = cell >= 0 ? processor.getParams().cell (cell).fx.power (card.effect)->get() : false;
    bool valueLocks = false;
    for (int f = 0; f < (int) sl::FxF::Count; ++f)
        if (sl::fxOwner ((sl::FxF) f) == card.effect && sl::isPerHitLockable ((sl::FxF) f) && s->hasLock (f))
            valueLocks = true;

    if (desired)
    {
        if (cellOn) s->clearLock (sw);                    // acceso come la cella: segue la cella
        else s->setLock (sw, 1.0f);
    }
    else
    {
        if (cellOn || valueLocks) s->setLock (sw, 0.0f);  // spento per questo step
        else s->clearLock (sw);
    }

    edited();
    refreshEditor();
}

void SeqPanel::refresh()
{
    patternBox.clear (juce::dontSendNotification);
    for (int i = 0; i < sl::kNumPatterns; ++i)
        patternBox.addItem (processor.getKit().patterns[(size_t) i].name, i + 1);
    patternBox.setSelectedId (processor.getActivePatternIndex() + 1, juce::dontSendNotification);
    swingSlider.setValue (pat().swing * 100.0, juce::dontSendNotification);

    if (selLane >= (int) pat().lanes.size())
        selLane = selStep = -1;

    lanes->rebuild();
    relayoutLanes();
    refreshEditor();
    lanes->updateAll();
    resized();
    theme::noButtonFocus (*this);       // i pulsanti delle nuove righe non rubano la barra spaziatrice
}

void SeqPanel::relayoutLanes()
{
    lanes->setSize (lanes->desiredWidth (juce::jmax (1, viewport.getMaximumVisibleWidth())), juce::jmax (lanes->desiredHeight(), 1));
    lanes->resized();
    repaint (rulerRect);
}

void SeqPanel::stepClicked (int lane, int step, bool selectOnly)
{
    if (lane < 0 || lane >= (int) pat().lanes.size() || step < 0 || step >= sl::kMaxSteps)
        return;

    // Step vuoto: si accende e si seleziona. Step acceso non selezionato: si seleziona (per modificarlo senza spegnerlo).
    // Step acceso e gia' selezionato: un altro clic lo spegne. Tasto destro o Maiusc+clic: solo selezione.
    auto& st = pat().lanes[(size_t) lane].steps[(size_t) step];
    const bool wasSelected = selLane == lane && selStep == step;
    if (! selectOnly)
    {
        if (! st.on)
        {
            st.on = true;
            edited();
        }
        else if (wasSelected)
        {
            st.on = false;
            edited();
        }
    }
    selectStep (lane, step);
}

void SeqPanel::paintStepAt (const juce::MouseEvent& e, bool on)
{
    int lane = 0, step = 0;
    if (! lanes->hitStep (e.getEventRelativeTo (lanes.get()).getPosition(), lane, step)
        || lane >= (int) pat().lanes.size() || step >= pat().lanes[(size_t) lane].length)
        return;
    auto& st = pat().lanes[(size_t) lane].steps[(size_t) step];
    if (st.on != on)
    {
        st.on = on;
        edited();
        lanes->updateAll();
    }
}

void SeqPanel::stepVelocity (int lane, int step, int velocity)
{
    if (lane < 0 || lane >= (int) pat().lanes.size() || step < 0 || step >= sl::kMaxSteps)
        return;

    pat().lanes[(size_t) lane].steps[(size_t) step].velocity = (std::uint8_t) juce::jlimit (1, 127, velocity);
    edited();
    if (selLane == lane && selStep == step)
        refreshEditor();                // la manopola Velocity dell'editor segue il trascinamento
    lanes->updateAll();
}

void SeqPanel::selectStep (int lane, int step)
{
    selLane = lane;
    selStep = step;
    refreshEditor();
    lanes->updateAll();

    if (onCellSelected && lane >= 0 && lane < (int) pat().lanes.size())
        onCellSelected (pat().lanes[(size_t) lane].cell);
}

// Rilegge lo step selezionato nei controlli. Le manopole degli effetti mostrano il valore bloccato (arancione) oppure
// quello della cella (grigio).
void SeqPanel::refreshEditor()
{
    auto* s = selectedStep();
    const bool valid = s != nullptr;
    const int cell = selectedCell();

    updatingEditor = true;
    for (juce::Component* c : { (juce::Component*) &velocityKnob, (juce::Component*) &probKnob, (juce::Component*) &nudgeKnob,
                                (juce::Component*) &gateKnob, (juce::Component*) &pitchKnob, (juce::Component*) &ratchetKnob,
                                (juce::Component*) &reverseToggle, (juce::Component*) &stepModeBox, (juce::Component*) &clearLocksButton })
        c->setEnabled (valid);

    if (valid)
    {
        velocityKnob.slider.setValue (s->velocity, juce::dontSendNotification);
        probKnob.slider.setValue (s->probability, juce::dontSendNotification);
        nudgeKnob.slider.setValue (s->nudge * 100.0f, juce::dontSendNotification);
        gateKnob.slider.setValue (s->gate * 100.0f, juce::dontSendNotification);
        pitchKnob.slider.setValue (s->pitch, juce::dontSendNotification);
        ratchetKnob.slider.setValue (s->ratchet, juce::dontSendNotification);
        reverseToggle.setToggleState (s->reverse, juce::dontSendNotification);
        stepModeBox.setSelectedId (s->mode + 2, juce::dontSendNotification);
        clearLocksButton.setEnabled (s->lockMask != 0);
    }

    for (auto& card : fxCards)
    {
        card->power.setEnabled (valid);
        card->power.setToggleState (valid && effectOn (*s, card->effect, cell), juce::dontSendNotification);
        for (auto& k : card->knobs)
        {
            const bool locked = valid && s->hasLock (k->which);
            const float value = locked ? s->lockValues[(size_t) k->which] : cellValue (cell, k->which);
            k->box->slider.setValue (value, juce::dontSendNotification);
            k->box->slider.getProperties().set ("dim", ! locked);        // grigio = segue la cella
            k->box->slider.setEnabled (valid);
            k->box->slider.setTooltip (locked ? "Locked on this step. Double-click to remove the lock."
                                              : "Follows the cell. Move it to lock a value on this step.");
            k->box->slider.repaint();
        }

        // Parametri della cella: agganciati ai parametri veri della cella dello step selezionato (seguono anche l'automazione)
        for (auto& ck : card->cellKnobs)
        {
            if (card->attachedCell != cell)
            {
                ck->attachment.reset();
                if (cell >= 0)
                    ck->attachment = std::make_unique<juce::SliderParameterAttachment> (*processor.getParams().cell (cell).fx.at (ck->which),
                                                                                         ck->box->slider);
            }
            ck->box->slider.setEnabled (cell >= 0);
        }
        // Sync e valore musicale del delay agganciati ai parametri della cella
        if (card->syncButton != nullptr && card->attachedCell != cell)
        {
            card->syncAttachment.reset();
            card->divisionAttachment.reset();
            if (cell >= 0)
            {
                const auto& refs = processor.getParams().cell (cell).fx;
                card->syncAttachment = std::make_unique<juce::ButtonParameterAttachment> (*refs.delaySync, *card->syncButton);
                card->divisionBox->clear (juce::dontSendNotification);
                card->divisionBox->addItemList (refs.delayDivision->choices, 1);
                card->divisionAttachment = std::make_unique<juce::ComboBoxParameterAttachment> (*refs.delayDivision, *card->divisionBox);
            }
            card->syncButton->setEnabled (cell >= 0);
        }
        card->attachedCell = cell;

        // Con il sync acceso conta il valore musicale e il tempo libero (ms) si disattiva; senza sync e' il contrario
        if (cell >= 0 && card->effect == sl::FxB::Delay)
        {
            const bool sync = processor.getParams().cell (cell).fx.delaySync->get();
            card->divisionBox->setEnabled (sync);
            card->divisionBox->setAlpha (sync ? 1.0f : 0.5f);
            for (auto& ck : card->cellKnobs)
                if (ck->which == sl::FxF::DelayTime)
                {
                    ck->box->slider.setEnabled (! sync);
                    ck->box->slider.setTooltip (sync ? "Delay is synced to the tempo: choose the note value with the menu above"
                                                     : "Cell setting shared by all steps (same as in the FX tab)");
                }
        }
    }
    updatingEditor = false;
    repaint (headerRect);
}

void SeqPanel::addAllLoadedCells()
{
    for (int i = 0; i < processor.getKit().numCells(); ++i)
        if (processor.getKit().cells[(size_t) i].hasSample())
            processor.addLaneForCell (i);
    refresh();
}

void SeqPanel::showPatternMenu()
{
    const int cur = processor.getActivePatternIndex();
    juce::PopupMenu m;
    m.addItem (1, "Duplicate to the next empty pattern", ! pat().lanes.empty());
    m.addSeparator();
    m.addItem (2, "Copy pattern", ! pat().lanes.empty());
    m.addItem (3, "Paste into this pattern", processor.hasPatternClipboard());
    m.addItem (4, "Clear pattern", ! pat().lanes.empty());
    m.addSeparator();
    m.addItem (5, "Rename...");

    juce::Component::SafePointer<SeqPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (patternMenuButton), [safe, cur] (int r)
    {
        if (safe == nullptr || r == 0)
            return;
        auto& p = safe->processor;

        if (r == 1)
        {
            if (p.duplicateActivePattern())
            {
                safe->selLane = safe->selStep = -1;
                safe->refresh();
            }
            else
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Sample Lab",
                                                        "There is no empty pattern left to duplicate into.\nClear a pattern first, or use Copy and Paste.");
        }
        else if (r == 2)
            p.copyActivePattern();
        else if (r == 3)
        {
            auto doPaste = [safe]
            {
                if (safe == nullptr) return;
                safe->processor.pasteIntoActivePattern();
                safe->selLane = safe->selStep = -1;
                safe->refresh();
            };
            if (safe->pat().lanes.empty())
                doPaste();
            else
                juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                                  .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                                  .withTitle ("Paste pattern")
                                                  .withMessage ("Replace the contents of this pattern with the copied one?")
                                                  .withButton ("Replace")
                                                  .withButton ("Cancel")
                                                  .withAssociatedComponent (safe.getComponent()),
                                              [doPaste] (int res) { if (res == 1) doPaste(); });
        }
        else if (r == 4)
        {
            juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                              .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                              .withTitle ("Clear pattern")
                                              .withMessage ("Remove all rows and steps from this pattern?")
                                              .withButton ("Clear")
                                              .withButton ("Cancel")
                                              .withAssociatedComponent (safe.getComponent()),
                                          [safe] (int res)
                                          {
                                              if (res != 1 || safe == nullptr) return;
                                              safe->processor.clearActivePattern();
                                              safe->selLane = safe->selStep = -1;
                                              safe->refresh();
                                          });
        }
        else if (r == 5)
        {
            auto* w = new juce::AlertWindow ("Rename pattern", "Pattern name:", juce::MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", p.getKit().patterns[(size_t) cur].name);
            w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, cur] (int res)
            {
                if (res == 1 && safe != nullptr)
                {
                    safe->processor.renamePattern (cur, w->getTextEditorContents ("name"));
                    safe->refresh();
                }
            }), true);
        }
    });
}

void SeqPanel::showAddMenu()
{
    const auto& kit = processor.getKit();
    juce::PopupMenu m;
    int count = 0;
    for (int i = 0; i < kit.numCells(); ++i)
    {
        const auto& c = kit.cells[(size_t) i];
        if (! c.hasSample() || pat().findLane (i) != nullptr)
            continue;
        // Voce con forma d'onda: passando il mouse il campione suona, cosi' si sceglie ascoltando
        m.addCustomItem (i + 1, std::make_unique<CellMenuItem> (processor, i), nullptr, sl::cellLabel (i, kit.cols) + "  " + c.name);
        ++count;
    }
    if (count == 0)
        m.addItem (-1, "No cells to add (load a sample first)", false);
    else
    {
        m.addSeparator();
        m.addItem (1000, "Add all loaded cells");
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addButton), [this] (int r)
    {
        if (r == 1000)
        {
            for (int i = 0; i < processor.getKit().numCells(); ++i)
                if (processor.getKit().cells[(size_t) i].hasSample())
                    processor.addLaneForCell (i);
            refresh();
        }
        else if (r > 0)
        {
            processor.addLaneForCell (r - 1);
            refresh();
        }
    });
}

juce::File SeqPanel::defaultExportFile (const juce::String& extension) const
{
    const auto& kit = processor.getKit();
    const auto dir = kit.file.getFullPathName().isNotEmpty() ? kit.file.getSiblingFile ("Bounces")
                                                             : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    dir.createDirectory();
    const auto name = sl::KitIO::sanitiseName (kit.name + "_" + pat().name + "_" + juce::String (juce::roundToInt (processor.getCurrentBpm()))
                                               + "bpm_" + juce::String (bars()) + "bar");
    return dir.getChildFile (name + extension);
}

void SeqPanel::exportMidi()
{
    chooser = std::make_unique<juce::FileChooser> ("Export pattern as MIDI", defaultExportFile (".mid"), "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              if (! f.hasFileExtension ("mid"))
                                  f = f.withFileExtension ("mid");
                              juce::String err;
                              if (processor.exportPatternMidi (f, bars(), err))
                                  showMessage ("MIDI exported:\n" + f.getFullPathName()
                                               + "\n\nNote: reverse and effect locks are not part of MIDI. Use Bounce WAV for those.");
                              else
                                  showMessage (err);
                          });
}

void SeqPanel::bounceWav()
{
    chooser = std::make_unique<juce::FileChooser> ("Bounce pattern to audio", defaultExportFile (".wav"), "*.wav");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              if (! f.hasFileExtension ("wav"))
                                  f = f.withFileExtension ("wav");
                              juce::String err;
                              if (processor.bouncePatternWav (f, bars(), err))
                                  showMessage ("Audio bounced:\n" + f.getFullPathName());
                              else
                                  showMessage (err.isEmpty() ? juce::String ("Bounce failed") : err);
                          });
}

void SeqPanel::exportStems()
{
    if (pat().lanes.empty())
    {
        showMessage ("There are no lanes in the sequencer yet: add cells first.");
        return;
    }

    const auto start = defaultExportFile ("").getParentDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Choose where to create the folder with one file per lane", start, "*", true);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto parent = fc.getResult();
                              if (parent == juce::File())
                                  return;

                              const auto folderName = sl::KitIO::sanitiseName (processor.getKit().name + "_" + pat().name + "_"
                                                                              + juce::String (juce::roundToInt (processor.getCurrentBpm())) + "bpm_"
                                                                              + juce::String (bars()) + "bar_lanes");
                              const auto folder = parent.getChildFile (folderName);
                              juce::String err;
                              const int n = processor.exportPatternStems (folder, bars(), err);
                              if (n >= 0)
                                  showMessage (juce::String (n) + " lane file(s) written to:\n" + folder.getFullPathName()
                                               + "\n\nAll files have the same length: place them at the start of the same bar in your DAW. "
                                                 "They include each cell's effects and the delay synced to " + juce::String (processor.getCurrentBpm(), 1)
                                               + " BPM, but not the master effects.");
                              else
                                  showMessage (err.isEmpty() ? juce::String ("Export failed") : err);
                          });
}

void SeqPanel::timerCallback()
{
    lanes->updateAll();
    if (viewport.getViewPositionX() != lastViewX)
    {
        lastViewX = viewport.getViewPositionX();
        lanes->setSticky (lastViewX);
        repaint (rulerRect.expanded (0, 2));
    }

    // I valori "grigi" delle manopole seguono la cella (per esempio quando la si regola nella scheda FX)
    if (++tick % 15 == 0 && selectedStep() != nullptr)
    {
        bool dragging = false;
        for (auto& c : fxCards)
            for (auto& k : c->knobs)
                dragging = dragging || k->box->slider.isMouseButtonDown();
        if (! dragging)
            refreshEditor();
    }
}

void SeqPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);

    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f * s, 1.0f);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
    g.drawText ("DRAG TO DAW", dragRow.withWidth ((int) (104 * s)).withTrimmedLeft ((int) (6 * s)), juce::Justification::centredLeft);

    // Separatori sottili tra i gruppi di comandi
    g.setColour (theme::border.withAlpha (0.8f));
    for (const auto& [x, row] : separatorsX)
    {
        const int top = (row == 1 ? row2Rect : row1Rect).getY();
        const int bottom = (row == 0 ? row1Rect : row2Rect).getBottom();
        g.drawLine ((float) x, (float) top + 6.0f * s, (float) x, (float) bottom - 6.0f * s, 1.0f);
    }

    // Swing: etichetta sopra la manopola
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
    g.drawText ("Swing", swingRect.withHeight ((int) (16 * s)), juce::Justification::centred);

    // Righello dei beat: numero dello step ogni 4, allineato alle colonne
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
    {
        const juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (juce::Rectangle<int> (viewport.getX() + lanes->headerW(), rulerRect.getY(),
                                                  viewport.getMaximumVisibleWidth() - lanes->headerW(), rulerRect.getHeight()));
        const int n = lanes->numColumns();
        for (int c = 0; c < n; c += 4)
            g.drawText (juce::String (c + 1),
                        juce::Rectangle<int> (viewport.getX() + lanes->columnX (c) - viewport.getViewPositionX(), rulerRect.getY(),
                                              lanes->stepWidth() * 4, rulerRect.getHeight()),
                        juce::Justification::centredLeft);
    }

    if (pat().lanes.empty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * s)));
        g.drawFittedText ("No cells in the sequencer yet.\nRight-click a cell in the grid and choose \"Add to sequencer\", or use \"+ Cell\".",
                          viewport.getBounds().reduced (20), juce::Justification::centred, 3);
    }

    // Editor dello step
    g.setColour (theme::panelHi.withAlpha (0.45f));
    g.fillRoundedRectangle (editorRect.toFloat(), 10.0f * s);

    // Intestazione: etichetta "STEP EDIT" + titolo
    auto head = headerRect;
    auto pill = head.removeFromLeft ((int) (104 * s));
    g.setColour (theme::accent);
    g.fillRoundedRectangle (pill.toFloat(), 6.0f * s);
    g.setColour (juce::Colour (0xff1a1006));
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
    g.drawText ("STEP EDIT", pill, juce::Justification::centred);

    juce::String title = "Click a step to turn it on and edit it  -  drag a lit step up/down for velocity  -  click it again to turn it off";
    if (selLane >= 0 && selLane < (int) pat().lanes.size())
    {
        const auto& step = pat().lanes[(size_t) selLane].steps[(size_t) selStep];
        int locks = 0;
        for (int i = 0; i < sl::kNumLockable; ++i)
            locks += step.hasLock (i) ? 1 : 0;

        title = "Editing step " + juce::String (selStep + 1) + " of " + sl::cellLabel (pat().lanes[(size_t) selLane].cell, processor.getKit().cols)
                + (step.on ? "" : "   (step is off: click it to turn it on)")
                + (locks > 0 ? "   -   " + juce::String (locks) + " lock(s)" : juce::String());
    }
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.5f * s), juce::Font::bold));
    g.drawText (title, head.withTrimmedLeft ((int) (12 * s)), juce::Justification::centredLeft, true);

    // Carte degli effetti per colpo
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s), juce::Font::bold));
    g.drawText ("EFFECTS (STEP)  -  orange = locked on this step, grey = follows the cell. Delay time/feedback and reverb size/damp are cell settings shared by all steps.",
                fxLabelRect, juce::Justification::centredLeft, true);

    for (auto& card : fxCards)
    {
        const bool on = card->power.getToggleState();
        widgets::drawModule (g, card->bounds.toFloat(), card->title, s);
        if (on)
        {
            g.setColour (theme::accent);
            g.drawRoundedRectangle (card->bounds.toFloat().reduced (0.5f), 10.0f * s, 1.2f);
        }
    }
}

void SeqPanel::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (10 * s));
    const int gap = (int) (6 * s);

    // Due righe come nel riferimento: in alto pattern | celle | esportazione | swing | seed; sotto le maniglie per la DAW.
    // Ogni gruppo e' separato da uno spazio con una linea sottile (disegnata in paint).
    const int rowH1 = (int) (34 * s), rowH2 = (int) (32 * s), vgap = (int) (8 * s);
    const int gGap = (int) (6 * s);                    // spazio tra i controlli dello stesso gruppo
    const int sGap = (int) (18 * s);                   // spazio (con separatore al centro) tra gruppi
    separatorsX.clear();

    row1Rect = area.removeFromTop (rowH1);
    area.removeFromTop (vgap);
    row2Rect = area.removeFromTop (rowH2);
    area.removeFromTop (vgap);

    auto r1 = row1Rect;
    auto groupEnd = [&] (auto& rowRect) { separatorsX.push_back ({ rowRect.getX() + sGap / 2, 0 }); rowRect.removeFromLeft (sGap); };

    // Destra: Reset Seed, poi lo swing (che occupa l'altezza di entrambe le righe)
    reseedButton.setBounds (r1.removeFromRight ((int) (128 * s)));
    r1.removeFromRight (sGap / 2);
    {
        const int swingW = (int) (170 * s);
        auto sw = juce::Rectangle<int> (r1.getRight() - swingW, row1Rect.getY(), swingW, rowH1 + vgap + rowH2);
        r1.removeFromRight (swingW + sGap / 2);
        separatorsX.push_back ({ sw.getX() - sGap / 4, 2 });
        separatorsX.push_back ({ sw.getRight() + sGap / 4, 0 });
        swingRect = sw;
        auto knobArea = sw.withTrimmedTop ((int) (14 * s));
        swingSlider.setBounds (knobArea);
    }

    // Sinistra: pattern
    prevPattern.setBounds (r1.removeFromLeft ((int) (32 * s)));
    r1.removeFromLeft (gGap / 2);
    patternBox.setBounds (r1.removeFromLeft ((int) (150 * s)));
    r1.removeFromLeft (gGap / 2);
    nextPattern.setBounds (r1.removeFromLeft ((int) (32 * s)));
    r1.removeFromLeft (gGap);
    patternMenuButton.setBounds (r1.removeFromLeft ((int) (34 * s)));
    groupEnd (r1);

    // Celle
    addButton.setBounds (r1.removeFromLeft ((int) (78 * s)));
    r1.removeFromLeft (2);
    addAllButton.setBounds (r1.removeFromLeft ((int) (36 * s)));
    groupEnd (r1);

    // Esportazione
    barsBox.setBounds (r1.removeFromLeft ((int) (136 * s)));
    r1.removeFromLeft (gGap);
    tailBox.setBounds (r1.removeFromLeft ((int) (112 * s)));
    groupEnd (r1);
    midiButton.setBounds (r1.removeFromLeft ((int) (128 * s)));
    r1.removeFromLeft (gGap);
    wavButton.setBounds (r1.removeFromLeft ((int) (128 * s)));
    r1.removeFromLeft (gGap);
    stemsButton.setBounds (r1.removeFromLeft ((int) (128 * s)));

    // Riga 2: maniglie per trascinare i file direttamente nella DAW
    dragRow = row2Rect;
    {
        auto d = dragRow;
        d.removeFromLeft ((int) (112 * s));                                // spazio per l'etichetta "DRAG TO DAW"
        separatorsX.push_back ({ d.getX() + gGap / 2, 1 });
        d.removeFromLeft (gGap * 2);
        const int w = (int) (128 * s);
        dragMidi->setBounds (d.removeFromLeft (w));
        d.removeFromLeft (gGap);
        dragAudio->setBounds (d.removeFromLeft (w));
        d.removeFromLeft (gGap);
        dragLanes->setBounds (d.removeFromLeft (w));
    }

    // Righello dei beat sopra le righe (il testo e' disegnato in paint)
    rulerRect = area.removeFromTop ((int) (16 * s));

    // Editor dello step in basso: altezza FISSA in unita' scalate (cosi' le carte degli effetti hanno sempre lo spazio per le
    // manopole), poi le righe riempiono lo spazio rimasto. La scala segue il lato piu' stretto della finestra, quindi lo spazio
    // per le righe resta sempre almeno ~200 unita'.
    const int editorH = juce::jmin ((int) (300 * s), area.getHeight() * 66 / 100);
    editorRect = area.removeFromBottom (editorH);
    area.removeFromBottom (gap);
    viewport.setBounds (area);
    relayoutLanes();

    auto ed = editorRect.reduced ((int) (10 * s), (int) (8 * s));
    headerRect = ed.removeFromTop ((int) (24 * s));
    ed.removeFromTop ((int) (6 * s));

    // Comportamento: modalita', reverse, cancella lock
    auto behaviour = ed.removeFromTop ((int) (26 * s));
    stepModeBox.setBounds (behaviour.removeFromLeft ((int) (130 * s)));
    behaviour.removeFromLeft (gap * 2);
    reverseToggle.setBounds (behaviour.removeFromLeft ((int) (120 * s)));
    clearLocksButton.setBounds (behaviour.removeFromRight ((int) (110 * s)));
    ed.removeFromTop ((int) (4 * s));

    // Manopole dello step (altezza fissa in unita' scalate), poi le carte degli effetti con tutto lo spazio rimasto
    const int knobsH = juce::jmin ((int) (90 * s), ed.getHeight() * 45 / 100);
    auto knobRow = ed.removeFromTop (knobsH);
    KnobBox* stepKnobs[6] = { &velocityKnob, &probKnob, &nudgeKnob, &gateKnob, &pitchKnob, &ratchetKnob };
    const int kw = knobRow.getWidth() / 6;
    for (auto* k : stepKnobs)
        k->setBounds (knobRow.removeFromLeft (kw));

    fxLabelRect = ed.removeFromTop ((int) (16 * s));
    ed.removeFromTop ((int) (2 * s));

    float total = 0.0f;
    for (auto& c : fxCards) total += (float) c->numKnobs() + 1.3f;      // le carte con poche manopole restano abbastanza larghe per il titolo
    const float perUnit = (float) (ed.getWidth() - gap * ((int) fxCards.size() - 1)) / total;
    int x = ed.getX();
    for (size_t i = 0; i < fxCards.size(); ++i)
    {
        auto& c = *fxCards[i];
        const int w = i + 1 == fxCards.size() ? ed.getRight() - x : (int) (((float) c.numKnobs() + 1.3f) * perUnit);
        c.bounds = { x, ed.getY(), w, ed.getHeight() };
        x += w + gap;

        const int pw2 = (int) (26 * s);
        c.power.setBounds (juce::Rectangle<int> (c.bounds.getRight() - pw2 - (int) (4 * s), c.bounds.getY() + (int) (4 * s), pw2, (int) (15 * s)));

        auto inner = c.bounds.reduced ((int) (4 * s), 0).withTrimmedTop ((int) (22 * s)).withTrimmedBottom ((int) (4 * s));
        if (c.syncButton != nullptr)
        {
            auto syncRow = inner.removeFromTop ((int) (22 * s)).reduced ((int) (2 * s), 0);
            c.syncButton->setBounds (syncRow.removeFromLeft (syncRow.getWidth() * 36 / 100));
            syncRow.removeFromLeft ((int) (4 * s));
            c.divisionBox->setBounds (syncRow);
        }
        const int n = (int) c.numKnobs();
        const int cw = inner.getWidth() / juce::jmax (1, n);
        for (auto& k : c.knobs)
            k->box->setBounds (inner.removeFromLeft (cw));
        for (auto& ck : c.cellKnobs)
            ck->box->setBounds (inner.removeFromLeft (cw));
    }
}
