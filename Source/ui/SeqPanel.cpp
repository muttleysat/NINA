#include "SeqPanel.h"
#include "Widgets.h"

namespace
{
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

    ~LaneRow() override
    {
        if (held)
            owner.processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
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
        auto area = getLocalBounds().reduced (0, (int) (2 * s));
        const int bw = (int) (24 * s);

        auto header = area.removeFromLeft (headerWidth());
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

        // Colonne di step con uno spazio ogni 4 (si leggono i beat)
        area.removeFromLeft ((int) (8 * s));
        const int gap = (int) (7 * s);
        const int w = (area.getWidth() - 3 * gap) / kPageSize;
        for (int i = 0; i < kPageSize; ++i)
        {
            steps[(size_t) i]->setBounds (area.removeFromLeft (w));
            if (i % 4 == 3 && i < kPageSize - 1)
                area.removeFromLeft (gap);
        }
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
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (held)
            owner.processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
        held = false;
        repaint (labelRect);
    }

    juce::Rectangle<int> getStepColumn (int c) const { return steps[(size_t) c]->getBounds(); }

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
    juce::Rectangle<int> columnBounds (int c) const
    {
        return rows.empty() ? juce::Rectangle<int>() : rows.front()->getStepColumn (c);
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

// ----------------------------------------------------------- Carte effetti dello step

struct SeqPanel::FxKnob
{
    std::unique_ptr<KnobBox> box;
    sl::FxF which;
    DoubleClickHandler handler;
};

struct SeqPanel::FxCard
{
    juce::String title;
    sl::FxB effect = sl::FxB::Sat;
    juce::ToggleButton power;
    std::vector<std::unique_ptr<FxKnob>> knobs;
    juce::Rectangle<int> bounds;
};

// ------------------------------------------------------------------- SeqPanel

SeqPanel::SeqPanel (SampleLabProcessor& p) : processor (p)
{
    lanes = std::make_unique<LaneList> (*this);
    viewport.setViewedComponent (lanes.get(), false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    // Il Play/Stop e' nella barra in alto dell'applicazione (e con la barra spaziatrice)
    for (auto* b : { &addButton, &midiButton, &wavButton, &reseedButton, &prevPattern, &nextPattern })
        addAndMakeVisible (*b);

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

    for (const char* t : { "1 bar", "2 bars", "4 bars", "8 bars" })
        barsBox.addItem (t, barsBox.getNumItems() + 1);
    barsBox.setSelectedId (2, juce::dontSendNotification);
    barsBox.setTooltip ("Length used by Export MIDI and Bounce WAV");
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
        b.onClick = [this, i] { page = i; lanes->updateAll(); repaint (rulerRect); };
        addAndMakeVisible (b);
    }
    pageButtons[0].setToggleState (true, juce::dontSendNotification);

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
    auto addCard = [this] (const char* title, sl::FxB effect, std::initializer_list<std::pair<const char*, sl::FxF>> knobList)
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
        fxCards.push_back (std::move (card));
    };
    using F = sl::FxF;
    addCard ("Saturation", sl::FxB::Sat, { { "Drive", F::Drive }, { "Mix", F::SatMix } });
    addCard ("LoFi", sl::FxB::Lofi, { { "Bits", F::Bits }, { "Rate", F::Downsample }, { "Mix", F::LofiMix } });
    addCard ("Filter", sl::FxB::Filter, { { "Cutoff", F::Cutoff }, { "Reso", F::Resonance } });
    addCard ("Delay", sl::FxB::Delay, { { "Send", F::DelayMix } });
    addCard ("Reverb", sl::FxB::Reverb, { { "Send", F::ReverbMix } });

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
        refreshEditor();                // la manopola Velocity dell'editor segue il trascinamento
    lanes->updateAll();
}

void SeqPanel::selectStep (int lane, int step)
{
    selLane = lane;
    selStep = step;
    refreshEditor();
    lanes->updateAll();
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
    }
    updatingEditor = false;
    repaint (headerRect);
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
    lanes->updateAll();

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

