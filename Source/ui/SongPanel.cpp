#include "SongPanel.h"
#include "Widgets.h"

juce::Colour SongPanel::patternColour (int pattern)
{
    return juce::Colour::fromHSV ((float) (pattern % sl::kNumPatterns) / (float) sl::kNumPatterns, 0.55f, 0.88f, 1.0f);
}

// ------------------------------------------------------------------------ Scheda di un blocco

class SongPanel::BlockCard final : public juce::Component
{
public:
    BlockCard (SongPanel& o, int idx) : owner (o), index (idx)
    {
        const auto& b = owner.song().blocks[(size_t) index];
        const auto& kit = owner.processor.getKit();
        for (int i = 0; i < sl::kNumPatterns; ++i)
            patternBox.addItem (juce::String (i + 1) + "  " + kit.patterns[(size_t) i].name, i + 1);
        patternBox.setSelectedId (b.pattern + 1, juce::dontSendNotification);
        patternBox.setTooltip ("Pattern played by this block");
        patternBox.onChange = [this]
        {
            owner.song().blocks[(size_t) index].pattern = patternBox.getSelectedId() - 1;
            owner.edited();
            repaint();
            owner.repaint (owner.mapRect);
        };
        addAndMakeVisible (patternBox);

        repeatsSlider.setSliderStyle (juce::Slider::IncDecButtons);
        repeatsSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);
        repeatsSlider.setRange (1, sl::Song::kMaxRepeats, 1);
        repeatsSlider.setValue (b.repeats, juce::dontSendNotification);
        repeatsSlider.setTextValueSuffix (" x");
        repeatsSlider.setTooltip ("How many times the pattern plays (one time = one cycle of the pattern, as long as its longest lane)");
        repeatsSlider.onValueChange = [this]
        {
            owner.song().blocks[(size_t) index].repeats = (int) repeatsSlider.getValue();
            owner.edited();
            repaint();
            owner.repaint (owner.mapRect.getUnion (owner.infoRect));
        };
        addAndMakeVisible (repeatsSlider);

        const char* names[] = { "<", ">", "Dup", "x" };
        const char* tips[] = { "Move this block earlier", "Move this block later", "Duplicate this block (inserted after it)", "Remove this block" };
        for (int i = 0; i < 4; ++i)
        {
            btn[i].setButtonText (names[i]);
            btn[i].setTooltip (tips[i]);
            addAndMakeVisible (btn[i]);
        }
        btn[0].onClick = [this] { later ([] (SongPanel& p, int i) { p.moveBlock (i, -1); }); };
        btn[1].onClick = [this] { later ([] (SongPanel& p, int i) { p.moveBlock (i, +1); }); };
        btn[2].onClick = [this] { later ([] (SongPanel& p, int i) { p.duplicateBlock (i); }); };
        btn[3].onClick = [this] { later ([] (SongPanel& p, int i) { p.removeBlock (i); }); };
    }

    void setPlaying (bool on, double fraction)
    {
        if (on == playing && std::abs (fraction - progress) < 0.004)
            return;
        playing = on;
        progress = fraction;
        repaint();
    }

    void setSelected (bool on)
    {
        if (on != selected)
        {
            selected = on;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const float s = juce::jmax (0.5f, (float) getHeight() / 160.0f);       // la scheda si scala con la sua altezza (le schede si rimpiccioliscono se sono tante)
        const auto b = getLocalBounds().toFloat().reduced (1.0f);
        const auto& blk = owner.song().blocks[(size_t) index];
        const auto col = SongPanel::patternColour (blk.pattern);

        g.setColour (theme::panelHi);
        g.fillRoundedRectangle (b, 9.0f * s);
        g.setColour (col.withAlpha (0.9f));
        g.fillRoundedRectangle (b.withHeight (5.0f * s), 3.0f * s);

        if (playing)
        {
            g.setColour (theme::accent.withAlpha (0.22f));
            g.fillRoundedRectangle (b.withWidth (b.getWidth() * (float) progress), 9.0f * s);
        }
        g.setColour (playing ? theme::accent : (selected ? theme::text : theme::border));
        g.drawRoundedRectangle (b, 9.0f * s, playing || selected ? 2.0f : 1.0f);

        const double beats = sl::Song::blockBeats (owner.processor.getKit().patterns, blk);
        const double bars = beats / 4.0;
        g.setColour (selected ? theme::accentSoft : theme::text);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
        g.drawText ((selected ? juce::String ("> ") : juce::String()) + "#" + juce::String (index + 1) + (selected ? "  Play from here" : juce::String()), titleRect.withTrimmedRight ((int) (58 * s)), juce::Justification::centredLeft);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s), juce::Font::bold));
        g.drawText (juce::String (bars, bars == std::floor (bars) ? 0 : 2) + (bars == 1.0 ? " bar" : " bars"), titleRect, juce::Justification::centredRight);
    }

    void resized() override
    {
        const float s = juce::jmax (0.5f, (float) getHeight() / 160.0f);       // la scheda si scala con la sua altezza (le schede si rimpiccioliscono se sono tante)
        auto r = getLocalBounds().reduced ((int) (9 * s), (int) (10 * s));
        const int gap = (int) (6 * s);
        titleRect = r.removeFromTop ((int) (20 * s));
        r.removeFromTop (gap);
        patternBox.setBounds (r.removeFromTop ((int) (30 * s)));
        r.removeFromTop (gap);
        repeatsSlider.setBounds (r.removeFromTop ((int) (30 * s)));
        r.removeFromTop (gap + gap / 2);
        const int bw = (r.getWidth() - 3 * gap) / 4;
        for (int i = 0; i < 4; ++i)
        {
            btn[i].setBounds (r.removeFromLeft (bw).withHeight (juce::jmin (r.getHeight(), (int) (28 * s))));
            r.removeFromLeft (gap);
        }
    }

    void mouseDown (const juce::MouseEvent&) override;

    int getIndex() const { return index; }

