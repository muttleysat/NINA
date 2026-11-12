#include "SeqPanel.h"

namespace
{
const char* lockNames[(size_t) sl::FxF::Count] = {
    "Sat Drive", "Sat Mix", "LoFi Bits", "LoFi Rate", "LoFi Mix", "Filter Cutoff", "Filter Reso",
    "Comp Threshold", "Comp Ratio", "Comp Attack", "Comp Release", "Comp Makeup",
    "Delay Time", "Delay Feedback", "Delay Mix", "Reverb Size", "Reverb Damp", "Reverb Mix" };

// Dopo i 18 valori, gli interruttori degli effetti: un lock puo' accendere o spegnere un effetto solo per quello step
const char* lockOnNames[(size_t) sl::FxB::Count] = {
    "Saturation On/Off", "LoFi On/Off", "Filter On/Off", "Compressor On/Off", "Delay On/Off", "Reverb On/Off" };

constexpr int kPageSize = 16;

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
        const auto b = getLocalBounds().toFloat().reduced (1.5f * s);

        g.setColour (state.beyond ? theme::panel : (state.beat ? theme::panelHi.brighter (0.07f) : theme::panelHi));
        g.fillRoundedRectangle (b, 5.0f * s);
        if (state.beyond)
            return;                                            // oltre la lunghezza della lane

        if (state.on)
        {
            // L'altezza della barra e' la velocity
            const float h = b.getHeight() * (0.25f + 0.75f * (float) state.velocity / 127.0f);
            g.setColour (theme::accent);
            g.fillRoundedRectangle (b.withTop (b.getBottom() - h), 5.0f * s);
        }
        if (state.current)
        {
            g.setColour (theme::text.withAlpha (0.95f));
            g.drawRoundedRectangle (b, 5.0f * s, 1.6f);
        }
        if (state.selected)
        {
            g.setColour (theme::accentSoft);
            g.drawRoundedRectangle (b.expanded (0.5f), 5.0f * s, 2.0f);
        }

        g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.5f * s), juce::Font::bold));
        g.setColour (theme::background);
        if (state.on && state.locked)
            g.fillEllipse (b.getRight() - 9.0f * s, b.getY() + 3.0f * s, 6.0f * s, 6.0f * s);
        if (state.on && state.ratchet > 1)
            g.drawText ("x" + juce::String (state.ratchet), b.reduced (3.0f * s), juce::Justification::topLeft);
        if (state.on && state.hasProb)
            g.drawText ("%", b.reduced (3.0f * s), juce::Justification::bottomRight);
    }

    // Clic: accende/seleziona (vedi SeqPanel::stepClicked). Trascinamento verticale su uno step acceso: cambia la velocity.
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragged = false;
        if (state.beyond)
            return;
        pressY = e.y;
        pressVelocity = state.velocity;
        rightOrShift = e.mods.isPopupMenu() || e.mods.isShiftDown();
        if (rightOrShift)
            owner.stepClicked (lane, owner.page * kPageSize + column, true);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (state.beyond || rightOrShift || ! state.on)
            return;
        if (std::abs (e.y - pressY) > 3)
            dragged = true;
        if (dragged)
        {
            const int v = juce::jlimit (1, 127, pressVelocity + (int) std::lround ((double) (pressY - e.y) * 127.0 / juce::jmax (20, getHeight() * 2)));
            owner.stepVelocity (lane, owner.page * kPageSize + column, v);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! state.beyond && ! dragged && ! rightOrShift)
            owner.stepClicked (lane, owner.page * kPageSize + column, false);
        dragged = false;
    }

private:
    SeqPanel& owner;
    int lane;
    State state;
    int pressY = 0, pressVelocity = 100;
    bool dragged = false, rightOrShift = false;
};

// ------------------------------------------------------------------------ Lane

