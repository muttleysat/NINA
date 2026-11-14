#include "AboutPanel.h"
#include <BinaryData.h>

// Pulsante di donazione: pillola gialla in stile PayPal con il logo PayPal e la scritta "Support my work"
class AboutPanel::SupportButton final : public juce::Button
{
public:
    explicit SupportButton (const juce::Image& paypalLogo) : juce::Button ("support"), logo (paypalLogo)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTooltip ("Opens PayPal in your browser. Donations are voluntary: NINA is the same for everyone.");
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto b = getLocalBounds().toFloat().reduced (1.0f);
        auto gold = juce::Colour (0xffffc439);
        if (over) gold = gold.brighter (0.08f);
        if (down) gold = gold.darker (0.1f);
        g.setColour (gold);
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);

        const float lh = b.getHeight() * 0.54f;
        const float lw = lh * (float) logo.getWidth() / (float) juce::jmax (1, logo.getHeight());
        const juce::String label = "Support my work";
        g.setFont (juce::FontOptions (b.getHeight() * 0.40f, juce::Font::bold));
        const float tw = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), label);
        const float gap = b.getHeight() * 0.28f;
        const float x0 = b.getCentreX() - (lw + gap + tw) * 0.5f;

        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (logo, juce::Rectangle<float> (x0, b.getCentreY() - lh * 0.5f, lw, lh), juce::RectanglePlacement::centred);
        g.setColour (juce::Colour (0xff003087));
        g.drawText (label, juce::Rectangle<float> (x0 + lw + gap, b.getY(), tw + 4.0f, b.getHeight()), juce::Justification::centredLeft);
    }

private:
    juce::Image logo;
};

AboutPanel::AboutPanel (float scale) : s (juce::jmin (scale, 1.1f))
{
    logo = juce::ImageCache::getFromMemory (BinaryData::nina_logo_png, BinaryData::nina_logo_pngSize);
    asio = juce::ImageCache::getFromMemory (BinaryData::asio_logo_jpg, BinaryData::asio_logo_jpgSize);

    supportButton = std::make_unique<SupportButton> (juce::ImageCache::getFromMemory (BinaryData::paypal_logo_png, BinaryData::paypal_logo_pngSize));
    supportButton->onClick = [] { juce::URL (AboutPanel::donateUrl()).launchInDefaultBrowser(); };

    for (juce::Component* c : std::initializer_list<juce::Component*> { &closeButton, &sourceButton, &licenseButton, supportButton.get() })
        addAndMakeVisible (*c);

    closeButton.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    };
    sourceButton.setTooltip ("Opens the source code of NINA (the GNU AGPL licence requires it to be available to everyone who gets the program)");
    sourceButton.onClick = [] { juce::URL (AboutPanel::sourceUrl()).launchInDefaultBrowser(); };
    licenseButton.setTooltip ("The licence of NINA and the notices of the libraries it is built with");
    licenseButton.onClick = [this] { LicensesPanel::show (s); };

    setSize ((int) (480 * s), (int) (690 * s));
}

AboutPanel::~AboutPanel() = default;

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

    g.setColour (theme::text);
    g.setFont (juce::FontOptions (juce::jmax (11.0f, 15.0f * s), juce::Font::bold));
    g.drawText (juce::CharPointer_UTF8 ("Copyright \xC2\xA9 2026 Vinz Trombetta"), copyrightRect, juce::Justification::centred);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
    g.drawFittedText ("Free software under the GNU Affero General Public License v3.\nThe source code is available to everyone who gets NINA.",
                      licenseTextRect, juce::Justification::centred, 2);

    g.setColour (theme::border);
    g.drawHorizontalLine (asioRect.getY() - (int) (10 * s), (float) getWidth() * 0.15f, (float) getWidth() * 0.85f);

    g.drawImage (asio, asioRect.toFloat(), juce::RectanglePlacement::centred);

    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.0f * s)));
    g.drawFittedText ("Made with JUCE and the Rubber Band Library. ASIO audio drivers in the Windows standalone.\n"
                      "VST is a trademark of Steinberg Media Technologies GmbH.",
                      creditsRect, juce::Justification::centred, 3);

    g.setColour (theme::border);
    g.drawHorizontalLine (supportTextRect.getY() - (int) (6 * s), (float) getWidth() * 0.15f, (float) getWidth() * 0.85f);
    g.setColour (theme::textDim);
    g.drawFittedText ("NINA is free and the same for everyone.\nIf you like it, a voluntary donation is welcome.", supportTextRect,
                      juce::Justification::centred, 2);
}

