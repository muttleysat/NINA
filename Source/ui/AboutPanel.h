#pragma once

#include "Theme.h"
#include "IconButton.h"

// Finestra "Info": logo dell'app, versione, copyright e logo ASIO (driver audio dello standalone).
class AboutPanel final : public juce::Component
{
public:
    explicit AboutPanel (float scale);

    void paint (juce::Graphics&) override;
    void resized() override;

    // Apre la finestra (non bloccante) al centro dello schermo
    static void show (float scale);

    // Pagina di donazione libera (PayPal): e' l'unico punto del programma dove compare. Per cambiarla basta modificare questo indirizzo.
    static juce::String donateUrl()
    {
        return "https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=removed"
               "&item_name=Support+NINA+%28voluntary+donation%29&currency_code=EUR";
    }

private:
    juce::TextButton closeButton { "Close" };
    IconButton supportButton { "Support NINA (PayPal)", IconButton::Icon::None };
    juce::Image logo, asio;
    juce::Rectangle<int> logoRect, versionRect, copyrightRect, asioRect, asioTextRect, supportTextRect;
    float s = 1.0f;
};
