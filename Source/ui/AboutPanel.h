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
    static juce::String donateUrl() { return "https://www.paypal.me/VincenzoTrombetta423"; }

    // Codice sorgente (la licenza AGPL richiede di metterlo a disposizione di chi riceve il programma)
    static juce::String sourceUrl() { return "https://github.com/muttleysat/NINA"; }

    // Pagina delle versioni pubblicate (Releases) e il suo indirizzo per il controllo degli aggiornamenti
    static juce::String releasesUrl() { return sourceUrl() + "/releases"; }
    static juce::String latestReleaseApiUrl() { return "https://api.github.com/repos/muttleysat/NINA/releases/latest"; }

private:
    class SupportButton;
    void checkForUpdates();                                    // solo quando l'utente preme il pulsante: nessuna connessione automatica

    juce::TextButton closeButton { "Close" }, sourceButton { "Source code" }, licenseButton { "Licenses" }, updateButton { "Check for updates" };
    std::unique_ptr<SupportButton> supportButton;
    juce::Image logo, asio;
    juce::Rectangle<int> logoRect, versionRect, copyrightRect, licenseTextRect, asioRect, creditsRect, supportTextRect, updateTextRect;
    juce::String updateMessage, updateUrl;                     // risultato dell'ultimo controllo (updateUrl: pagina da aprire se c'e' una versione nuova)
    bool checking = false;
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
