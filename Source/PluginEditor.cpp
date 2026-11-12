#include "PluginEditor.h"

SampleLabEditor::SampleLabEditor (SampleLabProcessor& p)
    : AudioProcessorEditor (&p), processor (p), grid (p), waveStrip (p), cellPanel (p), modules (p), fxPanel (p)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);

    setResizable (true, true);
    setResizeLimits (1000, 640, 3840, 2160);

    kitButton.onClick = [this] { showKitMenu(); };
    prevKitButton.onClick = [this] { confirmDiscard ([this] { doOpenAdjacent (-1); }); };
    nextKitButton.onClick = [this] { confirmDiscard ([this] { doOpenAdjacent (+1); }); };
    saveButton.onClick = [this] { doSave(); };
    sizeButton.onClick = [this] { showSizeMenu(); };
    for (auto* b : { &kitButton, &prevKitButton, &nextKitButton, &saveButton, &sizeButton })
        addAndMakeVisible (b);

    addAndMakeVisible (browser);
    addAndMakeVisible (grid);
    addAndMakeVisible (waveStrip);
    addAndMakeVisible (cellPanel);
    addAndMakeVisible (modules);
    addChildComponent (fxPanel);

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
        refreshTopBar();
    };

    auto edited = [this]
    {
        grid.repaint();
        refreshTopBar();
    };
    waveStrip.onEdited = edited;
    modules.onEdited = edited;
    fxPanel.onEdited = edited;
    cellPanel.onEdited = [this]
    {
        grid.repaint();
        refreshTopBar();
    };

    cellPanel.onStep = [this] (int delta) { grid.setSelected (grid.getSelected() + delta); };
    cellPanel.onLoadRequested = [this] (int c) { requestLoadDialog (c); };

    setSize (processor.uiWidth, processor.uiHeight);
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (processor.uiZoom * 100.0f)) + "%");
    refreshAll();
}

SampleLabEditor::~SampleLabEditor()
{
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
    kitButton.setButtonText (kit.name + (processor.isDirty() ? " *" : ""));
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
    const float s = juce::jmax (0.6f, (float) getWidth() / (float) baseWidth);
    auto r = tabBar.reduced ((int) (10 * s), (int) (4 * s));
    const int tw = (int) (74 * s), gap = (int) (6 * s);
    return { r.getX() + index * (tw + gap), r.getY(), tw, r.getHeight() };
}

void SampleLabEditor::showTab (int tab)
{
    currentTab = tab;
    modules.setVisible (tab == 0);
    fxPanel.setVisible (tab == 3);
    repaint (tabBar);
}

void SampleLabEditor::mouseDown (const juce::MouseEvent& e)
{
    // Schede attive: Main (0) e FX (3)
    for (int tab : { 0, 3 })
        if (tabRect (tab).contains (e.getPosition()))
            showTab (tab);
}

void SampleLabEditor::refreshAll()
{
    refreshTopBar();
    grid.setSelected (juce::jmin (grid.getSelected(), processor.getKit().numCells() - 1));
    refreshCellViews();
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

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (kitButton),
                     [this, recents] (int r)
                     {
                         if (r == 1) confirmDiscard ([this] { doNew(); });
                         else if (r == 2) confirmDiscard ([this] { doOpen(); });
                         else if (r == 3) doSave();
                         else if (r == 4) doSaveAs();
                         else if (r == 5) processor.clearRecentKits();
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

    const float s = juce::jmax (0.6f, (float) getWidth() / (float) baseWidth);

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
            g.setColour (i == 0 || i == 3 ? theme::text.withAlpha (0.85f) : theme::textDim.withAlpha (0.45f));   // le altre arriveranno
        g.drawText (tabs[i], tab, juce::Justification::centred);
    }
}

void SampleLabEditor::resized()
{
    processor.uiWidth  = getWidth();
    processor.uiHeight = getHeight();

    const float s = juce::jmax (0.6f, (float) getWidth() / (float) baseWidth);
    const int m = juce::jmax (6, (int) (10 * s));
    auto area = getLocalBounds();

    // Barra superiore: logo (disegnato in paint), navigazione kit al centro, dimensione a destra
    topBar = area.removeFromTop (juce::jmax (42, (int) (56 * s)));
    {
        auto bar = topBar.reduced ((int) (10 * s), (int) (8 * s));
        sizeButton.setBounds (bar.removeFromRight (juce::jmax (96, (int) (108 * s))));
        bar.removeFromRight ((int) (8 * s));
        saveButton.setBounds (bar.removeFromRight (juce::jmax (60, (int) (70 * s))));

        const int navW = juce::jmax (28, (int) (34 * s));
        const int nameW = juce::jmax (200, (int) (300 * s));
        auto center = bar.withSizeKeepingCentre (nameW + 2 * (navW + (int) (6 * s)), bar.getHeight());
        prevKitButton.setBounds (center.removeFromLeft (navW));
        center.removeFromLeft ((int) (6 * s));
        nextKitButton.setBounds (center.removeFromRight (navW));
        center.removeFromRight ((int) (6 * s));
        kitButton.setBounds (center);
    }

    tabBar = area.removeFromBottom (juce::jmax (24, (int) (32 * s)));
    area.reduce (m, m);

    // Fila dei moduli, a tutta larghezza
    const auto bottomArea = area.removeFromBottom (juce::jlimit (130, 240, (int) ((float) area.getHeight() * 0.30f)));
    modules.setBounds (bottomArea);
    fxPanel.setBounds (bottomArea);
    area.removeFromBottom (m);

    // Colonne: browser | centro | pannello della cella
    browser.setBounds (area.removeFromLeft (juce::jlimit (190, 330, getWidth() * 19 / 100)));
    area.removeFromLeft (m);
    cellPanel.setBounds (area.removeFromRight (juce::jlimit (210, 340, getWidth() * 21 / 100)));
    area.removeFromRight (m);

    const int waveH = juce::jlimit (70, 150, (int) ((float) area.getHeight() * 0.28f));
    waveStrip.setBounds (area.removeFromBottom (waveH));
    area.removeFromBottom (m);
    grid.setBounds (area);
}
