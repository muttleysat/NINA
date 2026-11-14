#include "MixerPanel.h"
#include "Widgets.h"

// ------------------------------------------------------------------------ Meter digitale (due barre a segmenti, picco che resta)

class MixerPanel::MeterView final : public juce::Component
{
public:
    // Chiamato a ~30 Hz con i picchi (lineari) accumulati dall'ultima lettura
    void push (float pl, float pr)
    {
        update (0, pl);
        update (1, pr);
        repaint();
    }

    void resetPeaks()
    {
        clip[0] = clip[1] = false;
        hold[0] = hold[1] = kFloor;
        maxDb = kFloor;
        repaint();
    }

    float peakDb() const { return maxDb; }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float gap = 3.0f;
        const float bw = (b.getWidth() - gap) * 0.5f;
        const float ledH = juce::jmax (4.0f, b.getHeight() * 0.025f);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float x = b.getX() + (float) ch * (bw + gap);
            const auto bar = juce::Rectangle<float> (x, b.getY() + ledH + 2.0f, bw, b.getHeight() - ledH - 2.0f);
            g.setColour (theme::background);
            g.fillRoundedRectangle (bar, 2.0f);

            // Indicatore di clip (rosso fino al reset con un clic)
            g.setColour (clip[ch] ? theme::danger : theme::border);
            g.fillRoundedRectangle (juce::Rectangle<float> (x, b.getY(), bw, ledH), 1.5f);

            const int segs = 36;                                            // un segmento ogni ~1.8 dB sulla scala -60..+6
            const float sh = bar.getHeight() / (float) segs;
            const float lit = frac (level[ch]);
            for (int i = 0; i < segs; ++i)
            {
                const float pos = (float) (i + 1) / (float) segs;
                const float db = kFloor + pos * (kCeil - kFloor);
                const bool on = pos <= lit;
                juce::Colour c = db > 0.0f ? juce::Colour (0xffe0384a) : (db > -6.0f ? juce::Colour (0xffe8b43c) : (db > -18.0f ? juce::Colour (0xff9bd13c) : juce::Colour (0xff3ec17f)));
                g.setColour (on ? c : c.withAlpha (0.10f));
                g.fillRect (bar.getX() + 1.0f, bar.getBottom() - (float) (i + 1) * sh + 0.8f, bar.getWidth() - 2.0f, sh - 1.2f);
            }

            const float hy = bar.getBottom() - frac (hold[ch]) * bar.getHeight();      // linea del picco
            if (hold[ch] > kFloor + 1.0f)
            {
                g.setColour (theme::text);
                g.fillRect (bar.getX() + 1.0f, hy - 1.0f, bar.getWidth() - 2.0f, 2.0f);
            }
        }
    }

    void mouseDown (const juce::MouseEvent&) override { resetPeaks(); }

private:
    static constexpr float kFloor = -60.0f, kCeil = 6.0f;
    static float frac (float db) { return juce::jlimit (0.0f, 1.0f, (db - kFloor) / (kCeil - kFloor)); }

    void update (int ch, float peak)
    {
        const float db = juce::Decibels::gainToDecibels (peak, kFloor - 10.0f);
        if (db > level[ch])
            level[ch] = db;
        else
            level[ch] = juce::jmax (kFloor, level[ch] - 1.6f);              // discesa ~48 dB/s a 30 Hz

        if (db >= hold[ch])
        {
            hold[ch] = db;
            holdTicks[ch] = 36;                                            // il picco resta ~1.2 s
        }
        else if (holdTicks[ch] > 0)
            --holdTicks[ch];
        else
            hold[ch] = juce::jmax (kFloor, hold[ch] - 0.8f);

        if (db > 0.0f)
            clip[ch] = true;
        maxDb = juce::jmax (maxDb, db);
    }

    float level[2] { kFloor, kFloor }, hold[2] { kFloor, kFloor };
    int holdTicks[2] { 0, 0 };
    bool clip[2] { false, false };
    float maxDb = kFloor;
};

// ------------------------------------------------------------------------ Un fader con la sua lettura in dB e il picco

