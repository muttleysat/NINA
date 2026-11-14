#pragma once

#include "Theme.h"

// Finestra "Info": logo dell'app, versione, copyright, licenza (AGPL-3.0) con il collegamento al codice sorgente, logo ASIO, crediti
// delle librerie, pulsante di donazione volontaria (PayPal) e la finestra con i testi delle licenze.
class AboutPanel final : public juce::Component
{
public:
    explicit AboutPanel (float scale);
    ~AboutPanel() override;

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

    // Codice sorgente (la licenza AGPL richiede di metterlo a disposizione di chi riceve il programma)
    static juce::String sourceUrl() { return "https://github.com/muttleysat/NINA"; }

private:
    class SupportButton;

    juce::TextButton closeButton { "Close" }, sourceButton { "Source code" }, licenseButton { "Licenses" };
    std::unique_ptr<SupportButton> supportButton;
    juce::Image logo, asio;
    juce::Rectangle<int> logoRect, versionRect, copyrightRect, licenseTextRect, asioRect, creditsRect, supportTextRect;
    float s = 1.0f;
};

// Testi delle licenze (licenza di NINA e riconoscimenti di terzi), incorporati nel programma
class LicensesPanel final : public juce::Component
{
public:
    explicit LicensesPanel (float scale);

    void resized() override;
    static void show (float scale);

private:
    void showText (bool nina);

    juce::TextButton ninaButton { "NINA licence (AGPL-3.0)" }, thirdButton { "Third-party notices" }, closeButton { "Close" };
    juce::TextEditor text;
    float s = 1.0f;
};