private:
    void later (void (*fn) (SongPanel&, int))
    {
        juce::MessageManager::callAsync ([o = juce::Component::SafePointer<SongPanel> (&owner), i = index, fn] { if (o != nullptr) fn (*o, i); });
    }

    SongPanel& owner;
    int index;
    juce::ComboBox patternBox;
    juce::Slider repeatsSlider;
    juce::TextButton btn[4];
    juce::Rectangle<int> titleRect;
    bool playing = false, selected = false;
    double progress = 0.0;
};

// ------------------------------------------------------------------------ Striscia con i blocchi

class SongPanel::Strip final : public juce::Component
{
public:
    explicit Strip (SongPanel& o) : owner (o) {}

    void rebuild()
    {
        cards.clear();
        for (int i = 0; i < (int) owner.song().blocks.size(); ++i)
        {
            auto c = std::make_unique<BlockCard> (owner, i);
            addAndMakeVisible (*c);
            cards.push_back (std::move (c));
        }
        refreshSelection();
    }

    int numCards() const { return (int) cards.size(); }

    // Le schede vanno a capo: quante ne stanno per riga dipende dalla larghezza. Se le righe non stanno nell'altezza disponibile le schede
    // si rimpiccioliscono (fino a un minimo leggibile); oltre, il contenitore scorre in verticale. Ritorna l'altezza necessaria.
    int arrange (int width, int availHeight, float s)
    {
        const int n = (int) cards.size();
        const float kMin = 0.72f;
        int cols = 1, rows = 1, cw = 0, ch = 0, gap = 0, need = 0;
        int w = width;
        float k = 1.0f;
        for (; k >= kMin - 1.0e-4f; k -= 0.04f)
        {
            cw = (int) (176 * s * k);
            ch = (int) (160 * s * k);
            gap = juce::jmax (4, (int) (10 * s * k));
            cols = juce::jmax (1, (w + gap) / (cw + gap));
            rows = juce::jmax (1, (n + cols - 1) / cols);
            need = rows * (ch + gap) - gap;
            if (need <= availHeight)
                break;
        }
        if (need > availHeight)                          // neanche alla dimensione minima: scorrimento verticale (si lascia posto alla barra)
        {
            w = juce::jmax (1, width - (int) (14 * s));
            cols = juce::jmax (1, (w + gap) / (cw + gap));
            rows = juce::jmax (1, (n + cols - 1) / cols);
            need = rows * (ch + gap) - gap;
        }
        for (int i = 0; i < n; ++i)
            cards[(size_t) i]->setBounds ((i % cols) * (cw + gap), (i / cols) * (ch + gap), cw, ch);
        setSize (w, juce::jmax (1, need));
        return need;
    }

    void refreshSelection()
    {
        for (auto& c : cards)
            c->setSelected (c->getIndex() == owner.selected);
    }

