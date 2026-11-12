#pragma once

#include "CellParams.h"
#include "SampleData.h"
#include <vector>

namespace sl
{
struct CellRuntime
{
    int cellIndex = -1;
    CellDsp dsp;
    FxParams fx;
    const SampleData* data = nullptr;  // i campioni restano vivi per tutta la vita del processore
};

// Fotografia immutabile dello stato del kit, letta dal thread audio.
struct KitSnapshot
{
    std::vector<CellRuntime> cells;
    float masterGain = 1.0f;
    FxParams masterFx;
    int generation = 0;                 // cambia quando si carica/crea un kit: ferma le voci attive
};

using KitSnapshotPtr = std::shared_ptr<const KitSnapshot>;

// Passaggio di snapshot dal thread UI al thread audio, senza lock bloccanti e senza
// allocazioni/deallocazioni nel thread audio.
class SnapshotExchange
{
public:
    // Thread UI
    void publish (KitSnapshotPtr p)
    {
        const juce::SpinLock::ScopedLockType l (lock);
        retired.reset();                // gli snapshot ritirati dal thread audio si liberano qui
        pending = std::move (p);
    }

    // Thread audio: ritorna true se current e' stato aggiornato
    bool tryAcquire (KitSnapshotPtr& current) noexcept
    {
        const juce::SpinLock::ScopedTryLockType l (lock);
        if (! l.isLocked() || pending == nullptr)
            return false;

        retired = std::move (current);  // niente free: resta in retired fino al prossimo publish
        current = std::move (pending);
        pending = nullptr;
        return true;
    }

private:
    juce::SpinLock lock;
    KitSnapshotPtr pending, retired;
};
}