namespace
{
juce::String dbText (float db)
{
    if (db <= -59.95f)
        return "-inf";
    return (db > 0.0f ? "+" : "") + juce::String (db, 1);
}

// Etichetta numerica modificabile (doppio clic) per un fader di livello in dB
void setupLevelLabel (juce::Label& l, juce::Slider& fader)
{
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, theme::accentSoft);
    l.setColour (juce::Label::backgroundColourId, theme::background);
    l.setColour (juce::Label::outlineColourId, theme::border);
    l.setEditable (false, true, false);
    l.setTooltip ("Level in dB (double-click to type a value)");
    l.onTextChange = [&l, &fader]
    {
        const auto t = l.getText().retainCharacters ("-+0123456789.");
        if (t.isNotEmpty())
            fader.setValue (juce::jlimit (fader.getMinimum(), fader.getMaximum(), t.getDoubleValue()), juce::sendNotificationSync);
        l.setText (dbText ((float) fader.getValue()), juce::dontSendNotification);
    };
}
}

// ------------------------------------------------------------------------ Canale di una cella

class MixerPanel::Strip final : public juce::Component
{
public:
    Strip (MixerPanel& o, int cellIndex) : owner (o), cell (cellIndex)
    {
        auto& proc = owner.processor;
        const auto& refs = proc.getParams().cell (cell);
        const auto& cp = proc.getKit().cells[(size_t) cell];

        // Uscita verso la DAW
        outBox.addItem ("Main", 1);
        for (int i = 2; i <= sl::Engine::kMaxOutputs; ++i)
            outBox.addItem ("Out " + juce::String (i), i);
        outBox.setSelectedId (juce::jlimit (0, 15, cp.output) + 1, juce::dontSendNotification);
        outBox.setTooltip ("Output of the plugin this cell plays on (Main mix or an extra output for your DAW)");
        outBox.onChange = [this]
        {
            owner.processor.getKit().cells[(size_t) cell].output = outBox.getSelectedId() - 1;
            owner.processor.cellEdited (cell);
        };
        addAndMakeVisible (outBox);

        // Effetti accesi/spenti
        static const char* fxNames[] = { "S", "L", "F", "C", "D", "R", "E" };
        static const char* fxTips[] = { "Saturation", "LoFi", "Filter", "Compressor", "Delay", "Reverb", "EQ" };
        for (int i = 0; i < 7; ++i)
        {
            fxBtn[i].setButtonText (fxNames[i]);
            fxBtn[i].setTooltip (juce::String (fxTips[i]) + " on/off for this cell");
            fxBtn[i].setClickingTogglesState (true);
            addAndMakeVisible (fxBtn[i]);
            fxAtt[i] = std::make_unique<juce::ButtonParameterAttachment> (*refs.fx.power ((sl::FxB) i), fxBtn[i]);
        }

        // Mandate
        for (auto* k : { &sendDelay, &sendReverb })
        {
            addAndMakeVisible (*k);
            k->slider.setNumDecimalPlacesToDisplay (2);
        }
        sendDelay.slider.setTooltip ("Send to this cell's delay (turns the delay on when raised)");
        sendReverb.slider.setTooltip ("Send to this cell's reverb (turns the reverb on when raised)");
        sendDelayAtt = std::make_unique<juce::SliderParameterAttachment> (*refs.fx.at (sl::FxF::DelayMix), sendDelay.slider);
        sendReverbAtt = std::make_unique<juce::SliderParameterAttachment> (*refs.fx.at (sl::FxF::ReverbMix), sendReverb.slider);
        auto autoOn = [] (juce::Slider& sld, juce::AudioParameterBool* power)
        {
            sld.onValueChange = [&sld, power]
            {
                if (sld.isMouseButtonDown() && sld.getValue() > 0.0 && ! power->get())
                    power->setValueNotifyingHost (1.0f);
            };
        };
        autoOn (sendDelay.slider, refs.fx.power (sl::FxB::Delay));
        autoOn (sendReverb.slider, refs.fx.power (sl::FxB::Reverb));

        // Pan
        pan.setSliderStyle (juce::Slider::LinearHorizontal);
        pan.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        pan.getProperties().set ("bipolar", true);
        pan.setTooltip ("Pan (double-click: centre)");
        addAndMakeVisible (pan);
        panAtt = std::make_unique<juce::SliderParameterAttachment> (*refs.at (sl::CellF::Pan), pan);
        pan.onValueChange = [this] { repaint (panRect); };

        // Mute / Solo
        mute.setButtonText ("M");
        solo.setButtonText ("S");
        mute.setClickingTogglesState (true);
        solo.setClickingTogglesState (true);
        mute.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe0384a));
        solo.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe8d23c));
        mute.setTooltip ("Mute this cell");
        solo.setTooltip ("Solo: only the cells in solo are heard");
        addAndMakeVisible (mute);
        addAndMakeVisible (solo);
        muteAtt = std::make_unique<juce::ButtonParameterAttachment> (*refs.mute, mute);
        soloAtt = std::make_unique<juce::ButtonParameterAttachment> (*refs.solo, solo);

        // Fader e meter
        fader.setSliderStyle (juce::Slider::LinearVertical);
        fader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        fader.setTooltip ("Level (double-click: 0 dB)");
        addAndMakeVisible (fader);
        faderAtt = std::make_unique<juce::SliderParameterAttachment> (*refs.at (sl::CellF::Level), fader);
        addAndMakeVisible (meter);
        addAndMakeVisible (levelLabel);
        addAndMakeVisible (peakLabel);
        setupLevelLabel (levelLabel, fader);
        levelLabel.setText (dbText ((float) fader.getValue()), juce::dontSendNotification);
        fader.onValueChange = [this] { levelLabel.setText (dbText ((float) fader.getValue()), juce::dontSendNotification); };
        peakLabel.setJustificationType (juce::Justification::centred);
        peakLabel.setColour (juce::Label::textColourId, theme::textDim);
        peakLabel.setInterceptsMouseClicks (false, false);
        peakLabel.setTooltip ("Highest peak (click the meter to reset)");

        // MIDI learn (tasto destro)
        auto level = [this] { return (juce::RangedAudioParameter*) owner.processor.getParams().cell (cell).at (sl::CellF::Level); };
        auto panP = [this] { return (juce::RangedAudioParameter*) owner.processor.getParams().cell (cell).at (sl::CellF::Pan); };
        auto dly = [this] { return (juce::RangedAudioParameter*) owner.processor.getParams().cell (cell).fx.at (sl::FxF::DelayMix); };
        auto rev = [this] { return (juce::RangedAudioParameter*) owner.processor.getParams().cell (cell).fx.at (sl::FxF::ReverbMix); };
        learn.push_back (std::make_unique<LearnMenu> (proc, fader, level));
        learn.push_back (std::make_unique<LearnMenu> (proc, pan, panP));
        learn.push_back (std::make_unique<LearnMenu> (proc, sendDelay, dly));
        learn.push_back (std::make_unique<LearnMenu> (proc, sendReverb, rev));
    }

    void update (std::pair<float, float> peaks)
    {
        meter.push (peaks.first, peaks.second);
        const float pk = meter.peakDb();
        peakLabel.setText (pk <= -59.0f ? juce::String ("-inf") : dbText (pk), juce::dontSendNotification);
        peakLabel.setColour (juce::Label::textColourId, pk > 0.0f ? theme::danger : theme::textDim);

        // Le mandate contano solo se l'effetto e' acceso: altrimenti sono attenuate
        const auto& fx = owner.processor.getParams().cell (cell).fx;
        sendDelay.setAlpha (fx.power (sl::FxB::Delay)->get() ? 1.0f : 0.4f);
        sendReverb.setAlpha (fx.power (sl::FxB::Reverb)->get() ? 1.0f : 0.4f);
    }

    void setSelected (bool on)
    {
        if (on != selected)
        {
            selected = on;
            repaint();
        }
    }

    int getCell() const { return cell; }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto b = getLocalBounds().toFloat().reduced (1.0f);
        const auto& cp = owner.processor.getKit().cells[(size_t) cell];
        const bool dim = cp.mute;

        g.setColour (theme::panelHi.withAlpha (0.55f));
        g.fillRoundedRectangle (b, 8.0f * s);
        g.setColour (selected ? theme::accent : theme::border);
        g.drawRoundedRectangle (b, 8.0f * s, selected ? 2.0f : 1.0f);

        // Barra del nome in basso, nel colore della cella
        const auto col = juce::Colour (cp.colour);
        g.setColour (col.withAlpha (dim ? 0.35f : 0.9f));
        g.fillRoundedRectangle (nameRect.toFloat(), 5.0f * s);
        g.setColour (juce::Colour (0xff10151c));
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s), juce::Font::bold));
        g.drawText (sl::cellLabel (cell, owner.processor.getKit().cols) + "  " + (cp.name.isEmpty() ? juce::String ("-") : cp.name), nameRect.reduced ((int) (4 * s), 0),
                    juce::Justification::centred, true);

        // Etichette
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.5f * s), juce::Font::bold));
        g.drawText ("FX", fxLabelRect, juce::Justification::centredLeft);
        const float pv = (float) pan.getValue();
        const juce::String pt = std::abs (pv) < 0.005f ? juce::String ("C") : (pv < 0 ? "L" + juce::String (juce::roundToInt (-pv * 100)) : "R" + juce::String (juce::roundToInt (pv * 100)));
        g.drawText ("PAN", panRect.withWidth (panRect.getWidth() / 2), juce::Justification::centredLeft);
        g.setColour (theme::accentSoft);
        g.drawText (pt, panRect.withTrimmedLeft (panRect.getWidth() / 2), juce::Justification::centredRight);
    }

    void resized() override
    {
        const float s = theme::scaleFor (*this);
        auto r = getLocalBounds().reduced ((int) (6 * s), (int) (6 * s));
        const int gap = (int) (5 * s);

        nameRect = r.removeFromBottom ((int) (22 * s));
        r.removeFromBottom (gap);

        outBox.setBounds (r.removeFromTop ((int) (22 * s)));
        r.removeFromTop (gap);

        fxLabelRect = r.removeFromTop ((int) (11 * s));
        {
            auto row = r.removeFromTop ((int) (22 * s));
            const int bw = row.getWidth() / 7;
            for (int i = 0; i < 7; ++i)
                fxBtn[i].setBounds (row.removeFromLeft (bw).reduced (1, 0));
        }
        r.removeFromTop (gap);

        {
            auto row = r.removeFromTop ((int) (66 * s));
            const int hw = row.getWidth() / 2;
            sendDelay.setBounds (row.removeFromLeft (hw));
            sendReverb.setBounds (row);
            sendDelay.label.setText ("Dly", juce::dontSendNotification);
            sendReverb.label.setText ("Rev", juce::dontSendNotification);
        }
        r.removeFromTop (gap);

        panRect = r.removeFromTop ((int) (12 * s));
        pan.setBounds (r.removeFromTop ((int) (16 * s)));
        r.removeFromTop (gap);

        {
            auto row = r.removeFromTop ((int) (26 * s));
            const int hw = row.getWidth() / 2;
            mute.setBounds (row.removeFromLeft (hw).reduced (2, 0));
            solo.setBounds (row.reduced (2, 0));
        }
        r.removeFromTop (gap);

        peakLabel.setBounds (r.removeFromBottom ((int) (14 * s)));
        levelLabel.setBounds (r.removeFromBottom ((int) (20 * s)).reduced ((int) (4 * s), 0));
        r.removeFromBottom (gap);

        // Fader (con la sua scala a sinistra) e meter
        const int meterW = (int) (22 * s);
        meter.setBounds (r.removeFromRight (meterW).reduced (0, (int) (10 * s)));
        r.removeFromRight ((int) (4 * s));
        fader.setBounds (r);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (nameRect.contains (e.getPosition()) && owner.onCellSelected)
            owner.onCellSelected (cell);
    }