    // Righello dei beat: numero dello step ogni 4, allineato alle colonne
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
    for (int c = 0; c < kPageSize; c += 4)
    {
        const auto col = lanes->columnBounds (c);
        if (col.isEmpty())
            continue;
        g.drawText (juce::String (page * kPageSize + c + 1),
                    juce::Rectangle<int> (viewport.getX() + col.getX(), rulerRect.getY(), col.getWidth() * 4, rulerRect.getHeight()),
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
    g.drawText ("EFFECTS (STEP)  -  per-hit: only this step changes. Compressor, delay time/feedback and reverb size belong to the cell (FX tab).",
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

    // Riga 1: trasporto, pattern, aggiunta celle, swing
    auto top = area.removeFromTop ((int) (28 * s));
    prevPattern.setBounds (top.removeFromLeft ((int) (28 * s)));
    patternBox.setBounds (top.removeFromLeft ((int) (130 * s)));
    nextPattern.setBounds (top.removeFromLeft ((int) (28 * s)));
    top.removeFromLeft (gap);
    addButton.setBounds (top.removeFromLeft ((int) (70 * s)));
    top.removeFromLeft (gap * 2);
    reseedButton.setBounds (top.removeFromRight ((int) (86 * s)));
    top.removeFromRight (gap);
    swingSlider.setBounds (top);
    area.removeFromTop (gap);

    // Riga 2: esportazione (battute, MIDI, audio) e pagine di step
    auto row2 = area.removeFromTop ((int) (26 * s));
    barsBox.setBounds (row2.removeFromLeft ((int) (96 * s)));
    row2.removeFromLeft (gap);
    midiButton.setBounds (row2.removeFromLeft ((int) (104 * s)));
    row2.removeFromLeft (gap);
    wavButton.setBounds (row2.removeFromLeft ((int) (104 * s)));
    const int pw = juce::jmin ((int) (58 * s), juce::jmax (30, row2.getWidth() / 4));
    auto pages = row2.removeFromRight (pw * 4);
    for (auto& b : pageButtons)
        b.setBounds (pages.removeFromLeft (pw).reduced ((int) (2 * s), 0));
    area.removeFromTop (gap);

    // Righello dei beat sopra le righe (il testo e' disegnato in paint)
    rulerRect = area.removeFromTop ((int) (16 * s));

    // Editor dello step in basso (altezza adattiva), poi le righe riempiono lo spazio rimasto
    const int editorH = juce::jlimit ((int) (190 * s), (int) (292 * s), area.getHeight() * 54 / 100);
    editorRect = area.removeFromBottom (editorH);
    area.removeFromBottom (gap);
    viewport.setBounds (area);
    lanes->setSize (juce::jmax (1, viewport.getMaximumVisibleWidth()), juce::jmax (lanes->desiredHeight(), 1));

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

    // Manopole dello step (altezza ~ 48% dello spazio rimasto), poi le carte degli effetti
    const int knobsH = ed.getHeight() * 46 / 100;
    auto knobRow = ed.removeFromTop (knobsH);
    KnobBox* stepKnobs[6] = { &velocityKnob, &probKnob, &nudgeKnob, &gateKnob, &pitchKnob, &ratchetKnob };
    const int kw = knobRow.getWidth() / 6;
    for (auto* k : stepKnobs)
        k->setBounds (knobRow.removeFromLeft (kw));

    fxLabelRect = ed.removeFromTop ((int) (16 * s));
    ed.removeFromTop ((int) (2 * s));

    float total = 0.0f;
    for (auto& c : fxCards) total += (float) c->knobs.size() + 1.3f;     // le carte con una sola manopola restano abbastanza larghe per il titolo
    const float perUnit = (float) (ed.getWidth() - gap * ((int) fxCards.size() - 1)) / total;
    int x = ed.getX();
    for (size_t i = 0; i < fxCards.size(); ++i)
    {
        auto& c = *fxCards[i];
        const int w = i + 1 == fxCards.size() ? ed.getRight() - x : (int) (((float) c.knobs.size() + 1.3f) * perUnit);
        c.bounds = { x, ed.getY(), w, ed.getHeight() };
        x += w + gap;

        const int pw2 = (int) (30 * s);
        c.power.setBounds (juce::Rectangle<int> (c.bounds.getRight() - pw2 - (int) (6 * s), c.bounds.getY() + (int) (4 * s), pw2, (int) (16 * s)));

        auto inner = c.bounds.reduced ((int) (4 * s), 0).withTrimmedTop ((int) (22 * s)).withTrimmedBottom ((int) (4 * s));
        const int n = (int) c.knobs.size();
        const int cw = inner.getWidth() / juce::jmax (1, n);
        for (auto& k : c.knobs)
            k->box->setBounds (inner.removeFromLeft (cw));
    }
}