    void setPlaying (int block, double fraction)
    {
        for (auto& c : cards)
            c->setPlaying (c->getIndex() == block, c->getIndex() == block ? fraction : 0.0);
    }


private:
    SongPanel& owner;
    std::vector<std::unique_ptr<BlockCard>> cards;
};

void SongPanel::BlockCard::mouseDown (const juce::MouseEvent&)
{
    owner.selected = owner.selected == index ? -1 : index;         // un secondo clic toglie la selezione (Play dall'inizio)
    owner.strip->refreshSelection();
    owner.updateStart();
}

// ------------------------------------------------------------------------ Scheda

SongPanel::SongPanel (SampleLabProcessor& p) : processor (p)
{
    strip = std::make_unique<Strip> (*this);
    viewport.setViewedComponent (strip.get(), false);
    viewport.setScrollBarsShown (true, false);            // solo scorrimento verticale: i blocchi vanno a capo
    addAndMakeVisible (viewport);

    for (juce::Button* b : std::initializer_list<juce::Button*> { &loopButton, &addButton, &moreButton, &openButton,
                                                                  &saveButton, &midiButton, &wavButton, &stemsButton })
        addAndMakeVisible (*b);

    loopButton.setClickingTogglesState (true);
    loopButton.setToggleState (song().loop, juce::dontSendNotification);
    loopButton.setTooltip ("Start again from the first block when the song ends (otherwise Play stops at the end)");
    loopButton.onClick = [this] { song().loop = loopButton.getToggleState(); edited(); };

    addButton.setTooltip ("Add a block at the end of the song: choose the pattern");
    addButton.onClick = [this] { showAddMenu(); };
    moreButton.setTooltip ("Song actions: clear, fill with the patterns that have notes");
    moreButton.onClick = [this] { showMoreMenu(); };
    openButton.setTooltip ("Load a chain of patterns from a .slsong file (the patterns themselves stay in the kit)");
    openButton.onClick = [this] { openSong(); };
    saveButton.setTooltip ("Save this chain of patterns as a .slsong file. The song is also saved inside the kit (Save).");
    saveButton.onClick = [this] { saveSong(); };

    nameBox.setText (song().name, juce::dontSendNotification);
    nameBox.setTooltip ("Song name (used for the file names of the exports)");
    nameBox.setSelectAllWhenFocused (true);
    nameBox.onTextChange = [this] { song().name = nameBox.getText().isEmpty() ? juce::String ("Song") : nameBox.getText(); processor.songEdited(); };
    addAndMakeVisible (nameBox);

    tailBox.addItem ("Tail: cut", 1);
    tailBox.addItem ("Tail: loop", 2);
    tailBox.addItem ("Tail: keep", 3);
    tailBox.setSelectedId (juce::jlimit (0, 2, (int) processor.getExportTail()) + 1, juce::dontSendNotification);
    tailBox.setTooltip ("What to do with delay/reverb tails in audio exports (same setting as in the Seq tab)");
    tailBox.onChange = [this] { processor.setExportTail ((SampleLabProcessor::ExportTail) (tailBox.getSelectedId() - 1)); };
    addAndMakeVisible (tailBox);

    midiButton.setTooltip ("The whole song as one MIDI clip (effects and reverse are not part of MIDI: use Bounce WAV or Export lanes)");
    midiButton.onClick = [this] { exportMidi(); };
    wavButton.setTooltip ("The whole song as ONE audio file: cell effects, step locks, tempo-synced delay and master effects included");
    wavButton.onClick = [this] { bounceWav(); };
    stemsButton.setTooltip ("One WAV per cell used in the song (same length, so they line up), with the cell's effects but without the master effects");
    stemsButton.onClick = [this] { exportStems(); };

    dragMidi = std::make_unique<DragHandle> ("Drag MIDI", "Drag this onto a MIDI or instrument track in your DAW: the whole song as a MIDI clip",
        [this] () -> juce::StringArray
        {
            const auto dir = dragFolder();
            dir.deleteRecursively();
            dir.createDirectory();
            const auto file = dir.getChildFile (baseName() + ".mid");
            juce::String err;
            return processor.exportSongMidi (file, err) ? juce::StringArray (file.getFullPathName()) : juce::StringArray();
        });
    dragAudio = std::make_unique<DragHandle> ("Drag audio", "Drag this onto an audio track: the whole song as one file, with all effects",
        [this] () -> juce::StringArray
        {
            const auto dir = dragFolder();
            dir.deleteRecursively();
            dir.createDirectory();
            const auto file = dir.getChildFile (baseName() + ".wav");
            juce::String err;
            return processor.bounceSongWav (file, err) ? juce::StringArray (file.getFullPathName()) : juce::StringArray();
        });
    dragLanes = std::make_unique<DragHandle> ("Drag lanes", "Drag this into the DAW: one audio file per cell used in the song (same length)",
        [this] () -> juce::StringArray
        {
            const auto dir = dragFolder().getChildFile (baseName() + "_lanes");
            dir.deleteRecursively();
            juce::String err;
            if (processor.exportSongStems (dir, err) <= 0)
                return {};
            juce::StringArray files;
            for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.wav"))
                files.add (f.getFullPathName());
            return files;
        });
    addAndMakeVisible (*dragMidi);
    addAndMakeVisible (*dragAudio);
    addAndMakeVisible (*dragLanes);

    refresh();
    startTimerHz (30);
}

