#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
// Diario minimo dell'interfaccia (dimensioni, scala, apertura nella DAW): serve a capire problemi di layout all'apertura.
void uiLog (const juce::String& text)
{
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("SampleLab").getChildFile ("ui.log");
    f.getParentDirectory().createDirectory();
    if (f.getSize() > 100000)
        f.deleteFile();
    f.appendText (juce::Time::getCurrentTime().toString (true, true, true, true) + "  " + text + "\n");
}

// Rifa il layout di un componente e di tutti i suoi figli (anche se la loro dimensione non e' cambiata)
void relayoutAll (juce::Component& c)
{
    c.resized();
    for (auto* child : c.getChildren())
        relayoutAll (*child);
}
}

SampleLabEditor::SampleLabEditor (SampleLabProcessor& p)
    : AudioProcessorEditor (&p), processor (p), grid (p), waveStrip (p), cellPanel (p), modules (p), fxPanel (p), seqPanel (p), slicePanel (p), songPanel (p)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);

    setResizable (true, true);
    setResizeLimits (1000, 640, 3840, 2160);

    kitLabel.setJustificationType (juce::Justification::centred);
    kitLabel.setColour (juce::Label::backgroundColourId, theme::panelHi);
    kitLabel.setColour (juce::Label::outlineColourId, theme::border);
    kitLabel.setColour (juce::Label::textColourId, theme::text);
    kitLabel.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    kitLabel.setTooltip ("Kit name: it becomes the name of the saved .slkit file");
    kitMenuButton.setTooltip ("Kit menu: new, open, save, recent kits, normalize");
    kitMenuButton.onClick = [this] { showKitMenu(); };
    prevKitButton.setTooltip ("Previous kit in the same folder");
    nextKitButton.setTooltip ("Next kit in the same folder");
    prevKitButton.onClick = [this] { confirmDiscard ([this] { doOpenAdjacent (-1); }); };
    nextKitButton.onClick = [this] { confirmDiscard ([this] { doOpenAdjacent (+1); }); };
    saveButton.onClick = [this] { doSave(); };
    sizeButton.onClick = [this] { showSizeMenu(); };
    for (auto* b : { &kitMenuButton, &prevKitButton, &nextKitButton, &saveButton, &sizeButton })
        addAndMakeVisible (b);
    addAndMakeVisible (kitLabel);

    transportButton.onClick = [this] { toggleTransport(); };
    addAndMakeVisible (transportButton);

    bpmLabel.setJustificationType (juce::Justification::centred);
    bpmLabel.setColour (juce::Label::backgroundColourId, theme::panelHi);
    bpmLabel.setColour (juce::Label::outlineColourId, theme::border);
    bpmLabel.setColour (juce::Label::textColourId, theme::accentSoft);
    bpmLabel.setColour (juce::TextEditor::backgroundColourId, theme::panelHi);
    bpmLabel.onTextChange = [this]
    {
        processor.setInternalBpm (bpmLabel.getText().retainCharacters ("0123456789.").getDoubleValue());
    };
    addAndMakeVisible (bpmLabel);

    addAndMakeVisible (browser);
    addAndMakeVisible (grid);
    addAndMakeVisible (waveStrip);
    addAndMakeVisible (cellPanel);
    addAndMakeVisible (modules);
    addChildComponent (fxPanel);
    addChildComponent (seqPanel);
    addChildComponent (slicePanel);
    addChildComponent (songPanel);
    slicePanel.onKitChanged = [this] { refreshAll(); };

    browser.onFileActivated = [this] (const juce::File& f)
    {
        if (currentTab == 4)
            slicePanel.loadFile (f);                     // scheda Slice: il file diventa la sorgente
        else
            loadIntoCell (grid.getSelected(), f);
    };
    slicePanel.browserFile = [this] { return browser.getSelectedFile(); };
    browser.onPreview = [this] (const juce::File& f) { processor.auditionFile (f, browser.getPreviewGainDb()); };
    browser.onPreviewStop = [this] { processor.stopAudition(); };

    grid.onSelectionChanged = [this] (int) { refreshCellViews(); };
    grid.onMultiChanged = [this] { cellPanel.repaint(); };
    cellPanel.selectedCells = [this] { return grid.selectedCells(); };
    grid.onFileDropped = [this] (int c, const juce::File& f) { loadIntoCell (c, f); };
    grid.onBrowserDrop = [this] (int c)
    {
        const auto f = browser.getSelectedFile();
        if (f.existsAsFile())
            loadIntoCell (c, f);
    };
    grid.onLoadRequested = [this] (int c) { requestLoadDialog (c); };
    grid.onKitChanged = [this]
    {
        refreshCellViews();
        seqPanel.refresh();                 // lane aggiunte/rimosse o celle svuotate
        refreshTopBar();
    };

    cellPanel.onEdited = [this]
    {
        grid.repaint();
        refreshTopBar();
    };

    cellPanel.onStep = [this] (int delta) { grid.setSelected (grid.getSelected() + delta); };
    seqPanel.onCellSelected = [this] (int cell) { grid.setSelected (cell); };     // lo step selezionato porta con se' la sua cella
    cellPanel.onLoadRequested = [this] (int c) { requestLoadDialog (c); };

    // Dimensione iniziale: quella salvata, ma mai piu' grande dello schermo (altrimenti la DAW la riduce e il layout resta a meta')
    {
        int w = processor.uiWidth, h = processor.uiHeight;
        if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        {
            w = juce::jmin (w, juce::jmax (1000, d->userArea.getWidth() - 60));
            h = juce::jmin (h, juce::jmax (640, d->userArea.getHeight() - 140));
        }
        uiLog ("editor created: requested " + juce::String (processor.uiWidth) + "x" + juce::String (processor.uiHeight)
               + " -> " + juce::String (w) + "x" + juce::String (h));
        setSize (w, h);
    }
    // Solo per prove: SAMPLELAB_TAB / SAMPLELAB_CELL aprono direttamente una scheda e selezionano una cella (schermate automatiche); SAMPLELAB_DETECT preme Detect
    if (const auto tabEnv = juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_TAB", {}); tabEnv.isNotEmpty())
    {
        if (const auto cellEnv = juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_CELL", {}); cellEnv.isNotEmpty())
            grid.setSelected (cellEnv.getIntValue());
        showTab (tabEnv.getIntValue());
        if (const auto fileEnv = juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_SLICEFILE", {}); fileEnv.isNotEmpty())
            slicePanel.loadFile (juce::File (fileEnv));
        if (juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_DETECTBPM", {}).isNotEmpty())
            slicePanel.runDetectBpm();
        if (juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_DETECT", {}).isNotEmpty())
            slicePanel.runDetect();
    }
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (processor.uiZoom * 100.0f)) + "%");
    refreshAll();

    // Solo per prove: SAMPLELAB_UNDOTEST esegue una sequenza di modifiche con annulla/ripeti e scrive l'esito in ui.log
    if (juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_UNDOTEST", {}).isNotEmpty())
    {
        juce::RangedAudioParameter* lvl = processor.getParams().cell (0).at (sl::CellF::Level);
        auto& c0 = processor.getKit().cells[0];
        processor.resetUndo();
        processor.undoTick (false);                              // stato di riferimento
        const float v0 = lvl->getValue();
        const int note0 = c0.lowNote;
        processor.noteUserInput();
        lvl->setValueNotifyingHost (0.8f);
        processor.undoTick (false);                              // modifica 1: parametro
        processor.noteUserInput();
        c0.lowNote = note0 + 5;
        processor.cellEdited (0);
        processor.undoTick (false);                              // modifica 2: tasto della cella
        const bool a1 = processor.canUndo();
        processor.undo();
        const bool a2 = c0.lowNote == note0 && std::abs (lvl->getValue() - 0.8f) < 1.0e-4f;
        processor.undo();
        const bool a3 = std::abs (lvl->getValue() - v0) < 1.0e-4f;
        processor.redo();
        const bool a4 = std::abs (lvl->getValue() - 0.8f) < 1.0e-4f && c0.lowNote == note0;
        processor.redo();
        const bool a5 = c0.lowNote == note0 + 5;
        // L'automazione (nessuna azione dell'utente) non entra nella cronologia
        processor.undoTick (false);
        const bool before = processor.canUndo();
        processor.undoTick (false);
        lvl->setValueNotifyingHost (0.3f);
        processor.undoTick (false);
        const bool a6 = processor.canUndo() == before && std::abs (lvl->getValue() - 0.3f) < 1.0e-4f;
        // Pattern: uno step acceso si annulla e si ripete
        processor.resetUndo();
        processor.undoTick (false);
        auto& pat0 = processor.activePattern();
        processor.addLaneForCell (0);
        processor.undoTick (false);
        processor.noteUserInput();
        pat0.lanes.back().steps[3].on = true;
        processor.patternEdited();
        processor.undoTick (false);
        const bool p1 = processor.activePattern().lanes.back().steps[3].on;
        processor.undo();
        const bool p2 = ! processor.activePattern().lanes.empty() ? ! processor.activePattern().lanes.back().steps[3].on : false;
        processor.redo();
        const bool p3 = processor.activePattern().lanes.back().steps[3].on;
        uiLog ("UNDOTEST canUndo=" + juce::String ((int) a1) + " step1=" + juce::String ((int) a2) + " step2=" + juce::String ((int) a3)
               + " redo1=" + juce::String ((int) a4) + " redo2=" + juce::String ((int) a5) + " automation-ignored=" + juce::String ((int) a6)
               + " pattern: on=" + juce::String ((int) p1) + " undone=" + juce::String ((int) p2) + " redone=" + juce::String ((int) p3));
    }

    // Solo per prove: SAMPLELAB_SONGTEST costruisce una song di prova, la esporta (MIDI e WAV) e scrive i risultati in ui.log
    if (juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_SONGTEST", {}).isNotEmpty())
    {
        auto& kit = processor.getKit();
        for (int pi = 0; pi < 2; ++pi)
        {
            sl::Lane l;
            l.cell = 0;
            l.division = 10;
            l.length = 16;
            l.steps[(size_t) (pi * 4)].on = true;               // pattern 0: step 0; pattern 1: step 4
            kit.patterns[(size_t) pi].lanes.clear();
            kit.patterns[(size_t) pi].lanes.push_back (l);
        }
        kit.song.blocks = { { 0, 2 }, { 1, 1 } };               // 2 giri + 1 giro = 12 quarti (3 battute)
        if (const auto mode = juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_SONGTEST", {}); mode.startsWith ("many"))
            for (int i = 0; i < (mode == "many60" ? 58 : 22); ++i)
                kit.song.blocks.push_back ({ i % 2, 1 + i % 3 });
        processor.songEdited();
        juce::String err;
        const auto mid = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("songtest.mid");
        const bool okMidi = processor.exportSongMidi (mid, err);
        int notes = 0;
        double lastOn = 0.0;
        if (okMidi)
        {
            juce::FileInputStream in (mid);
            juce::MidiFile mf;
            mf.readFrom (in);
            mf.convertTimestampTicksToSeconds();
            if (auto* t1 = mf.getTrack (1))
                for (int i = 0; i < t1->getNumEvents(); ++i)
                    if (t1->getEventPointer (i)->message.isNoteOn())
                    {
                        ++notes;
                        lastOn = t1->getEventPointer (i)->message.getTimeStamp();
                    }
        }
        const auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("songtest.wav");
        const bool okWav = processor.bounceSongWav (wav, err);
        uiLog ("SONGTEST midi=" + juce::String ((int) okMidi) + " notes=" + juce::String (notes) + " lastOn=" + juce::String (lastOn, 3)
               + " wav=" + juce::String ((int) okWav) + " wavBytes=" + juce::String (wav.getSize()) + " total=" + juce::String (processor.songTotalBeats(), 2)
               + " err=" + err);
        songPanel.refresh();
        if (juce::SystemStats::getEnvironmentVariable ("SAMPLELAB_SONGTEST", {}) == "play")
        {
            processor.setSongMode (true);
            processor.setSequencerRunning (true);
            juce::Timer::callAfterDelay (2500, [this] { uiLog ("SONGPLAY t=2.5s running=" + juce::String ((int) processor.isSequencerRunning())
                                                                + " playing=" + juce::String ((int) processor.isSequencerPlaying())
                                                                + " ppq=" + juce::String (processor.getSequencerPpq(), 2)); });
            juce::Timer::callAfterDelay (8000, [this] { uiLog ("SONGPLAY t=8s running=" + juce::String ((int) processor.isSequencerRunning())
                                                                + " playing=" + juce::String ((int) processor.isSequencerPlaying())); });
        }
    }

    juce::Desktop::getInstance().addGlobalMouseListener (&inputWatcher);
    setWantsKeyboardFocus (true);
    theme::noButtonFocus (*this);
    startTimerHz (10);
}

void SampleLabEditor::toggleTransport()
{
    if (processor.isHostPlaying())
        return;                                         // la DAW sta suonando: comanda il suo trasporto
    processor.setSequencerRunning (! processor.isSequencerRunning());
}

void SampleLabEditor::doUndoRedo (bool redo)
{
    processor.noteUserInput();
    processor.undoTick (false);
    const double gt = redo ? processor.redoTime() : processor.undoTime();
    const double lt = currentTab == 4 ? (redo ? slicePanel.localRedoTime() : slicePanel.localUndoTime()) : 0.0;
    bool done = false;
    if (lt > 0.0 && lt >= gt)                                  // l'ultima cosa fatta e' stata nella sorgente File della scheda Slice
        done = redo ? slicePanel.localRedo() : slicePanel.localUndo();
    else if (gt > 0.0)
        done = redo ? processor.redo() : processor.undo();
    if (done)
    {
        slicePanel.reloadFromKit();
        refreshAll();
    }
}

bool SampleLabEditor::keyPressed (const juce::KeyPress& key)
{
    processor.noteUserInput();

    // Annulla / ripeti (Main, FX, Slice): Ctrl+Z, Ctrl+Y o Ctrl+Maiusc+Z
    if ((currentTab == 0 || currentTab == 1 || currentTab == 3 || currentTab == 4 || currentTab == 5) && key.getModifiers().isCommandDown())
    {
        const int k = key.getKeyCode();
        if (k == 'Z' || k == 'z')
        {
            doUndoRedo (key.getModifiers().isShiftDown());
            return true;
        }
        if (k == 'Y' || k == 'y')
        {
            doUndoRedo (true);
            return true;
        }
    }

    // Scheda Seq: Ctrl+C / Ctrl+V copiano e incollano lo step selezionato
    if (currentTab == 1 && key.getModifiers().isCommandDown())
    {
        const int k = key.getKeyCode();
        if (k == 'C' || k == 'c') { seqPanel.copySelection(); return true; }
        if (k == 'V' || k == 'v') { seqPanel.pasteSelection(); return true; }
    }

    // Scheda Slice: Spazio suona/ferma la zona (non il sequencer), Esc ferma qualsiasi ascolto
    if (currentTab == 4)
    {
        if (key == juce::KeyPress::spaceKey)
        {
            slicePanel.togglePlay();
            return true;
        }
        if (key == juce::KeyPress::escapeKey && slicePanel.isPlaying())
        {
            slicePanel.stopAll();
            return true;
        }
    }

    if (key == juce::KeyPress::spaceKey)
    {
        toggleTransport();
        return true;
    }

    // Scheda Slice: M mette un marcatore durante il Chop
    if (currentTab == 4 && slicePanel.isChopping() && (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M'))
    {
        slicePanel.chopMark();
        return true;
    }

    // Griglia delle celle (scheda Main): Canc/Backspace svuota le celle selezionate, Ctrl+A le seleziona tutte, Esc deseleziona
    if (currentTab == 0)
    {
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            grid.clearSelectedCells();
            return true;
        }
        if (key == juce::KeyPress ('a', juce::ModifierKeys::commandModifier, 0))
        {
            grid.selectAll();
            return true;
        }
        if (key == juce::KeyPress::escapeKey)
        {
            grid.clearMultiSelection();
            return true;
        }
    }
    return false;
}

// Il nome del kit mostra l'asterisco anche quando un parametro cambia dall'automazione o dal MIDI
void SampleLabEditor::timerCallback()
{
    // Subito dopo l'aggancio alla finestra della DAW si rifa piu' volte il layout di tutti i componenti (a 0.1, 0.3, 1 e 3 s):
    // se la DAW assegna dimensione/scala in ritardo, nessun componente resta con una disposizione calcolata per valori provvisori
    if (kickTick < 40)
    {
        ++kickTick;
        if (kickTick == 1 || kickTick == 3 || kickTick == 10 || kickTick == 30)
        {
            relayoutAll (*this);
            repaint();
            uiLog ("relayout tick " + juce::String (kickTick) + ": " + juce::String (getWidth()) + "x" + juce::String (getHeight()));
        }
    }

    processor.pollMidiLearn();      // completa l'assegnazione se e' arrivato un CC durante il MIDI learn
    processor.undoTick (juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown());
    if (dynamic_cast<juce::TextEditor*> (juce::Component::getCurrentlyFocusedComponent()) != nullptr)
        processor.noteUserInput();      // si sta scrivendo un valore (campo di una manopola): conta come azione dell'utente
    refreshTopBar();

    transportButton.setButtonText (processor.isSequencerPlaying() ? "Stop" : "Play");
    transportButton.setToggleState (processor.isSequencerPlaying(), juce::dontSendNotification);
    transportButton.setEnabled (! processor.isHostPlaying());
    transportButton.setTooltip (processor.isHostPlaying() ? "The host transport is running: the sequencer follows it"
                                                          : "Start/stop the sequencer (Space)");

    // BPM: dalla DAW (non modificabile) oppure interno (si modifica con doppio clic)
    const bool host = processor.isHostTempo();
    bpmLabel.setEditable (! host, ! host, false);
    bpmLabel.setTooltip (host ? "Tempo from the host" : "Internal tempo (double-click to edit)");
    if (! bpmLabel.isBeingEdited())
        bpmLabel.setText (juce::String (processor.getCurrentBpm(), 1) + " BPM", juce::dontSendNotification);
}

SampleLabEditor::~SampleLabEditor()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (&inputWatcher);
    stopTimer();
    if (&juce::LookAndFeel::getDefaultLookAndFeel() == &lnf)
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

// ----------------------------------------------------------------- Dimensioni

void SampleLabEditor::showSizeMenu()
{
    // Fallback ridimensionamento a percentuali (UI-12b)
    juce::PopupMenu menu;
    const float zooms[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    for (int i = 0; i < 5; ++i)
        menu.addItem (i + 1, juce::String (juce::roundToInt (zooms[i] * 100.0f)) + "%",
                      true, std::abs (processor.uiZoom - zooms[i]) < 0.01f);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (sizeButton),
                        [this, zooms] (int result)
                        {
                            if (result > 0)
                                applyScale (zooms[result - 1]);
                        });
}

void SampleLabEditor::applyScale (float zoom)
{
    if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (getScreenBounds()))
    {
        const auto area = display->userArea;
        zoom = juce::jmin (zoom, (float) area.getWidth() / (float) baseWidth,
                                 (float) area.getHeight() / (float) baseHeight);
    }

    processor.uiZoom = zoom;
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (zoom * 100.0f)) + "%");
    setSize (juce::roundToInt (baseWidth * zoom), juce::roundToInt (baseHeight * zoom));
}

// ------------------------------------------------------------------------ Kit

void SampleLabEditor::refreshTopBar()
{
    const auto& kit = processor.getKit();
    kitLabel.setText (kit.name + (processor.isDirty() ? " *" : ""), juce::dontSendNotification);

    // Frecce: attive solo se nella cartella del kit corrente ce n'e' un altro
    if (kit.file != lastKitFile)
    {
        lastKitFile = kit.file;
        neighbourKits.clear();
        if (kit.file.existsAsFile())
        {
            neighbourKits = kit.file.getParentDirectory().findChildFiles (juce::File::findFiles, false, "*.slkit");
            neighbourKits.sort();
        }
    }
    const bool canStep = neighbourKits.size() > 1;
    prevKitButton.setEnabled (canStep);
    nextKitButton.setEnabled (canStep);
}

void SampleLabEditor::refreshCellViews()
{
    const int c = grid.getSelected();
    waveStrip.setCell (c);
    cellPanel.setCell (c);
    modules.setCell (c);
    fxPanel.setCell (c);
    if (c != sliceCell)
    {
        sliceCell = c;
        slicePanel.setCell (c);
    }
    else
        slicePanel.refresh();
}

// ------------------------------------------------------------------- Schede

juce::Rectangle<int> SampleLabEditor::tabRect (int index) const
{
    const float s = theme::scaleFor (*this);
    auto r = tabBar.reduced ((int) (12 * s), (int) (5 * s));
    const int tw = (int) (124 * s), gap = (int) (10 * s);
    static constexpr int pos[] = { 0, 1, 3, 4, 5, 2 };              // indice scheda > posizione nella barra (Song sta dopo Seq)
    return { r.getX() + pos[juce::jlimit (0, 5, index)] * (tw + gap), r.getY(), tw, r.getHeight() };
}

void SampleLabEditor::showTab (int tab)
{
    currentTab = tab;
    const bool seq = tab == 1, slice = tab == 4, song = tab == 5;

    // Le schede Seq e Song occupano lo spazio di griglia, forma d'onda e moduli
    browser.setVisible (! seq && ! song);
    cellPanel.setVisible (! seq && ! slice && ! song);
    grid.setVisible (! seq && ! slice && ! song);
    waveStrip.setVisible (! seq && ! slice && ! song);
    slicePanel.setVisible (slice);
    seqPanel.setVisible (seq);
    songPanel.setVisible (song);
    if (song)
        songPanel.refresh();
    modules.setVisible (tab == 0);
    if (slice)
        slicePanel.setCell (grid.getSelected());
    fxPanel.setVisible (tab == 3);
    if (seq)
        seqPanel.refresh();
    theme::noButtonFocus (*this);
    repaint (tabBar);
}

void SampleLabEditor::mouseDown (const juce::MouseEvent& e)
{
    // Schede attive: Main (0), Seq (1), FX (3), Slice (4) e Song (5)
    for (int tab : { 0, 1, 3, 4, 5 })
        if (tabRect (tab).contains (e.getPosition()))
            showTab (tab);
}

void SampleLabEditor::refreshAll()
{
    refreshTopBar();
    grid.setSelected (juce::jmin (grid.getSelected(), processor.getKit().numCells() - 1));
    refreshCellViews();
    seqPanel.refresh();
    songPanel.refresh();
    grid.repaint();
}

void SampleLabEditor::showError (const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "NINA", message);
}

void SampleLabEditor::confirmDiscard (std::function<void()> proceed)
{
    if (! processor.isDirty())
    {
        proceed();
        return;
    }

    juce::Component::SafePointer<SampleLabEditor> safe (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle ("Unsaved changes")
                                      .withMessage ("The current kit has unsaved changes. Discard them?")
                                      .withButton ("Discard")
                                      .withButton ("Cancel")
                                      .withAssociatedComponent (this),
                                  [safe, proceed] (int r)
                                  {
                                      if (r == 1 && safe != nullptr)
                                          proceed();
                                  });
}

void SampleLabEditor::showKitMenu()
{
    juce::PopupMenu m;
    const bool undoTab = currentTab == 0 || currentTab == 1 || currentTab == 3 || currentTab == 4 || currentTab == 5;
    m.addItem (30, "Undo  (Ctrl+Z)", undoTab && (processor.canUndo() || slicePanel.localUndoTime() > 0.0));
    m.addItem (31, "Redo  (Ctrl+Y)", undoTab && (processor.canRedo() || slicePanel.localRedoTime() > 0.0));
    m.addSeparator();
    m.addItem (1, "New kit");
    m.addItem (2, "Open...");
    m.addItem (3, "Save");
    m.addItem (4, "Save as...");
    m.addSeparator();
    m.addItem (6, "Normalize all cells (peak -1 dBFS)");
    m.addSeparator();
    m.addItem (7, "Outputs: one cell per output (first 15 loaded cells)");
    m.addItem (8, "Outputs: all cells to Main");
    m.addSeparator();

    const auto recents = processor.getRecentKits();
    juce::PopupMenu recent;
    for (int i = 0; i < recents.size(); ++i)
    {
        const juce::File f (recents[i]);
        recent.addItem (100 + i, f.getFileNameWithoutExtension() + (f.existsAsFile() ? "" : "  (missing)"));
    }
    if (recents.isEmpty())
        recent.addItem (-1, "(none)", false);
    else
    {
        recent.addSeparator();
        recent.addItem (5, "Clear recent");
    }
    m.addSubMenu ("Recent kits", recent);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (kitMenuButton),
                     [this, recents] (int r)
                     {
                         if (r == 30) doUndoRedo (false);
                         else if (r == 31) doUndoRedo (true);
                         else if (r == 1) confirmDiscard ([this] { doNew(); });
                         else if (r == 2) confirmDiscard ([this] { doOpen(); });
                         else if (r == 3) doSave();
                         else if (r == 4) doSaveAs();
                         else if (r == 5) processor.clearRecentKits();
                         else if (r == 6)
                         {
                             const int n = processor.normalizeAllCells();
                             refreshCellViews();
                             juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "NINA",
                                 juce::String (n) + " cell(s) normalized: each Level was set so the loudest peak reaches -1 dBFS.\n"
                                                    "The audio files are not modified; you can still adjust each Level.");
                         }
                         else if (r == 7 || r == 8)
                         {
                             // Uscite multiple: la prima cella con campione va su Out 2, la seconda su Out 3... (max 15); le altre su Main
                             int next = 1;
                             for (auto& c : processor.getKit().cells)
                                 c.output = (r == 7 && c.hasSample() && next < sl::Engine::kMaxOutputs) ? next++ : 0;
                             processor.cellEdited (0);
                             refreshCellViews();
                         }
                         else if (r >= 100 && r - 100 < recents.size())
                         {
                             const juce::File f (recents[r - 100]);
                             if (! f.existsAsFile())
                             {
                                 processor.removeRecentKit (f.getFullPathName());
                                 showError ("The kit file no longer exists:\n" + f.getFullPathName());
                             }
                             else
                                 confirmDiscard ([this, f] { doOpenFile (f); });
                         }
                     });
}

