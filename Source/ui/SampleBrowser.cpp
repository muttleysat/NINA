#include "SampleBrowser.h"

SampleBrowser::SampleBrowser()
{
    thread.startThread (juce::Thread::Priority::low);

    rebuildRoots();
    rootBox.onChange = [this]
    {
        const int idx = rootBox.getSelectedId() - 1;
        if (idx >= 0 && idx < (int) roots.size())
            setRoot (roots[(size_t) idx]);
    };
    addAndMakeVisible (rootBox);

    searchBox.setFont (juce::FontOptions (14.0f));
    searchBox.setJustification (juce::Justification::centredLeft);
    searchBox.setIndents (8, 0);
    searchBox.setTextToShowWhenEmpty ("Search samples...", theme::textDim);
    searchBox.setTooltip ("Filter the files shown in the folders by name");
    searchBox.onTextChange = [this]
    {
        filter.setText (searchBox.getText());
        contents.refresh();
    };
    addAndMakeVisible (searchBox);

    tree.setDragAndDropDescription ("SLFILE");
    tree.setColour (juce::FileTreeComponent::backgroundColourId, theme::background);
    tree.setColour (juce::FileTreeComponent::textColourId, theme::text);
    tree.setColour (juce::FileTreeComponent::selectedItemBackgroundColourId, theme::accent.withAlpha (0.3f));
    tree.setItemHeight (22);
    tree.addListener (this);
    addAndMakeVisible (tree);

    // Anteprima: interruttore e volume (UI-3)
    autoPlay.setToggleState (true, juce::dontSendNotification);
    autoPlay.setColour (juce::ToggleButton::textColourId, theme::text);
    autoPlay.setColour (juce::ToggleButton::tickColourId, theme::accent);
    autoPlay.onClick = [this] { selectionChanged(); };
    addAndMakeVisible (autoPlay);

    previewVolume.setSliderStyle (juce::Slider::LinearHorizontal);
    previewVolume.setRange (-24.0, 6.0, 0.1);
    previewVolume.setValue (0.0, juce::dontSendNotification);
    previewVolume.setDoubleClickReturnValue (true, 0.0);
    previewVolume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    previewVolume.setColour (juce::Slider::trackColourId, theme::accent);
    previewVolume.setColour (juce::Slider::thumbColourId, theme::text);
    previewVolume.setTooltip ("Preview volume");
    addAndMakeVisible (previewVolume);

    // Parte da Documenti (o dalla prima unita' disponibile)
    const auto docs = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    for (int i = 0; i < (int) roots.size(); ++i)
        if (roots[(size_t) i] == docs)
            rootBox.setSelectedId (i + 1, juce::sendNotificationSync);
    if (rootBox.getSelectedId() == 0 && ! roots.empty())
        rootBox.setSelectedId (1, juce::sendNotificationSync);
}

SampleBrowser::~SampleBrowser()
{
    tree.removeListener (this);
    thread.stopThread (1000);
}

void SampleBrowser::rebuildRoots()
{
    roots.clear();
    rootBox.clear (juce::dontSendNotification);

    auto add = [this] (const juce::String& label, const juce::File& f)
    {
        roots.push_back (f);
        rootBox.addItem (label, (int) roots.size());
    };

    add ("Home", juce::File::getSpecialLocation (juce::File::userHomeDirectory));
    add ("Desktop", juce::File::getSpecialLocation (juce::File::userDesktopDirectory));
    add ("Documents", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory));
    add ("Music", juce::File::getSpecialLocation (juce::File::userMusicDirectory));

    juce::Array<juce::File> drives;
    juce::File::findFileSystemRoots (drives);
    for (const auto& d : drives)
        add ("Drive  " + d.getFullPathName(), d);
}

void SampleBrowser::setRoot (const juce::File& f)
{
    contents.setDirectory (f, true, true);
}

void SampleBrowser::paint (juce::Graphics& g)
{
    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f * theme::scaleFor (*this));
}

void SampleBrowser::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds().reduced ((int) (6 * s));
    rootBox.setBounds (area.removeFromTop ((int) (26 * s)));
    area.removeFromTop ((int) (4 * s));
    searchBox.setBounds (area.removeFromTop ((int) (26 * s)));
    area.removeFromTop ((int) (6 * s));

    auto footer = area.removeFromBottom ((int) (28 * s));
    autoPlay.setBounds (footer.removeFromLeft (juce::jmax (96, (int) (110 * s))));
    previewVolume.setBounds (footer);
    area.removeFromBottom ((int) (4 * s));
    tree.setItemHeight (juce::jmax (16, (int) (18 * s)));
    tree.setBounds (area);
}