SongPanel::~SongPanel() { stopTimer(); }

void SongPanel::refresh()
{
    if (selected >= (int) song().blocks.size())
        selected = -1;
    updateStart();
    loopButton.setToggleState (song().loop, juce::dontSendNotification);
    if (! nameBox.hasKeyboardFocus (true))
        nameBox.setText (song().name, juce::dontSendNotification);
    tailBox.setSelectedId (juce::jlimit (0, 2, (int) processor.getExportTail()) + 1, juce::dontSendNotification);
    strip->rebuild();
    relayoutStrip();
    repaint();
}

void SongPanel::relayoutStrip()
{
    const float s = theme::scaleFor (*this);
    strip->arrange (viewport.getWidth(), viewport.getHeight(), s);
}

void SongPanel::updateStart()
{
    processor.setSongStartBeats (selected >= 0 && selected < (int) song().blocks.size() ? song().blockStart (processor.getKit().patterns, selected) : 0.0);
    repaint (mapRect);
}

void SongPanel::edited()
{
    processor.songEdited();
    updateStart();
    repaint (mapRect.getUnion (infoRect));
}

void SongPanel::timerCallback()
{
    int pb = -1;
    double frac = 0.0, pos = 0.0;
    if (processor.isSongMode() && processor.isSequencerPlaying() && ! song().empty())
    {
        const auto& pats = processor.getKit().patterns;
        const double total = processor.songTotalBeats();
        if (total > 0.0)
        {
            const double ppq = processor.getSequencerPpq();
            pos = song().loop ? std::fmod (ppq, total) : ppq;
            pb = song().blockAt (pats, pos);
            if (pb >= 0)
            {
                const double start = song().blockStart (pats, pb);
                const double len = sl::Song::blockBeats (pats, song().blocks[(size_t) pb]);
                frac = len > 0.0 ? juce::jlimit (0.0, 1.0, (pos - start) / len) : 0.0;
            }
        }
    }
    if (pb != playingBlock || std::abs (pos - playPos) > 0.01)
    {
        playingBlock = pb;
        playFraction = frac;
        playPos = pos;
        strip->setPlaying (pb, frac);
        repaint (mapRect);
    }

    // Stato che puo' cambiare da fuori (annulla, apertura di un kit): si riallinea
    if (strip->numCards() != (int) song().blocks.size())
        refresh();
    if (loopButton.getToggleState() != song().loop)
        loopButton.setToggleState (song().loop, juce::dontSendNotification);
}

juce::String SongPanel::lengthText() const
{
    const double beats = processor.songTotalBeats();
    const double bpm = juce::jmax (1.0, processor.getCurrentBpm());
    const double secs = beats * 60.0 / bpm;
    return "Length: " + juce::String (beats / 4.0, 1) + " bars  -  " + juce::String ((int) secs / 60) + ":"
           + juce::String ((int) secs % 60).paddedLeft ('0', 2) + " at " + juce::String (juce::roundToInt (bpm)) + " BPM  -  "
           + juce::String ((int) processor.song().blocks.size()) + " block(s)";
}

void SongPanel::addBlock (int pattern)
{
    if ((int) song().blocks.size() >= sl::Song::kMaxBlocks)
        return;
    song().blocks.push_back ({ juce::jlimit (0, sl::kNumPatterns - 1, pattern), 1 });
    selected = (int) song().blocks.size() - 1;
    edited();
    refresh();
    viewport.setViewPosition (0, strip->getHeight());         // porta in vista l'ultimo blocco aggiunto (in fondo)
}