private:
    MixerPanel& owner;
    int cell;
    bool selected = false;
    juce::ComboBox outBox;
    juce::TextButton fxBtn[7], mute, solo;
    KnobBox sendDelay { "Dly" }, sendReverb { "Rev" };
    juce::Slider pan, fader;
    juce::Label levelLabel, peakLabel;
    MeterView meter;
    juce::Rectangle<int> nameRect, fxLabelRect, panRect;

    std::unique_ptr<juce::ButtonParameterAttachment> fxAtt[7], muteAtt, soloAtt;
    std::unique_ptr<juce::SliderParameterAttachment> sendDelayAtt, sendReverbAtt, panAtt, faderAtt;
    std::vector<std::unique_ptr<LearnMenu>> learn;
};

// ------------------------------------------------------------------------ Master

class MixerPanel::MasterStrip final : public juce::Component
{
public:
    explicit MasterStrip (MixerPanel& o) : owner (o)
    {
        auto& proc = owner.processor;
        fader.setSliderStyle (juce::Slider::LinearVertical);
        fader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        fader.setTooltip ("Master volume (double-click: 0 dB)");
        addAndMakeVisible (fader);
        att = std::make_unique<juce::SliderParameterAttachment> (*proc.getParams().masterLevel(), fader);
        addAndMakeVisible (meter);
        addAndMakeVisible (levelLabel);
        addAndMakeVisible (peakLabel);
        setupLevelLabel (levelLabel, fader);
        levelLabel.setText (dbText ((float) fader.getValue()), juce::dontSendNotification);
        fader.onValueChange = [this] { levelLabel.setText (dbText ((float) fader.getValue()), juce::dontSendNotification); };
        peakLabel.setJustificationType (juce::Justification::centred);
        peakLabel.setColour (juce::Label::textColourId, theme::textDim);
        peakLabel.setInterceptsMouseClicks (false, false);
        learn = std::make_unique<LearnMenu> (proc, fader, [this] { return (juce::RangedAudioParameter*) owner.processor.getParams().masterLevel(); });
    }