class SeqPanel::LaneRow final : public juce::Component
{
public:
    LaneRow (SeqPanel& o, int laneIndex) : owner (o), lane (laneIndex)
    {
        const auto& l = owner.pat().lanes[(size_t) lane];

        for (auto* b : { &muteButton, &soloButton, &removeButton })
            addAndMakeVisible (*b);
        muteButton.setClickingTogglesState (true);
        soloButton.setClickingTogglesState (true);
        muteButton.setToggleState (l.mute, juce::dontSendNotification);
        soloButton.setToggleState (l.solo, juce::dontSendNotification);
        // FX: acceso = la riga usa gli effetti della cella (default); spento = la riga suona "dry" (gli step possono riaccenderli con i lock)
        fxButton.setClickingTogglesState (true);
        fxButton.setToggleState (! l.dry, juce::dontSendNotification);
        fxButton.setTooltip ("On: this lane uses the cell's effects. Off: it plays dry (a step lock can switch an effect back on).");
        fxButton.onClick = [this] { owner.pat().lanes[(size_t) lane].dry = ! fxButton.getToggleState(); owner.edited(); };
        addAndMakeVisible (fxButton);
        muteButton.onClick = [this] { owner.pat().lanes[(size_t) lane].mute = muteButton.getToggleState(); owner.edited(); };
        soloButton.onClick = [this] { owner.pat().lanes[(size_t) lane].solo = soloButton.getToggleState(); owner.edited(); };
        removeButton.onClick = [this]
        {
            const int cell = owner.pat().lanes[(size_t) lane].cell;
            juce::MessageManager::callAsync ([o = &owner, cell] { o->processor.removeLaneForCell (cell); o->selLane = -1; o->refresh(); });
        };

        for (int i = 0; i < sl::kNumDivisions; ++i)
            divisionBox.addItem (sl::kDivisionNames[i], i + 1);
        divisionBox.setSelectedId (l.division + 1, juce::dontSendNotification);
        divisionBox.onChange = [this] { owner.pat().lanes[(size_t) lane].division = divisionBox.getSelectedId() - 1; owner.edited(); };
        addAndMakeVisible (divisionBox);

        lengthSlider.setSliderStyle (juce::Slider::IncDecButtons);
        lengthSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
        lengthSlider.setRange (1, sl::kMaxSteps, 1);
        lengthSlider.setValue (l.length, juce::dontSendNotification);
        lengthSlider.setTooltip ("Number of steps of this lane (each lane can have its own length)");
        lengthSlider.onValueChange = [this] { owner.pat().lanes[(size_t) lane].length = (int) lengthSlider.getValue(); owner.edited(); };
        addAndMakeVisible (lengthSlider);

        for (int i = 0; i < kPageSize; ++i)
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

    void update (int currentStep)
    {
        const auto& l = owner.pat().lanes[(size_t) lane];
        for (int c = 0; c < kPageSize; ++c)
        {
            const int idx = owner.page * kPageSize + c;
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
                st.beat = (c % 4) == 0;
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
        g.setColour (theme::panelHi.withAlpha (held ? 0.9f : 0.5f));
        g.fillRoundedRectangle (labelRect.toFloat(), 5.0f * s);
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
        auto area = getLocalBounds().reduced (0, (int) (2 * s));
        const int bw = (int) (24 * s);

        auto header = area.removeFromLeft (headerWidth());
        muteButton.setBounds (header.removeFromLeft (bw));
        header.removeFromLeft ((int) (2 * s));
        soloButton.setBounds (header.removeFromLeft (bw));
        header.removeFromLeft ((int) (6 * s));
        removeButton.setBounds (header.removeFromRight (bw));
        header.removeFromRight ((int) (4 * s));
        lengthSlider.setBounds (header.removeFromRight ((int) (70 * s)));
        header.removeFromRight ((int) (4 * s));
        divisionBox.setBounds (header.removeFromRight ((int) (62 * s)));
        header.removeFromRight ((int) (4 * s));
        fxButton.setBounds (header.removeFromRight ((int) (28 * s)));
        header.removeFromRight ((int) (6 * s));
        labelRect = header;

        area.removeFromLeft ((int) (6 * s));
        const int w = area.getWidth() / kPageSize;
        for (auto& b : steps)
            b->setBounds (area.removeFromLeft (w));
    }

    int headerWidth() const { return (int) (310 * theme::scaleFor (*this)); }

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
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (held)
            owner.processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
        held = false;
        repaint (labelRect);
    }

    ~LaneRow() override
    {
        if (held)
            owner.processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
    }

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

    int rowHeight() const { return juce::jmax (26, (int) (40 * theme::scaleFor (*this))); }
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

    void resized() override
    {
        const int rh = rowHeight();
        for (size_t i = 0; i < rows.size(); ++i)
            rows[i]->setBounds (0, (int) i * rh, getWidth(), rh);
    }

private:
    SeqPanel& owner;
    std::vector<std::unique_ptr<LaneRow>> rows;
};

// ------------------------------------------------------------------- SeqPanel

SeqPanel::SeqPanel (SampleLabProcessor& p) : processor (p)
{
    lanes = std::make_unique<LaneList> (*this);
    viewport.setViewedComponent (lanes.get(), false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    for (auto* b : { &playButton, &addButton, &midiButton, &wavButton, &reseedButton })
        addAndMakeVisible (*b);

    playButton.onClick = [this] { processor.setSequencerRunning (! processor.isSequencerRunning()); };
    addButton.onClick = [this] { showAddMenu(); };
    midiButton.onClick = [this] { exportMidi(); };
    wavButton.onClick = [this] { bounceWav(); };
    reseedButton.setTooltip ("New random seed: probability and ratchet choices change but stay repeatable");
    reseedButton.onClick = [this] { pat().seed = (std::uint32_t) juce::Random::getSystemRandom().nextInt(); edited(); };

    for (int i = 0; i < sl::kNumPatterns; ++i)
        patternBox.addItem (juce::String (i + 1) + "  " + processor.getKit().patterns[(size_t) i].name, i + 1);
    patternBox.onChange = [this]
    {
        processor.setActivePattern (patternBox.getSelectedId() - 1);
        selLane = selStep = -1;
        refresh();
    };
    addAndMakeVisible (patternBox);

    for (const char* t : { "1 bar", "2 bars", "4 bars", "8 bars" })
        barsBox.addItem (t, barsBox.getNumItems() + 1);
    barsBox.setSelectedId (2, juce::dontSendNotification);
    addAndMakeVisible (barsBox);

    swingSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    swingSlider.setRange (0.0, 50.0, 1.0);
    swingSlider.setTextValueSuffix (" % swing");
    swingSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 90, 20);
    swingSlider.onValueChange = [this] { pat().swing = (float) swingSlider.getValue() / 100.0f; edited(); };
    addAndMakeVisible (swingSlider);

    const char* pageNames[4] = { "1-16", "17-32", "33-48", "49-64" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = pageButtons[i];
        b.setButtonText (pageNames[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (4713);
        b.onClick = [this, i] { page = i; lanes->updateAll(); };
        addAndMakeVisible (b);
    }
    pageButtons[0].setToggleState (true, juce::dontSendNotification);

    // Editor dello step
    auto setup = [this] (juce::Slider& s, double lo, double hi, double step, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setRange (lo, hi, step);
        s.setTextValueSuffix (suffix);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
        addAndMakeVisible (s);
    };
    setup (velocitySlider, 1, 127, 1, "");
    setup (probSlider, 0, 100, 1, " %");
    setup (nudgeSlider, -50, 50, 1, " %");
    setup (gateSlider, 5, 100, 1, " %");
    setup (pitchSlider, -24, 24, 1, " st");
    setup (lockSlider, 0, 1, 0.01, "");

    auto onStep = [this] (auto fn)
    {
        return [this, fn]
        {
            if (updatingEditor || selLane < 0 || selStep < 0 || selLane >= (int) pat().lanes.size())
                return;
            fn (pat().lanes[(size_t) selLane].steps[(size_t) selStep]);
            edited();
        };
    };
    velocitySlider.onValueChange = onStep ([this] (sl::Step& s) { s.velocity = (std::uint8_t) velocitySlider.getValue(); });
    probSlider.onValueChange = onStep ([this] (sl::Step& s) { s.probability = (std::uint8_t) probSlider.getValue(); });
    nudgeSlider.onValueChange = onStep ([this] (sl::Step& s) { s.nudge = (float) nudgeSlider.getValue() / 100.0f; });
    gateSlider.onValueChange = onStep ([this] (sl::Step& s) { s.gate = (float) gateSlider.getValue() / 100.0f; });
    pitchSlider.onValueChange = onStep ([this] (sl::Step& s) { s.pitch = (std::int8_t) pitchSlider.getValue(); });

    for (int i = 1; i <= 4; ++i)
        ratchetBox.addItem (i == 1 ? juce::String ("No ratchet") : "Ratchet x" + juce::String (i), i);
    ratchetBox.onChange = onStep ([this] (sl::Step& s) { s.ratchet = (std::uint8_t) ratchetBox.getSelectedId(); });
    addAndMakeVisible (ratchetBox);

    // Comportamento dello step: reverse e modalita' di riproduzione (come la cella, oppure one-shot/gate/loop)
    reverseToggle.onClick = onStep ([this] (sl::Step& s) { s.reverse = reverseToggle.getToggleState(); });
    addAndMakeVisible (reverseToggle);

    stepModeBox.addItem ("Mode: as cell", 1);
    stepModeBox.addItem ("Mode: one-shot", 2);
    stepModeBox.addItem ("Mode: gate", 3);
    stepModeBox.addItem ("Mode: loop", 4);
    stepModeBox.onChange = onStep ([this] (sl::Step& s) { s.mode = (std::int8_t) (stepModeBox.getSelectedId() - 2); });
    addAndMakeVisible (stepModeBox);

    // Parameter lock: parametro dell'effetto (o interruttore on/off) + valore solo per questo step
    for (int i = 0; i < (int) sl::FxF::Count; ++i)
        lockParamBox.addItem (lockNames[i], i + 1);
    for (int i = 0; i < (int) sl::FxB::Count; ++i)
        lockParamBox.addItem (lockOnNames[i], (int) sl::FxF::Count + i + 1);
    lockParamBox.setSelectedId (6, juce::dontSendNotification);           // Filter Cutoff
    lockParamBox.onChange = [this] { updateLockControls(); };
    addAndMakeVisible (lockParamBox);

    lockSlider.onValueChange = [this] { applyLockSlider (true); };
    lockToggle.onClick = [this] { applyLockSlider (false); };
    addAndMakeVisible (lockToggle);

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

void SeqPanel::refresh()
{
    patternBox.setSelectedId (processor.getActivePatternIndex() + 1, juce::dontSendNotification);
    swingSlider.setValue (pat().swing * 100.0, juce::dontSendNotification);

    if (selLane >= (int) pat().lanes.size())
        selLane = selStep = -1;

    lanes->rebuild();
    lanes->setSize (juce::jmax (1, viewport.getMaximumVisibleWidth()), lanes->desiredHeight());
    refreshEditor();
    lanes->updateAll();
    resized();
    theme::noButtonFocus (*this);       // i pulsanti delle nuove righe non rubano la barra spaziatrice
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

void SeqPanel::stepVelocity (int lane, int step, int velocity)
{
    if (lane < 0 || lane >= (int) pat().lanes.size() || step < 0 || step >= sl::kMaxSteps)
        return;

    pat().lanes[(size_t) lane].steps[(size_t) step].velocity = (std::uint8_t) juce::jlimit (1, 127, velocity);
    edited();
    if (selLane == lane && selStep == step)
        refreshEditor();                // il cursore Velocity dell'editor segue il trascinamento
    lanes->updateAll();
}

void SeqPanel::selectStep (int lane, int step)
{
    selLane = lane;
    selStep = step;
    refreshEditor();
    lanes->updateAll();
}

void SeqPanel::refreshEditor()
{
    const bool valid = selLane >= 0 && selLane < (int) pat().lanes.size() && selStep >= 0;
    for (juce::Component* c : { (juce::Component*) &velocitySlider, (juce::Component*) &probSlider, (juce::Component*) &nudgeSlider,
                                (juce::Component*) &gateSlider, (juce::Component*) &pitchSlider, (juce::Component*) &ratchetBox,
                                (juce::Component*) &lockParamBox, (juce::Component*) &lockSlider, (juce::Component*) &lockToggle,
                                (juce::Component*) &reverseToggle, (juce::Component*) &stepModeBox })
        c->setEnabled (valid);

    updatingEditor = true;
    if (valid)
    {
        const auto& s = pat().lanes[(size_t) selLane].steps[(size_t) selStep];
        velocitySlider.setValue (s.velocity, juce::dontSendNotification);
        probSlider.setValue (s.probability, juce::dontSendNotification);
        nudgeSlider.setValue (s.nudge * 100.0f, juce::dontSendNotification);
        gateSlider.setValue (s.gate * 100.0f, juce::dontSendNotification);
        pitchSlider.setValue (s.pitch, juce::dontSendNotification);
        ratchetBox.setSelectedId (s.ratchet, juce::dontSendNotification);
        reverseToggle.setToggleState (s.reverse, juce::dontSendNotification);
        stepModeBox.setSelectedId (s.mode + 2, juce::dontSendNotification);
    }
    updatingEditor = false;
    updateLockControls();
    repaint();
}

// Lock: intervallo e testo del cursore seguono il parametro scelto; il valore e' quello bloccato dello step
// oppure, se non c'e' un lock, il valore corrente del parametro nella cella.
void SeqPanel::updateLockControls()
{
    const bool valid = selLane >= 0 && selLane < (int) pat().lanes.size() && selStep >= 0;
    const int idx = juce::jmax (0, lockParamBox.getSelectedId() - 1);
    const bool isSwitch = idx >= (int) sl::FxF::Count;                  // interruttore on/off di un effetto
    const auto fx = (sl::FxF) juce::jmin (idx, (int) sl::FxF::Count - 1);
    const auto sw = (sl::FxB) juce::jmax (0, idx - (int) sl::FxF::Count);

    updatingEditor = true;
    if (isSwitch)
    {
        // Interruttore: cursore a due posizioni (0 = spento, 1 = acceso)
        lockSlider.setRange (0.0, 1.0, 1.0);
        lockSlider.setSkewFactor (1.0);
        lockSlider.textFromValueFunction = [] (double v) { return v >= 0.5 ? juce::String ("On") : juce::String ("Off"); };
        lockSlider.valueFromTextFunction = [] (const juce::String& t) { return t.trim().equalsIgnoreCase ("on") ? 1.0 : 0.0; };
    }
    else
    {
        const auto* param = processor.getParams().masterFx().at (fx);   // stesso intervallo per ogni cella
        const auto range = param->getNormalisableRange();
        lockSlider.setRange ((double) range.start, (double) range.end, (double) range.interval);
        lockSlider.setSkewFactor ((double) range.skew);
        const juce::AudioProcessorParameter* paramBase = param;         // getText e' pubblico nella classe base
        lockSlider.textFromValueFunction = [param, paramBase] (double v) { return paramBase->getText (param->convertTo0to1 ((float) v), 0); };
        lockSlider.valueFromTextFunction = [] (const juce::String& t) { return (double) t.getFloatValue(); };
    }

    if (valid)
    {
        const auto& lane = pat().lanes[(size_t) selLane];
        const auto& s = lane.steps[(size_t) selStep];
        const bool locked = s.hasLock (idx);
        lockToggle.setToggleState (locked, juce::dontSendNotification);

        // Senza lock il cursore mostra il valore attuale della cella
        float base = 0.0f;
        if (lane.cell >= 0)
            base = isSwitch ? (processor.getParams().cell (lane.cell).fx.power (sw)->get() ? 1.0f : 0.0f)
                            : processor.getParams().cell (lane.cell).fx.at (fx)->get();
        lockSlider.setValue (locked ? s.lockValues[(size_t) idx] : base, juce::dontSendNotification);
    }
    lockSlider.updateText();
    updatingEditor = false;
}

void SeqPanel::applyLockSlider (bool fromUser)
{
    if (updatingEditor || selLane < 0 || selStep < 0 || selLane >= (int) pat().lanes.size())
        return;

    auto& s = pat().lanes[(size_t) selLane].steps[(size_t) selStep];
    const int idx = lockParamBox.getSelectedId() - 1;

    if (fromUser)
    {
        s.setLock (idx, (float) lockSlider.getValue());        // muovere il cursore attiva il lock
        updatingEditor = true;
        lockToggle.setToggleState (true, juce::dontSendNotification);
        updatingEditor = false;
    }
    else if (lockToggle.getToggleState())
        s.setLock (idx, (float) lockSlider.getValue());
    else
        s.clearLock (idx);

    edited();
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

void SeqPanel::timerCallback()
{
    const bool host = processor.isHostPlaying();
    playButton.setButtonText (processor.isSequencerPlaying() ? "Stop" : "Play");
    playButton.setEnabled (! host);
    playButton.setTooltip (host ? "The host transport is running: the sequencer follows it"
                                : "Start/stop the sequencer (Space)");
    playButton.setToggleState (processor.isSequencerPlaying(), juce::dontSendNotification);

    lanes->updateAll();
}

void SeqPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);

    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f * s, 1.0f);

    if (pat().lanes.empty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * s)));
        g.drawFittedText ("No cells in the sequencer yet.\nRight-click a cell in the grid and choose \"Add to sequencer\", or use \"+ Cell\".",
                          viewport.getBounds().reduced (20), juce::Justification::centred, 3);
    }