void SongPanel::showAddMenu()
{
    juce::PopupMenu m;
    const auto& kit = processor.getKit();
    m.addSectionHeader ("Add a block with the pattern...");
    for (int i = 0; i < sl::kNumPatterns; ++i)
    {
        const bool has = ! kit.patterns[(size_t) i].lanes.empty();
        m.addItem (1 + i, juce::String (i + 1) + "  " + kit.patterns[(size_t) i].name + (has ? "" : "   (empty)"), true, i == kit.activePattern);
    }
    juce::Component::SafePointer<SongPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addButton), [safe] (int r)
    {
        if (safe != nullptr && r > 0)
            safe->addBlock (r - 1);
    });
}

void SongPanel::showMoreMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Add every pattern that has notes (one block each)");
    m.addItem (2, "Clear the song", ! song().empty());
    juce::Component::SafePointer<SongPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (moreButton), [safe] (int r)
    {
        if (safe == nullptr || r == 0)
            return;
        auto& s = safe->song();
        if (r == 1)
        {
            for (int i = 0; i < sl::kNumPatterns && (int) s.blocks.size() < sl::Song::kMaxBlocks; ++i)
                if (! safe->processor.getKit().patterns[(size_t) i].lanes.empty())
                    s.blocks.push_back ({ i, 1 });
        }
        else if (r == 2)
            s.blocks.clear();
        safe->selected = -1;
        safe->edited();
        safe->refresh();
    });
}

void SongPanel::moveBlock (int index, int delta)
{
    auto& b = song().blocks;
    const int to = index + delta;
    if (index < 0 || index >= (int) b.size() || to < 0 || to >= (int) b.size())
        return;
    std::swap (b[(size_t) index], b[(size_t) to]);
    selected = to;
    edited();
    refresh();
}

void SongPanel::duplicateBlock (int index)
{
    auto& b = song().blocks;
    if (index < 0 || index >= (int) b.size() || (int) b.size() >= sl::Song::kMaxBlocks)
        return;
    b.insert (b.begin() + index + 1, b[(size_t) index]);
    selected = index + 1;
    edited();
    refresh();
}

void SongPanel::removeBlock (int index)
{
    auto& b = song().blocks;
    if (index < 0 || index >= (int) b.size())
        return;
    b.erase (b.begin() + index);
    selected = -1;
    edited();
    refresh();
}

// ------------------------------------------------------------------------ File ed esportazione

void SongPanel::showMessage (const juce::String& text)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "NINA", text);
}

juce::File SongPanel::dragFolder() const
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("SampleLab_drag");
}

juce::String SongPanel::baseName() const
{
    return sl::KitIO::sanitiseName (processor.getKit().name + "_" + processor.song().name + "_" + juce::String (juce::roundToInt (processor.getCurrentBpm()))
                                    + "bpm");
}

juce::File SongPanel::defaultFile (const juce::String& folder, const juce::String& extension) const
{
    const auto& kit = processor.getKit();
    const auto dir = kit.file.getFullPathName().isNotEmpty() ? kit.file.getSiblingFile (folder)
                                                             : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    dir.createDirectory();
    return dir.getChildFile (baseName() + extension);
}

void SongPanel::saveSong()
{
    const auto suggested = processor.getKit().file.getFullPathName().isNotEmpty()
                               ? defaultFile ("Songs", ".slsong").getSiblingFile (sl::KitIO::sanitiseName (processor.getKit().name + "_" + song().name) + ".slsong")
                               : defaultFile ("Songs", ".slsong");
    chooser = std::make_unique<juce::FileChooser> ("Save song", suggested, "*.slsong");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              if (! f.hasFileExtension ("slsong"))
                                  f = f.withFileExtension ("slsong");
                              juce::String err;
                              if (processor.saveSongFile (f, err))
                                  showMessage ("Song saved:\n" + f.getFullPathName() + "\n\nIt holds the chain of blocks. The patterns are in the kit: "
                                               "save the kit too (Save) to keep them.");
                              else
                                  showMessage (err);
                          });
}

void SongPanel::openSong()
{
    chooser = std::make_unique<juce::FileChooser> ("Open song", defaultFile ("Songs", ".slsong").getParentDirectory(), "*.slsong");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              juce::String err;
                              if (processor.loadSongFile (f, err))
                              {
                                  selected = -1;
                                  refresh();
                              }
                              else
                                  showMessage (err);
                          });
}