    void update (std::pair<float, float> peaks)
    {
        meter.push (peaks.first, peaks.second);
        const float pk = meter.peakDb();
        peakLabel.setText (pk <= -59.0f ? juce::String ("-inf") : dbText (pk), juce::dontSendNotification);
        peakLabel.setColour (juce::Label::textColourId, pk > 0.0f ? theme::danger : theme::textDim);
    }

    void paint (juce::Graphics& g) override
    {
        const float s = theme::scaleFor (*this);
        const auto b = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (theme::panelHi.withAlpha (0.8f));
        g.fillRoundedRectangle (b, 8.0f * s);
        g.setColour (theme::accent.withAlpha (0.6f));
        g.drawRoundedRectangle (b, 8.0f * s, 1.4f);
        g.setColour (theme::accent);
        g.fillRoundedRectangle (nameRect.toFloat(), 5.0f * s);
        g.setColour (juce::Colour (0xff1a1006));
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 12.5f * s), juce::Font::bold));
        g.drawText ("MASTER", nameRect, juce::Justification::centred);
    }

    void resized() override
    {
        const float s = theme::scaleFor (*this);
        auto r = getLocalBounds().reduced ((int) (6 * s), (int) (6 * s));
        const int gap = (int) (5 * s);
        nameRect = r.removeFromBottom ((int) (22 * s));
        r.removeFromBottom (gap);
        peakLabel.setBounds (r.removeFromBottom ((int) (14 * s)));
        levelLabel.setBounds (r.removeFromBottom ((int) (20 * s)).reduced ((int) (4 * s), 0));
        r.removeFromBottom (gap);
        r.removeFromTop ((int) (6 * s));
        meter.setBounds (r.removeFromRight ((int) (30 * s)).reduced (0, (int) (10 * s)));
        r.removeFromRight ((int) (4 * s));
        fader.setBounds (r);
    }

