#pragma once

#include "engine/FxParams.h"
#include <juce_core/juce_core.h>
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

    // Parameter lock: parametri degli effetti della cella sovrascritti solo da questo step
    std::uint32_t lockMask = 0;        // bit i = FxF(i) bloccato
    std::array<float, (size_t) FxF::Count> lockValues {};

    bool hasLock (FxF f) const noexcept { return (lockMask >> (int) f) & 1u; }
    void setLock (FxF f, float v) noexcept { lockMask |= 1u << (int) f; lockValues[(size_t) f] = v; }
    void clearLock (FxF f) noexcept { lockMask &= ~(1u << (int) f); }
};

// Una riga del sequencer: una cella con i suoi step, la sua risoluzione e la sua lunghezza.
struct Lane
{
    int cell = -1;                     // indice della cella suonata
    int division = 10;                 // indice in kDivisionNames (10 = 1/16)
    int length = 16;                   // numero di step (1..64): ogni lane puo' avere la sua lunghezza
    bool mute = false;
    bool solo = false;
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

    bool anySolo() const noexcept
    {
        for (const auto& l : lanes)
            if (l.solo)
                return true;
        return false;
    }
};
}