void SongPanel::exportMidi()
{
    if (song().empty())
    {
        showMessage ("The song is empty: add some blocks first.");
        return;
    }
    chooser = std::make_unique<juce::FileChooser> ("Export song as MIDI", defaultFile ("Bounces", ".mid"), "*.mid");
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
                              if (processor.exportSongMidi (f, err))
                                  showMessage ("MIDI exported:\n" + f.getFullPathName()
                                               + "\n\nNote: reverse and effect locks are not part of MIDI. Use Bounce WAV for those.");
                              else
                                  showMessage (err);
                          });
}

void SongPanel::bounceWav()
{
    if (song().empty())
    {
        showMessage ("The song is empty: add some blocks first.");
        return;
    }
    chooser = std::make_unique<juce::FileChooser> ("Bounce song to audio", defaultFile ("Bounces", ".wav"), "*.wav");
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
                              if (processor.bounceSongWav (f, err))
                                  showMessage ("Audio bounced:\n" + f.getFullPathName());
                              else
                                  showMessage (err.isEmpty() ? juce::String ("Bounce failed") : err);
                          });
}

void SongPanel::exportStems()
{
    if (song().empty())
    {
        showMessage ("The song is empty: add some blocks first.");
        return;
    }
    chooser = std::make_unique<juce::FileChooser> ("Choose where to create the folder with one file per cell",
                                                   defaultFile ("Bounces", "").getParentDirectory(), "*", true);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto parent = fc.getResult();
                              if (parent == juce::File())
                                  return;
                              const auto folder = parent.getChildFile (baseName() + "_lanes");
                              juce::String err;
                              const int n = processor.exportSongStems (folder, err);
                              if (n >= 0)
                                  showMessage (juce::String (n) + " file(s) written to:\n" + folder.getFullPathName()
                                               + "\n\nAll files have the same length: place them at the start of the same bar in your DAW. "
                                                 "They include each cell's effects but not the master effects.");
                              else
                                  showMessage (err.isEmpty() ? juce::String ("Export failed") : err);
                          });
}

// ------------------------------------------------------------------------ Disegno e layout

void SongPanel::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);

    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f * s, 1.0f);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
    g.drawText ("DRAG TO DAW", row2Rect.withWidth ((int) (104 * s)).withTrimmedLeft ((int) (6 * s)), juce::Justification::centredLeft);

    g.setColour (theme::border.withAlpha (0.8f));
    for (const auto& [x, row] : separatorsX)
    {
        const int top = (row == 1 ? row2Rect : row1Rect).getY();
        const int bottom = (row == 0 ? row1Rect : row2Rect).getBottom();
        g.drawLine ((float) x, (float) top + 6.0f * s, (float) x, (float) bottom - 6.0f * s, 1.0f);
    }

    // Il Play (barra in alto, barra spaziatrice) in questa scheda suona la song
    g.setColour (theme::accentSoft);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s), juce::Font::bold));
    g.drawText ("Play here plays the song", hintRect, juce::Justification::centredLeft, true);

    // Durata (a destra dell'etichetta della mappa)
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
    g.drawText (lengthText(), infoRect, juce::Justification::centredLeft, true);

    if (song().empty())
    {
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (11.0f, 14.0f * s)));
        g.drawFittedText ("The song is empty.\nBuild patterns in the Seq tab, then click \"+ Block\" to chain them here:\n"
                          "each block plays a pattern a number of times, and the blocks play one after the other.",
                          stripRect.withTrimmedTop ((int) (20 * s)).reduced (20), juce::Justification::centred, 4);
    }

    // Mappa proporzionale della song con il cursore di riproduzione
    {
        const auto& pats = processor.getKit().patterns;
        const double total = processor.songTotalBeats();
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
        g.drawText ("PATTERN SEQUENCER  -  SONG TIMELINE", mapRect.withHeight ((int) (18 * s)).withWidth ((int) (300 * s)), juce::Justification::centredLeft);

        const auto bar = mapRect.withTrimmedTop ((int) (22 * s)).toFloat();
        g.setColour (theme::panelHi.withAlpha (0.6f));
        g.fillRoundedRectangle (bar, 8.0f * s);

        if (total > 0.0)
        {
            double t = 0.0;
            for (int i = 0; i < (int) song().blocks.size(); ++i)
            {
                const auto& blk = song().blocks[(size_t) i];
                const double len = sl::Song::blockBeats (pats, blk);
                auto r = juce::Rectangle<float> (bar.getX() + bar.getWidth() * (float) (t / total), bar.getY(),
                                                 bar.getWidth() * (float) (len / total), bar.getHeight()).reduced (1.5f, 3.0f * s);
                g.setColour (patternColour (blk.pattern).withAlpha (i == playingBlock ? 0.95f : (i == selected ? 0.75f : 0.55f)));
                g.fillRoundedRectangle (r, 5.0f * s);
                if (r.getWidth() > 34.0f * s)
                {
                    g.setColour (juce::Colour (0xff10151c));
                    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s), juce::Font::bold));
                    g.drawText (juce::String (blk.pattern + 1) + (blk.repeats > 1 ? " x" + juce::String (blk.repeats) : juce::String()),
                                r.reduced (5.0f * s, 0.0f), juce::Justification::centredLeft, true);
                }
                t += len;
            }
            if (playingBlock >= 0)
            {
                const float x = bar.getX() + bar.getWidth() * (float) (playPos / total);
                g.setColour (theme::text);
                g.drawLine (x, bar.getY() + 1.0f, x, bar.getBottom() - 1.0f, 2.0f);
            }
        }
    }
}

