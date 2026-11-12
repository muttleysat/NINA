#include "PluginEditor.h"

SampleLabEditor::SampleLabEditor (SampleLabProcessor& p)
    : AudioProcessorEditor (&p), processor (p), grid (p), waveStrip (p), cellPanel (p), modules (p), fxPanel (p), seqPanel (p)
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

    browser.onFileActivated = [this] (const juce::File& f) { loadIntoCell (grid.getSelected(), f); };
    browser.onPreview = [this] (const juce::File& f) { processor.auditionFile (f, browser.getPreviewGainDb()); };
    browser.onPreviewStop = [this] { processor.stopAudition(); };

    grid.onSelectionChanged = [this] (int) { refreshCellViews(); };
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

    setSize (processor.uiWidth, processor.uiHeight);
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (processor.uiZoom * 100.0f)) + "%");
    refreshAll();

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

bool SampleLabEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        toggleTransport();
        return true;
    }
    return false;
}

// Il nome del kit mostra l'asterisco anche quando un parametro cambia dall'automazione o dal MIDI
void SampleLabEditor::timerCallback()
{
    processor.pollMidiLearn();      // completa l'assegnazione se e' arrivato un CC durante il MIDI learn
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
}

// ------------------------------------------------------------------- Schede

juce::Rectangle<int> SampleLabEditor::tabRect (int index) const
{
    const float s = theme::scaleFor (*this);
    auto r = tabBar.reduced ((int) (10 * s), (int) (4 * s));
    const int tw = (int) (74 * s), gap = (int) (6 * s);
    return { r.getX() + index * (tw + gap), r.getY(), tw, r.getHeight() };
}

void SampleLabEditor::showTab (int tab)
{
    currentTab = tab;
    const bool seq = tab == 1;

    // La scheda Seq occupa lo spazio di griglia, forma d'onda e moduli
    grid.setVisible (! seq);
    waveStrip.setVisible (! seq);
    seqPanel.setVisible (seq);
    modules.setVisible (tab == 0);
    fxPanel.setVisible (tab == 3);
    if (seq)
        seqPanel.refresh();
    theme::noButtonFocus (*this);
    repaint (tabBar);
}

void SampleLabEditor::mouseDown (const juce::MouseEvent& e)
{
    // Schede attive: Main (0), Seq (1) e FX (3)
    for (int tab : { 0, 1, 3 })
        if (tabRect (tab).contains (e.getPosition()))
            showTab (tab);
}

void SampleLabEditor::refreshAll()
{
    refreshTopBar();
    grid.setSelected (juce::jmin (grid.getSelected(), processor.getKit().numCells() - 1));
    refreshCellViews();
    seqPanel.refresh();
    grid.repaint();
}

void SampleLabEditor::showError (const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Sample Lab", message);
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
    m.addItem (1, "New kit");
    m.addItem (2, "Open...");
    m.addItem (3, "Save");
    m.addItem (4, "Save as...");
    m.addSeparator();
    m.addItem (6, "Normalize all cells (peak -1 dBFS)");
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
                         if (r == 1) confirmDiscard ([this] { doNew(); });
                         else if (r == 2) confirmDiscard ([this] { doOpen(); });
                         else if (r == 3) doSave();
                         else if (r == 4) doSaveAs();
                         else if (r == 5) processor.clearRecentKits();
                         else if (r == 6)
                         {
                             const int n = processor.normalizeAllCells();
                             refreshCellViews();
                             juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Sample Lab",
                                 juce::String (n) + " cell(s) normalized: each Level was set so the loudest peak reaches -1 dBFS.\n"
                                                    "The audio files are not modified; you can still adjust each Level.");
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

    const auto logoArea = topBar.reduced ((int) (18 * s), 0).withWidth ((int) (260 * s));
    g.setFont (juce::FontOptions (juce::jmax (15.0f, 24.0f * s), juce::Font::bold));
    const juce::String sample = "SAMPLE ";
    const int sw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), sample);
    g.setColour (theme::text);
    g.drawText (sample, logoArea, juce::Justification::centredLeft);
    g.setColour (theme::accent);
    g.drawText ("LAB", logoArea.withTrimmedLeft (sw), juce::Justification::centredLeft);

    // Barra delle schede in basso: per ora e' attiva solo "Main" (le altre arriveranno con le fasi successive)
    g.setColour (theme::panel);
    g.fillRect (tabBar);
    g.setColour (theme::border);
    g.drawHorizontalLine (tabBar.getY(), 0.0f, (float) getWidth());

    const char* tabs[] = { "Main", "Seq", "Mixer", "FX", "Macros" };
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
    for (int i = 0; i < 5; ++i)
    {
        const auto tab = tabRect (i);
        if (i == currentTab)
        {
            g.setColour (theme::accent);
            g.fillRoundedRectangle (tab.toFloat(), 6.0f * s);
            g.setColour (juce::Colour (0xff1a1006));
        }
        else
            g.setColour (i == 0 || i == 1 || i == 3 ? theme::text.withAlpha (0.85f) : theme::textDim.withAlpha (0.45f));   // Mixer e Macros arriveranno
        g.drawText (tabs[i], tab, juce::Justification::centred);
    }
}

void SampleLabEditor::resized()
{
    processor.uiWidth  = getWidth();
    processor.uiHeight = getHeight();

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

    tabBar = area.removeFromBottom (juce::jmax (22, (int) (32 * s)));
    area.reduce (m, m);

    // Fila dei moduli, a tutta larghezza: altezza proporzionale (limiti espressi in unita' scalate)
    const auto bottomArea = area.removeFromBottom (juce::jlimit ((int) (150 * s), (int) (250 * s), (int) ((float) area.getHeight() * 0.30f)));
    modules.setBounds (bottomArea);
    fxPanel.setBounds (bottomArea);
    area.removeFromBottom (m);
    const int bottomAreaTop = bottomArea.getY();

    // Colonne: browser | centro | pannello della cella
    browser.setBounds (area.removeFromLeft (juce::jlimit (190, 330, getWidth() * 19 / 100)));
    area.removeFromLeft (m);
    cellPanel.setBounds (area.removeFromRight (juce::jlimit (210, 340, getWidth() * 21 / 100)));
    area.removeFromRight (m);

    const int waveH = juce::jlimit ((int) (70 * s), (int) (150 * s), (int) ((float) area.getHeight() * 0.28f));
    waveStrip.setBounds (area.removeFromBottom (waveH));
    area.removeFromBottom (m);
    grid.setBounds (area);

    // Scheda Seq: da sopra la griglia fino al fondo dei moduli
    seqPanel.setBounds (area.getX(), area.getY(), area.getWidth(), bottomArea.getBottom() - area.getY());
    juce::ignoreUnused (bottomAreaTop);
}
