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

    tree.setDragAndDropDescription ("SLFILE");
    tree.setColour (juce::FileTreeComponent::backgroundColourId, theme::background);
    tree.setColour (juce::FileTreeComponent::textColourId, theme::text);
    tree.setColour (juce::FileTreeComponent::selectedItemBackgroundColourId, theme::accent.withAlpha (0.3f));
    tree.setItemHeight (22);
    tree.addListener (this);
    addAndMakeVisible (tree);

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
    area.removeFromTop ((int) (6 * s));
    tree.setBounds (area);
}