void SongPanel::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (10 * s));
    const int rowH1 = (int) (34 * s), rowH2 = (int) (32 * s), vgap = (int) (8 * s);
    const int gGap = (int) (6 * s), sGap = (int) (18 * s);
    separatorsX.clear();

    row1Rect = area.removeFromTop (rowH1);
    area.removeFromTop (vgap);
    row2Rect = area.removeFromTop (rowH2);
    area.removeFromTop (vgap * 2);

    auto groupEnd = [&] (juce::Rectangle<int>& r) { separatorsX.push_back ({ r.getX() + sGap / 2, 0 }); r.removeFromLeft (sGap); };

    auto r1 = row1Rect;
    loopButton.setBounds (r1.removeFromLeft ((int) (72 * s)));
    groupEnd (r1);
    addButton.setBounds (r1.removeFromLeft ((int) (96 * s)));
    r1.removeFromLeft (gGap / 2);
    moreButton.setBounds (r1.removeFromLeft ((int) (34 * s)));
    groupEnd (r1);
    tailBox.setBounds (r1.removeFromLeft ((int) (112 * s)));
    r1.removeFromLeft (gGap);
    midiButton.setBounds (r1.removeFromLeft ((int) (128 * s)));
    r1.removeFromLeft (gGap);
    wavButton.setBounds (r1.removeFromLeft ((int) (128 * s)));
    r1.removeFromLeft (gGap);
    stemsButton.setBounds (r1.removeFromLeft ((int) (128 * s)));
    hintRect = r1.withTrimmedLeft (sGap);

    {
        auto d = row2Rect;
        d.removeFromLeft ((int) (112 * s));
        separatorsX.push_back ({ d.getX() + gGap / 2, 1 });
        d.removeFromLeft (gGap * 2);
        const int w = (int) (128 * s);
        dragMidi->setBounds (d.removeFromLeft (w));
        d.removeFromLeft (gGap);
        dragAudio->setBounds (d.removeFromLeft (w));
        d.removeFromLeft (gGap);
        dragLanes->setBounds (d.removeFromLeft (w));
        d.removeFromLeft (gGap);
        separatorsX.push_back ({ d.getX() + gGap / 2, 1 });
        d.removeFromLeft (gGap * 2);
        nameBox.setBounds (d.removeFromLeft ((int) (170 * s)));
        d.removeFromLeft (gGap);
        openButton.setBounds (d.removeFromLeft ((int) (120 * s)));
        d.removeFromLeft (gGap);
        saveButton.setBounds (d.removeFromLeft ((int) (120 * s)));
    }

    // Mappa proporzionale della song in basso; i blocchi occupano lo spazio sopra e vanno a capo (rimpiccioliti se sono tanti)
    mapRect = area.removeFromBottom ((int) (84 * s));
    infoRect = mapRect.withHeight ((int) (18 * s)).withTrimmedLeft ((int) (310 * s));
    area.removeFromBottom (vgap * 2);
    stripRect = area;
    viewport.setBounds (area);
    relayoutStrip();
}