void SampleLabEditor::doNew()
{
    processor.newKit();
    refreshAll();
}

void SampleLabEditor::doOpen()
{
    auto start = processor.getKit().file.existsAsFile() ? processor.getKit().file.getParentDirectory()
                                                        : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    chooser = std::make_unique<juce::FileChooser> ("Open kit", start, "*.slkit");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  doOpenFile (f);
                          });
}

void SampleLabEditor::doOpenFile (const juce::File& f)
{
    juce::String err;
    if (! processor.openKit (f, err))
    {
        showError (err);
        return;
    }
    refreshAll();

    int missing = 0;
    for (const auto& c : processor.getKit().cells)
        if (c.missing) ++missing;
    if (missing > 0)
        showError (juce::String (missing) + " sample(s) could not be found next to the kit (marked MISSING in the grid).");
}

void SampleLabEditor::doOpenAdjacent (int direction)
{
    // Kit precedente/successivo nella stessa cartella del kit corrente
    const auto& kit = processor.getKit();
    if (! kit.file.existsAsFile())
        return;

    auto files = kit.file.getParentDirectory().findChildFiles (juce::File::findFiles, false, "*.slkit");
    files.sort();
    const int idx = files.indexOf (kit.file);
    if (idx < 0 || files.size() < 2)
        return;

    doOpenFile (files[(idx + direction + files.size()) % files.size()]);
}