    // Editor dello step
    g.setColour (theme::panelHi.withAlpha (0.5f));
    g.fillRoundedRectangle (editorRect.toFloat(), 8.0f * s);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
    const char* names[5] = { "VELOCITY", "PROBABILITY", "NUDGE", "GATE", "PITCH" };
    for (int i = 0; i < 5; ++i)
        g.drawText (names[i], labelRects[(size_t) i], juce::Justification::centredLeft);

    juce::String title = "Click a step to turn it on and edit it  -  drag a lit step up/down for velocity  -  click it again to turn it off";
    if (selLane >= 0 && selLane < (int) pat().lanes.size())
        title = "Editing step " + juce::String (selStep + 1) + " of " + sl::cellLabel (pat().lanes[(size_t) selLane].cell, processor.getKit().cols)
                + (pat().lanes[(size_t) selLane].steps[(size_t) selStep].on ? "" : "   (step is off: click it to turn it on)");
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.5f * s), juce::Font::bold));
    g.drawText (title, titleRect, juce::Justification::centredLeft);
}

void SeqPanel::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (10 * s));
    const int gap = (int) (6 * s);

    // Riga 1: trasporto, pattern, aggiunta celle, swing
    auto top = area.removeFromTop ((int) (28 * s));
    playButton.setBounds (top.removeFromLeft ((int) (66 * s)));
    top.removeFromLeft (gap);
    patternBox.setBounds (top.removeFromLeft ((int) (150 * s)));
    top.removeFromLeft (gap);
    addButton.setBounds (top.removeFromLeft ((int) (70 * s)));
    top.removeFromLeft (gap * 2);
    reseedButton.setBounds (top.removeFromRight ((int) (66 * s)));
    top.removeFromRight (gap);
    swingSlider.setBounds (top);
    area.removeFromTop (gap);

    // Riga 2: esportazione (battute, MIDI, audio)
    auto exp = area.removeFromTop ((int) (28 * s));
    barsBox.setBounds (exp.removeFromLeft ((int) (104 * s)));
    exp.removeFromLeft (gap);
    midiButton.setBounds (exp.removeFromLeft ((int) (110 * s)));
    exp.removeFromLeft (gap);
    wavButton.setBounds (exp.removeFromLeft ((int) (110 * s)));
    area.removeFromTop (gap);

    // Riga 3: pagine di step, allineate alle colonne degli step
    auto pages = area.removeFromTop ((int) (24 * s));
    pages.removeFromLeft ((int) (310 * s) + (int) (6 * s));
    const int pw = juce::jmin ((int) (74 * s), pages.getWidth() / 4);
    for (auto& b : pageButtons)
        b.setBounds (pages.removeFromLeft (pw).reduced ((int) (2 * s), 0));
    area.removeFromTop (gap);

    // Editor dello step (in basso): titolo + 4 righe di controlli
    editorRect = area.removeFromBottom ((int) (166 * s));
    area.removeFromBottom (gap);

    viewport.setBounds (area);
    lanes->setSize (juce::jmax (1, viewport.getMaximumVisibleWidth()), juce::jmax (lanes->desiredHeight(), 1));

    auto ed = editorRect.reduced ((int) (10 * s), (int) (6 * s));
    titleRect = ed.removeFromTop ((int) (18 * s));
    ed.removeFromTop ((int) (4 * s));

    auto place = [&] (juce::Rectangle<int> row, int first, int count, juce::Slider* sliders[], int idxOffset)
    {
        const int cw = row.getWidth() / 3;
        for (int i = 0; i < count; ++i)
        {
            auto cell = row.removeFromLeft (cw);
            labelRects[(size_t) (idxOffset + i)] = cell.removeFromLeft ((int) (82 * s));
            sliders[first + i]->setBounds (cell.reduced ((int) (2 * s), 0));
        }
        return row;
    };

    juce::Slider* all5[5] = { &velocitySlider, &probSlider, &nudgeSlider, &gateSlider, &pitchSlider };
    place (ed.removeFromTop ((int) (24 * s)), 0, 3, all5, 0);                    // velocity, probabilita', nudge
    ed.removeFromTop ((int) (4 * s));
    auto row2 = ed.removeFromTop ((int) (24 * s));
    auto rest = place (row2, 3, 2, all5, 3);                                     // gate, pitch
    ratchetBox.setBounds (rest.reduced ((int) (4 * s), 0));                      // ratchet nel terzo posto

    ed.removeFromTop ((int) (4 * s));
    auto rowBehaviour = ed.removeFromTop ((int) (24 * s));                       // reverse e modalita' dello step
    reverseToggle.setBounds (rowBehaviour.removeFromLeft ((int) (190 * s)));
    stepModeBox.setBounds (rowBehaviour.removeFromLeft ((int) (170 * s)).reduced ((int) (4 * s), 0));

    ed.removeFromTop ((int) (4 * s));
    auto row3 = ed.removeFromTop ((int) (24 * s));
    lockParamBox.setBounds (row3.removeFromLeft ((int) (170 * s)));
    row3.removeFromLeft (gap);
    lockToggle.setBounds (row3.removeFromRight ((int) (80 * s)));
    lockSlider.setBounds (row3.reduced ((int) (4 * s), 0));
}
