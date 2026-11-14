#include "AboutPanel.h"
#include <BinaryData.h>

AboutPanel::AboutPanel (float scale) : s (scale)
{
    logo = juce::ImageCache::getFromMemory (BinaryData::nina_logo_png, BinaryData::nina_logo_pngSize);
    asio = juce::ImageCache::getFromMemory (BinaryData::asio_logo_jpg, BinaryData::asio_logo_jpgSize);
    addAndMakeVisible (closeButton);
    closeButton.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    };
    setSize ((int) (460 * s), (int) (500 * s));
}

void AboutPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::panel);
    g.setColour (theme::border);
    g.drawRect (getLocalBounds(), 1);

    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (logo, logoRect.toFloat(), juce::RectanglePlacement::centred);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s)));
    g.drawText ("Version " + juce::String (JucePlugin_VersionString), versionRect, juce::Justification::centred);

    g.setColour (theme::border);
    g.drawHorizontalLine (copyrightRect.getY() - (int) (10 * s), (float) getWidth() * 0.18f, (float) getWidth() * 0.82f);

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (11.0f, 15.0f * s), juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("Copyright \xC2\xA9 2026 Vinz Trombetta"), copyrightRect, juce::Justification::centred);

    g.drawImage (asio, asioRect.toFloat(), juce::RectanglePlacement::centred);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s)));
    g.drawText ("ASIO audio drivers (standalone)", asioTextRect, juce::Justification::centred);
}

void AboutPanel::resized()
{
    auto r = getLocalBounds().reduced ((int) (24 * s));
    logoRect = r.removeFromTop ((int) (130 * s));
    versionRect = r.removeFromTop ((int) (24 * s));
    r.removeFromTop ((int) (22 * s));
    copyrightRect = r.removeFromTop ((int) (28 * s));
    r.removeFromTop ((int) (18 * s));
    asioRect = r.removeFromTop ((int) (130 * s));
    asioTextRect = r.removeFromTop ((int) (22 * s));
    closeButton.setBounds (r.removeFromBottom ((int) (34 * s)).withSizeKeepingCentre ((int) (110 * s), (int) (34 * s)));
}

void AboutPanel::show (float scale)
{
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (new AboutPanel (scale));
    o.dialogTitle = "About NINA";
    o.dialogBackgroundColour = theme::panel;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = false;
    o.launchAsync();
}
