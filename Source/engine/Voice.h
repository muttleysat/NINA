#pragma once

#include "KitSnapshot.h"

namespace sl
{
// Una voce di riproduzione: legge un campione con interpolazione cubica, pitch, reverse/loop e AHDSR.
class Voice
{
public:
    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return stage == Stage::Release; }
    int getNote() const noexcept { return note; }
    int getCellIndex() const noexcept { return cellIndex; }
    juce::uint64 getAge() const noexcept { return age; }
    TriggerMode getMode() const noexcept { return dsp.mode; }

    void start (const CellRuntime& cell, int noteNumber, float velocity01, double outputSampleRate, juce::uint64 ageStamp) noexcept;
    void release() noexcept;
    void kill() noexcept { active = false; }

    // Somma l'uscita nei buffer l / r
    void render (float* l, float* r, int numSamples) noexcept;

private:
    enum class Stage { Attack, Hold, Decay, Sustain, Release };

    bool active = false;
    const SampleData* data = nullptr;
    CellDsp dsp;
    int cellIndex = -1, note = -1;
    juce::uint64 age = 0;

    double pos = 0.0, step = 1.0;
    int regionStart = 0, regionEnd = 0;
    bool forward = true;
    float gainL = 1.0f, gainR = 1.0f;

    Stage stage = Stage::Attack;
    float env = 0.0f, attackInc = 1.0f, decayDec = 0.0f, releaseDec = 0.0f, sustainLevel = 1.0f;
    int holdSamplesLeft = 0;
    double sampleRate = 44100.0;
};
}
