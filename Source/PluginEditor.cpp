#include "PluginEditor.h"

SampleLabEditor::SampleLabEditor (SampleLabProcessor& p)
    : AudioProcessorEditor (&p), processor (p), grid (p), editor (p)
{
    setResizable (true, true);
    setResizeLimits (1000, 640, 3840, 2160);

    kitButton.onClick = [this] { showKitMenu(); };
    kitButton.setColour (juce::TextButton::buttonColourId, theme::panelHi);
    kitButton.setColour (juce::TextButton::textColourOffId, theme::text);
    addAndMakeVisible (kitButton);

    sizeButton.onClick = [this] { showSizeMenu(); };
    sizeButton.setColour (juce::TextButton::buttonColourId, theme::panelHi);
    sizeButton.setColour (juce::TextButton::textColourOffId, theme::text);
    addAndMakeVisible (sizeButton);

    addAndMakeVisible (browser);
    addAndMakeVisible (grid);
    addAndMakeVisible (editor);

    browser.onFileActivated = [this] (const juce::File& f) { loadIntoCell (grid.getSelected(), f); };

    grid.onSelectionChanged = [this] (int c) { editor.setCell (c); };
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
        editor.refresh();
        refreshTopBar();
    };

    editor.onEdited = [this]
    {
        grid.repaint();
        refreshTopBar();
    };

    setSize (processor.uiWidth, processor.uiHeight);
    sizeButton.setButtonText ("Size " + juce::String (juce::roundToInt (processor.uiZoom * 100.0f)) + "%");
    refreshAll();
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
    kitButton.setButtonText (kit.name + (processor.isDirty() ? " *" : "") + "  " + juce::String::fromUTF8 ("\xE2\x96\xBE"));
}

void SampleLabEditor::refreshAll()
{
    refreshTopBar();
    grid.setSelected (juce::jmin (grid.getSelected(), processor.getKit().numCells() - 1));
    editor.setCell (grid.getSelected());
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
        editor.refresh();
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

    const float s = (float) getWidth() / (float) baseWidth;
    g.setColour (theme::panel);
    g.fillRect (topBar);
    g.setColour (theme::accent);
    g.setFont (juce::FontOptions (juce::jmax (14.0f, 22.0f * s), juce::Font::bold));
    g.drawText ("SAMPLE LAB", topBar.reduced ((int) (16 * s), 0).withTrimmedRight (getWidth() / 2), juce::Justification::centredLeft);
}

void SampleLabEditor::resized()
{
    processor.uiWidth  = getWidth();
    processor.uiHeight = getHeight();

    const float s = juce::jmax (0.6f, (float) getWidth() / (float) baseWidth);
    auto area = getLocalBounds();
    const int m = juce::jmax (6, getWidth() / 160);

    topBar = area.removeFromTop (juce::jmax (36, getHeight() / 14));
    auto bar = topBar.reduced ((int) (8 * s), (int) (6 * s));
    sizeButton.setBounds (bar.removeFromRight (juce::jmax (100, (int) (110 * s))));
    bar.removeFromRight ((int) (8 * s));
    // Nome del kit al centro della barra
    kitButton.setBounds (bar.withSizeKeepingCentre (juce::jmax (220, (int) (300 * s)), bar.getHeight()));

    area.reduce (m, m);
    browser.setBounds (area.removeFromLeft (juce::jlimit (190, 420, getWidth() / 5)));
    area.removeFromLeft (m);

    auto bottom = area.removeFromBottom (area.getHeight() * 47 / 100);
    area.removeFromBottom (m);
    grid.setBounds (area);
    editor.setBounds (bottom);
}