void SampleLabEditor::doSave()
{
    const auto& kit = processor.getKit();
    if (! kit.file.getFullPathName().isNotEmpty())
    {
        doSaveAs();
        return;
    }

    juce::String err;
    if (! processor.saveKit (kit.file, err))
        showError (err);
    refreshAll();
}

void SampleLabEditor::doSaveAs()
{
    const auto& kit = processor.getKit();
    auto start = kit.file.getFullPathName().isNotEmpty()
                     ? kit.file
                     : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (sl::KitIO::sanitiseName (kit.name) + ".slkit");

    chooser = std::make_unique<juce::FileChooser> ("Save kit as", start, "*.slkit");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              if (! f.hasFileExtension ("slkit"))
                                  f = f.withFileExtension ("slkit");

                              juce::String err;
                              if (! processor.saveKit (f, err))
                                  showError (err);
                              refreshAll();
                          });
}

// ---------------------------------------------------------------------- Celle

void SampleLabEditor::loadIntoCell (int cell, const juce::File& file)
{
    juce::String err;
    if (! processor.loadSampleIntoCell (cell, file, err))
    {
        showError (err);
        return;
    }
    grid.repaint();
    if (cell == grid.getSelected())
        refreshCellViews();
    refreshTopBar();
}

void SampleLabEditor::requestLoadDialog (int cell)
{
    chooser = std::make_unique<juce::FileChooser> ("Load sample", juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, cell] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  loadIntoCell (cell, f);
                          });
}

