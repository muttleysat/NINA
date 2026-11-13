#pragma once

#include "engine/FxParams.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace sl
{
constexpr int kMaxSteps = 64;
constexpr int kNumPatterns = 16;

// Uno step di una lane. I valori non attivi (on = false) non suonano.
struct Step
{
    bool on = false;
    std::uint8_t velocity = 100;       // 1..127
    std::uint8_t probability = 100;    // 0..100 %
    std::uint8_t ratchet = 1;          // 1..4 ripetizioni dentro lo step
    float nudge = 0.0f;                // spostamento in frazione di step (-0.5..+0.5)
    float gate = 0.5f;                 // durata della nota in frazione di step (0.05..1)
    std::int8_t pitch = 0;             // trasposizione dello step in semitoni (-24..+24)

    // Comportamento di riproduzione dello step (SQ-4)
    bool reverse = false;              // inverte il verso della cella solo per questo step
    std::int8_t mode = -1;             // -1 = come la cella; 0 one-shot, 1 gate, 2 loop

    // Parameter lock: parametri degli effetti della cella sovrascritti solo da questo step.
    // Indici 0..17 = valori degli effetti (FxF), 18..23 = interruttori on/off degli effetti (FxB).
    std::uint32_t lockMask = 0;        // bit i = parametro i bloccato
    std::array<float, (size_t) kNumLockable> lockValues {};   // per gli interruttori 0 = spento, 1 = acceso

    bool hasLock (int i) const noexcept { return (lockMask >> i) & 1u; }
    void setLock (int i, float v) noexcept { lockMask |= 1u << i; lockValues[(size_t) i] = v; }
    void clearLock (int i) noexcept { lockMask &= ~(1u << i); }
    bool hasLock (FxF f) const noexcept { return hasLock ((int) f); }
    void setLock (FxF f, float v) noexcept { setLock ((int) f, v); }
    void clearLock (FxF f) noexcept { clearLock ((int) f); }
};

// Una riga del sequencer: una cella con i suoi step, la sua risoluzione e la sua lunghezza.
struct Lane
{
    int cell = -1;                     // indice della cella suonata
    int division = 10;                 // indice in kDivisionNames (10 = 1/16)
    int length = 16;                   // numero di step (1..64): ogni lane puo' avere la sua lunghezza
    bool mute = false;
    bool solo = false;
    bool dry = false;                  // true: la riga suona senza gli effetti della cella (gli step possono riaccenderli con i lock)
    std::array<Step, kMaxSteps> steps;
};

struct Pattern
{
    juce::String name = "Pattern";
    std::vector<Lane> lanes;           // solo le celle aggiunte dall'utente (tasto destro > Aggiungi al sequencer)
    float swing = 0.0f;                // 0..0.5: ritardo degli step dispari, in frazione di step
    std::uint32_t seed = 1;            // seme per probabilita' ripetibili

    Lane* findLane (int cell)
    {
        for (auto& l : lanes)
            if (l.cell == cell)
                return &l;
        return nullptr;
    }

    const Lane* findLane (int cell) const
    {
        for (const auto& l : lanes)
            if (l.cell == cell)
                return &l;
        return nullptr;
    }

    // Durata del ciclo del pattern in quarti: la riga piu' lunga (step x risoluzione). Senza righe: una battuta.
    // E' la distanza tra due punti in cui si puo' passare a un altro pattern senza tagliare il ciclo in corso.
    double lengthBeats() const noexcept
    {
        double beats = 0.0;
        for (const auto& l : lanes)
            if (l.cell >= 0)
                beats = std::max (beats, (double) std::clamp (l.length, 1, kMaxSteps)
                                             * (double) kDivisionBeats[std::clamp (l.division, 0, kNumDivisions - 1)]);
        return beats > 0.0 ? beats : 4.0;
    }

    bool anySolo() const noexcept
    {
        for (const auto& l : lanes)
            if (l.solo)
                return true;
        return false;
    }
};
}