private:
    MixerPanel& owner;
    juce::Slider fader;
    juce::Label levelLabel, peakLabel;
    MeterView meter;
    juce::Rectangle<int> nameRect;
    std::unique_ptr<juce::SliderParameterAttachment> att;
    std::unique_ptr<LearnMenu> learn;
};

// ------------------------------------------------------------------------ Contenitore scorrevole dei canali

class MixerPanel::Holder final : public juce::Component
{
public:
    void rebuild (MixerPanel& owner, const std::vector<int>& cells)
    {
        strips.clear();
        for (const int c : cells)
        {
            auto s = std::make_unique<Strip> (owner, c);
            addAndMakeVisible (*s);
            strips.push_back (std::move (s));
        }
    }

    int arrange (int height, float s)
    {
        const int w = (int) (104 * s), gap = (int) (6 * s);
        for (int i = 0; i < (int) strips.size(); ++i)
            strips[(size_t) i]->setBounds (i * (w + gap), 0, w, height);
        const int total = juce::jmax (1, (int) strips.size()) * (w + gap);
        setSize (total, height);
        return total;
    }

    std::vector<std::unique_ptr<Strip>> strips;
};

// ------------------------------------------------------------------------ Scheda

MixerPanel::MixerPanel (SampleLabProcessor& p) : processor (p)
{
    holder = std::make_unique<Holder>();
    viewport.setViewedComponent (holder.get(), false);
    viewport.setScrollBarsShown (false, true);
    addAndMakeVisible (viewport);

    master = std::make_unique<MasterStrip> (*this);
    addAndMakeVisible (*master);

    addAndMakeVisible (allCellsToggle);
    addAndMakeVisible (resetMuteButton);
    addAndMakeVisible (resetSoloButton);
    allCellsToggle.setTooltip ("Show the cells without a sample too");
    allCellsToggle.onClick = [this] { refresh(); };
    resetMuteButton.setTooltip ("Turn off Mute on every cell");
    resetMuteButton.onClick = [this] { clearMutes(); };
    resetSoloButton.setTooltip ("Turn off Solo on every cell");
    resetSoloButton.onClick = [this] { clearSolos(); };

    refresh();
    startTimerHz (30);
}