// --------------------------------------------------------------------- Layout

void SampleLabEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    const float s = theme::scaleFor (*this);

    // Barra superiore con logo
    g.setColour (theme::panel);
    g.fillRect (topBar);
    g.setColour (theme::border);
    g.drawHorizontalLine (topBar.getBottom() - 1, 0.0f, (float) getWidth());

    // Logo NINA (incorporato), allineato a sinistra nella barra superiore
    {
        static const juce::Image logo = juce::ImageCache::getFromMemory (BinaryData::nina_logo_png, BinaryData::nina_logo_pngSize);
        const auto logoArea = topBar.reduced ((int) (14 * s), (int) (2 * s)).withWidth ((int) (230 * s));
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (logo, logoArea.toFloat(), juce::RectanglePlacement::xLeft | juce::RectanglePlacement::yMid);
    }

    // Barra delle schede in basso: pulsanti con icona; quello della scheda aperta e' acceso (arancione con alone)
    g.setColour (theme::panel);
    g.fillRect (tabBar);
    g.setColour (theme::border);
    g.drawHorizontalLine (tabBar.getY(), 0.0f, (float) getWidth());

    struct TabInfo { const char* name; IconButton::Icon icon; bool enabled; };
    const TabInfo tabs[] = { { "Main", IconButton::Icon::TabMain, true }, { "Seq", IconButton::Icon::TabSeq, true },
                             { "Mixer", IconButton::Icon::Mixer, false }, { "FX", IconButton::Icon::Cube, true },
                             { "Slice", IconButton::Icon::Scissors, true }, { "Song", IconButton::Icon::Layers, true } };
    for (int i = 0; i < 6; ++i)
    {
        const auto tab = tabRect (i).toFloat();
        const bool active = i == currentTab;
        const float rad = 9.0f * s;

        if (active)
        {
            for (int k = 3; k >= 1; --k)                                       // alone
            {
                g.setColour (theme::accent.withAlpha (0.07f * (float) (4 - k)));
                g.drawRoundedRectangle (tab.expanded ((float) k * 1.8f * s), rad + (float) k, 1.8f * s);
            }
            g.setGradientFill (juce::ColourGradient (theme::accent.withAlpha (0.42f), 0.0f, tab.getY(), theme::accent.withAlpha (0.16f), 0.0f, tab.getBottom(), false));
            g.fillRoundedRectangle (tab, rad);
            g.setColour (theme::accent);
            g.drawRoundedRectangle (tab.reduced (0.7f), rad, 1.8f);
        }
        else
        {
            g.setColour (theme::panelHi.withAlpha (tabs[i].enabled ? 1.0f : 0.5f));
            g.fillRoundedRectangle (tab, rad);
            g.setColour (theme::border);
            g.drawRoundedRectangle (tab.reduced (0.5f), rad, 1.0f);
        }

        const juce::Colour ink = ! tabs[i].enabled ? theme::textDim.withAlpha (0.5f) : (active ? juce::Colour (0xfffff3e8) : theme::text.withAlpha (0.92f));
        const float isz = juce::jmin (tab.getHeight() * 0.46f, 24.0f * s);
        const float textW = 44.0f * s;
        const float contentW = isz + 10.0f * s + textW;
        const float x0 = tab.getCentreX() - contentW * 0.5f;
        IconButton::drawIcon (g, tabs[i].icon, juce::Rectangle<float> (x0, tab.getCentreY() - isz * 0.5f, isz, isz),
                              active ? theme::accentSoft : ink, s);
        g.setColour (ink);
        g.setFont (juce::FontOptions (juce::jmax (10.0f, 15.0f * s), juce::Font::bold));
        g.drawText (tabs[i].name, juce::Rectangle<float> (x0 + isz + 10.0f * s, tab.getY(), textW + 20.0f * s, tab.getHeight()), juce::Justification::centredLeft);
    }
}

