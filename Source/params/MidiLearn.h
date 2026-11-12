#pragma once

#include "ParamSet.h"
#include <atomic>

namespace sl
{
// MIDI learn dei parametri: un CC controlla uno o piu' parametri (stesso sistema dell'automazione della DAW).
//
// - Thread messaggi: avvio/annullamento dell'apprendimento, cancellazione, elenco delle assegnazioni.
// - Thread audio: process() legge i CC dal flusso MIDI e muove i parametri (valido anche nell'export offline
//   quando i CC arrivano da una clip MIDI). Nessun lock e nessuna allocazione.
//
// Limiti della versione attuale: solo messaggi CC (non pitch bend/aftertouch), tutti i canali MIDI, modalita'
// assoluta senza soft takeover.
class MidiLearn
{
public:
    explicit MidiLearn (const ParamSet& p) : params (p) { clearTable(); }

    // --- thread messaggi ---
    void startLearn (juce::RangedAudioParameter* p);     // il prossimo CC ricevuto viene assegnato a p
    void cancelLearn();
    bool isLearning (const juce::RangedAudioParameter* p) const { return p != nullptr && p == pendingParam; }
    juce::RangedAudioParameter* learningParam() const { return pendingParam; }

    int ccFor (const juce::RangedAudioParameter* p) const;   // -1 se non assegnato
    void clear (const juce::RangedAudioParameter* p);
    void clearAll();

    // Da chiamare periodicamente (timer UI): se e' arrivato un CC durante l'apprendimento crea l'assegnazione.
    // Ritorna true se e' cambiato qualcosa.
    bool poll();

    std::vector<MidiMapping> mappings() const;
    void setMappings (const std::vector<MidiMapping>& list);

    // --- thread audio ---
    void process (const juce::MidiBuffer& midi) noexcept;

private:
    struct Entry
    {
        int cc;
        juce::RangedAudioParameter* param;
    };

    static constexpr int kSlots = 4;      // parametri massimi per lo stesso CC

    void rebuildTable();
    void clearTable();

    const ParamSet& params;
    std::vector<Entry> entries;                              // solo thread messaggi
    juce::RangedAudioParameter* pendingParam = nullptr;      // solo thread messaggi

    std::array<std::array<std::atomic<juce::RangedAudioParameter*>, kSlots>, 128> table;
    std::atomic<juce::RangedAudioParameter*> learnTarget { nullptr };
    std::atomic<int> learnedCc { -1 };
};
}