MixerPanel::~MixerPanel() { stopTimer(); }

std::vector<int> MixerPanel::visibleCells() const
{
    std::vector<int> out;
    const auto& kit = processor.getKit();
    for (int c = 0; c < juce::jmin (kit.numCells(), sl::kNumCells); ++c)
        if (allCellsToggle.getToggleState() || kit.cells[(size_t) c].hasSample())
            out.push_back (c);
    return out;
}

void MixerPanel::refresh()
{
    shownCells = visibleCells();
    holder->rebuild (*this, shownCells);
    const float s = theme::scaleFor (*this);
    holder->arrange (juce::jmax (1, viewport.getMaximumVisibleHeight()), s);
    setSelectedCell (selectedCell);
    theme::noButtonFocus (*this);
    resized();
    repaint();
}

void MixerPanel::setSelectedCell (int cell)
{
    selectedCell = cell;
    for (auto& s : holder->strips)
        s->setSelected (s->getCell() == cell);
}

void MixerPanel::clearMutes()
{
    for (int c = 0; c < sl::kNumCells; ++c)
        if (auto* m = processor.getParams().cell (c).mute; m->get())
            m->setValueNotifyingHost (0.0f);
}

void MixerPanel::clearSolos()
{
    for (int c = 0; c < sl::kNumCells; ++c)
        if (auto* m = processor.getParams().cell (c).solo; m->get())
            m->setValueNotifyingHost (0.0f);
}

void MixerPanel::timerCallback()
{
    if (! isVisible())
        return;

    // Se sono cambiate le celle con campione (apertura di un kit, caricamento) i canali si rifanno
    if (shownCells != visibleCells())
        refresh();

    for (auto& s : holder->strips)
        s->update (processor.takeCellPeaks (s->getCell()));
    master->update (processor.takeMasterPeaks());
}

void MixerPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f * s, 1.0f);

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * s), juce::Font::bold));
    g.drawText ("MIXER", headerRect.withWidth ((int) (90 * s)).withTrimmedLeft ((int) (6 * s)), juce::Justification::centredLeft);

    if (shownCells.empty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * s)));
        g.drawFittedText ("No samples loaded yet.\nLoad samples in the Main tab (or turn on \"All cells\"): each cell gets a channel here.",
                          viewport.getBounds().reduced (20), juce::Justification::centred, 3);
    }
}

void MixerPanel::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (10 * s));
    headerRect = area.removeFromTop ((int) (28 * s));
    area.removeFromTop ((int) (8 * s));

    {
        auto h = headerRect;
        h.removeFromLeft ((int) (100 * s));
        allCellsToggle.setBounds (h.removeFromLeft ((int) (110 * s)));
        h.removeFromLeft ((int) (8 * s));
        resetMuteButton.setBounds (h.removeFromLeft ((int) (120 * s)));
        h.removeFromLeft ((int) (6 * s));
        resetSoloButton.setBounds (h.removeFromLeft ((int) (120 * s)));
    }

    const int masterW = (int) (124 * s);
    master->setBounds (area.removeFromRight (masterW));
    area.removeFromRight ((int) (8 * s));

    viewport.setBounds (area);
    holder->arrange (juce::jmax (1, viewport.getMaximumVisibleHeight()), s);
}
