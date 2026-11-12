#pragma once

#include "Voice.h"
#include <array>
#include <atomic>

namespace sl
{
class Engine
{
public:
    static constexpr int kMaxVoices = 64;
    static constexpr int kMaxCells = 128;

    void prepare (double sampleRate);

    // Thread UI
    void publish (KitSnapshotPtr snapshot) { exchange.publish (std::move (snapshot)); }

    // Thread audio. Somma nel buffer (che deve essere gia' azzerato dal chiamante).
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi) noexcept;

    // Numero di voci attive per cella (letto dalla UI per l'indicatore di attivita')
    int getCellActivity (int cell) const noexcept
    {
        return (cell >= 0 && cell < kMaxCells) ? activity[(size_t) cell].load (std::memory_order_relaxed) : 0;
    }

private:
    void noteOn (int note, float velocity) noexcept;
    void noteOff (int note) noexcept;
    void renderVoices (float* l, float* r, int start, int count) noexcept;

    double sampleRate = 44100.0;
    SnapshotExchange exchange;
    KitSnapshotPtr current;
    int lastGeneration = -1;
    juce::uint64 ageCounter = 0;

    std::array<Voice, kMaxVoices> voices;
    std::array<std::atomic<int>, kMaxCells> activity {};
};
}