void AboutPanel::resized()
{
    auto r = getLocalBounds().reduced ((int) (24 * s));
    logoRect = r.removeFromTop ((int) (118 * s));
    versionRect = r.removeFromTop ((int) (22 * s));
    r.removeFromTop ((int) (8 * s));
    copyrightRect = r.removeFromTop ((int) (26 * s));
    r.removeFromTop ((int) (6 * s));
    licenseTextRect = r.removeFromTop ((int) (36 * s));
    r.removeFromTop ((int) (8 * s));
    {
        auto row = r.removeFromTop ((int) (30 * s)).withSizeKeepingCentre ((int) (290 * s), (int) (30 * s));
        sourceButton.setBounds (row.removeFromLeft ((int) (140 * s)));
        row.removeFromLeft ((int) (10 * s));
        licenseButton.setBounds (row);
    }
    r.removeFromTop ((int) (22 * s));
    asioRect = r.removeFromTop ((int) (104 * s));
    r.removeFromTop ((int) (8 * s));
    creditsRect = r.removeFromTop ((int) (50 * s));
    r.removeFromTop ((int) (18 * s));
    supportTextRect = r.removeFromTop ((int) (34 * s));
    r.removeFromTop ((int) (8 * s));
    supportButton->setBounds (r.removeFromTop ((int) (42 * s)).withSizeKeepingCentre ((int) (250 * s), (int) (42 * s)));
    closeButton.setBounds (r.removeFromBottom ((int) (32 * s)).withSizeKeepingCentre ((int) (110 * s), (int) (32 * s)));
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

// ------------------------------------------------------------------------------------------ Licenze

LicensesPanel::LicensesPanel (float scale) : s (juce::jmin (scale, 1.1f))
{
    for (juce::Button* b : std::initializer_list<juce::Button*> { &ninaButton, &thirdButton, &closeButton })
        addAndMakeVisible (*b);
    ninaButton.setClickingTogglesState (true);
    thirdButton.setClickingTogglesState (true);
    ninaButton.setRadioGroupId (9100);
    thirdButton.setRadioGroupId (9100);
    ninaButton.onClick = [this] { showText (true); };
    thirdButton.onClick = [this] { showText (false); };
    closeButton.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    };

    text.setMultiLine (true, true);
    text.setReadOnly (true);
    text.setCaretVisible (false);
    text.setFont (juce::FontOptions (juce::jmax (11.0f, 13.0f * s)));
    addAndMakeVisible (text);

    thirdButton.setToggleState (true, juce::dontSendNotification);
    showText (false);
    setSize ((int) (720 * s), (int) (560 * s));
}

void LicensesPanel::showText (bool nina)
{
    text.setText (nina ? juce::String::createStringFromData (BinaryData::LICENSE, BinaryData::LICENSESize)
                       : juce::String::createStringFromData (BinaryData::THIRD_PARTY_NOTICES_md, BinaryData::THIRD_PARTY_NOTICES_mdSize),
                  false);
    text.moveCaretToTop (false);
}

void LicensesPanel::resized()
{
    auto r = getLocalBounds().reduced ((int) (14 * s));
    auto top = r.removeFromTop ((int) (30 * s));
    ninaButton.setBounds (top.removeFromLeft ((int) (210 * s)));
    top.removeFromLeft ((int) (8 * s));
    thirdButton.setBounds (top.removeFromLeft ((int) (170 * s)));
    r.removeFromTop ((int) (8 * s));
    closeButton.setBounds (r.removeFromBottom ((int) (32 * s)).withSizeKeepingCentre ((int) (110 * s), (int) (32 * s)));
    r.removeFromBottom ((int) (8 * s));
    text.setBounds (r);
}

void LicensesPanel::show (float scale)
{
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (new LicensesPanel (scale));
    o.dialogTitle = "Licenses";
    o.dialogBackgroundColour = theme::panel;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = true;
    o.launchAsync();
}