void SampleLabEditor::parentHierarchyChanged()
{
    // L'editor e' stato agganciato (o spostato) nella finestra della DAW: si rifa il layout subito e di nuovo tra poco,
    // quando la DAW ha finito di assegnare dimensione e scala
    kickTick = 0;
    if (auto* top = getTopLevelComponent())
        uiLog ("attached: editor " + juce::String (getWidth()) + "x" + juce::String (getHeight()) + ", top-level "
               + juce::String (top->getWidth()) + "x" + juce::String (top->getHeight())
               + ", display scale " + juce::String (juce::Desktop::getInstance().getDisplays().getPrimaryDisplay() != nullptr
                                                        ? juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale : 0.0, 2));
}

void SampleLabEditor::resized()
{
    processor.uiWidth  = getWidth();
    processor.uiHeight = getHeight();
    if (getWidth() != lastLoggedW || getHeight() != lastLoggedH)
    {
        lastLoggedW = getWidth();
        lastLoggedH = getHeight();
        uiLog ("resized: " + juce::String (getWidth()) + "x" + juce::String (getHeight()) + " scale " + juce::String (theme::scaleFor (*this), 3));
    }

    // Scala sul lato piu' stretto (vedi theme::scaleForSize): anche in una finestra bassa tutti i controlli restano visibili
    const float s = theme::scaleFor (*this);
    const int m = juce::jmax (4, (int) (10 * s));
    auto area = getLocalBounds();

    // Barra superiore: logo (disegnato in paint), navigazione kit al centro, dimensione a destra
    topBar = area.removeFromTop (juce::jmax (36, (int) (56 * s)));
    {
        auto bar = topBar.reduced ((int) (10 * s), (int) (8 * s));
        sizeButton.setBounds (bar.removeFromRight (juce::jmax (84, (int) (108 * s))));
        bar.removeFromRight ((int) (8 * s));
        bpmLabel.setBounds (bar.removeFromRight (juce::jmax (80, (int) (110 * s))));
        bar.removeFromRight ((int) (8 * s));
        transportButton.setBounds (bar.removeFromRight (juce::jmax (56, (int) (78 * s))));

        // Gruppo kit, come nel riferimento: [<] nome [>] [Save] [Kit menu]. Le frecce passano al kit precedente/successivo
        // nella stessa cartella e si attivano solo se ce n'e' un altro.
        const int navW = juce::jmax (28, (int) (34 * s));
        const int nameW = juce::jmax (170, (int) (250 * s));
        const int btnW = juce::jmax (56, (int) (66 * s));
        const int groupW = 2 * navW + nameW + 2 * btnW + 5 * (int) (6 * s);
        auto center = bar.withSizeKeepingCentre (juce::jmin (groupW, bar.getWidth()), bar.getHeight());
        prevKitButton.setBounds (center.removeFromLeft (navW));
        center.removeFromLeft ((int) (6 * s));
        kitLabel.setBounds (center.removeFromLeft (nameW));
        center.removeFromLeft ((int) (6 * s));
        nextKitButton.setBounds (center.removeFromLeft (navW));
        center.removeFromLeft ((int) (6 * s));
        saveButton.setBounds (center.removeFromLeft (btnW));
        center.removeFromLeft ((int) (6 * s));
        kitMenuButton.setBounds (center.removeFromLeft (btnW));
    }

    tabBar = area.removeFromBottom (juce::jmax (30, (int) (52 * s)));
    area.reduce (m, m);

    // Fila dei moduli, a tutta larghezza: altezza proporzionale (limiti espressi in unita' scalate)
    const auto bottomArea = area.removeFromBottom (juce::jlimit ((int) (150 * s), (int) (250 * s), (int) ((float) area.getHeight() * 0.30f)));
    modules.setBounds (bottomArea);
    fxPanel.setBounds (bottomArea);
    area.removeFromBottom (m);
    const int bottomAreaTop = bottomArea.getY();

    // Colonne: browser | centro | pannello della cella (la scheda Seq usa tutta la larghezza, per vedere fino a 64 step)
    const auto fullArea = area;
    browser.setBounds (area.removeFromLeft (juce::jlimit (190, 330, getWidth() * 19 / 100)));
    area.removeFromLeft (m);
    const int browserRight = area.getX();
    cellPanel.setBounds (area.removeFromRight (juce::jlimit (210, 340, getWidth() * 21 / 100)));
    area.removeFromRight (m);

    const int waveH = juce::jlimit ((int) (70 * s), (int) (150 * s), (int) ((float) area.getHeight() * 0.28f));
    waveStrip.setBounds (area.removeFromBottom (waveH));
    area.removeFromBottom (m);
    grid.setBounds (area);

    // Scheda Slice: tra il browser e il bordo destro, da sopra la griglia fino al fondo dei moduli
    slicePanel.setBounds (browserRight, fullArea.getY(), fullArea.getRight() - browserRight, bottomArea.getBottom() - fullArea.getY());

    // Scheda Seq: da sopra la griglia fino al fondo dei moduli
    seqPanel.setBounds (fullArea.getX(), fullArea.getY(), fullArea.getWidth(), bottomArea.getBottom() - fullArea.getY());
    songPanel.setBounds (seqPanel.getBounds());
    juce::ignoreUnused (bottomAreaTop);
}
