#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include <vector>
#include "engine/Engine.h"
#include "kit/Kit.h"

namespace sl
{
// Parametri della cella (float)
enum class CellF { Start, End, Level, Pan, Tune, Pitch, Attack, Hold, Decay, Sustain, Release, Count };
// Parametri degli effetti (float), uguali per ogni cella e per il master
enum class FxF { Drive, SatMix, Bits, Downsample, LofiMix, Cutoff, Resonance,
                 CompThreshold, CompRatio, CompAttack, CompRelease, CompMakeup,
                 DelayTime, DelayFeedback, DelayMix, ReverbSize, ReverbDamp, ReverbMix, Count };
// Interruttori on/off degli effetti
enum class FxB { Sat, Lofi, Filter, Comp, Delay, Reverb, Count };

constexpr int kNumCells = Kit::kMaxCells;
static_assert (kNumCells == Engine::kMaxCells, "Kit e motore devono avere lo stesso numero massimo di celle");

// Riferimenti ai parametri di una catena di effetti
struct FxRefs
{
    std::array<juce::AudioParameterFloat*, (size_t) FxF::Count> f {};
    std::array<juce::AudioParameterBool*, (size_t) FxB::Count> on {};
    juce::AudioParameterChoice* filterType = nullptr;
    juce::AudioParameterBool* delaySync = nullptr;         // delay: tempo libero (ms) o sincronizzato al BPM
    juce::AudioParameterChoice* delayDivision = nullptr;   // valore musicale quando il sync e' acceso

    juce::AudioParameterFloat* at (FxF p) const { return f[(size_t) p]; }
    juce::AudioParameterBool* power (FxB p) const { return on[(size_t) p]; }
};

struct CellRefs
{
    std::array<juce::AudioParameterFloat*, (size_t) CellF::Count> f {};
    juce::AudioParameterBool* reverse = nullptr;
    juce::AudioParameterChoice* mode = nullptr;
    FxRefs fx;

    juce::AudioParameterFloat* at (CellF p) const { return f[(size_t) p]; }
};

// Tutti i parametri automatizzabili di Sample Lab. Sono la fonte di verita' dei valori: l'interfaccia, la DAW
// (automazione), il MIDI learn e il sequencer scrivono su di essi; il motore li legge nel thread audio.
// Il Kit conserva i valori solo per salvarli/caricarli (writeToKit / readFromKit).
class ParamSet
{
public:
    ParamSet();

    // Passa la proprieta' dei parametri al processore (AudioProcessor::addParameterGroup), da chiamare una volta nel costruttore.
    std::vector<std::unique_ptr<juce::AudioProcessorParameterGroup>> releaseGroups();

    // Thread audio: legge i valori correnti nelle strutture del motore. Nessuna allocazione.
    void fillLive (Engine::LiveParams& live) const noexcept;

    // Thread messaggi
    void readFromKit (const Kit& kit);                 // imposta tutti i parametri dai valori del kit
    void readCellFromKit (int cell, const Kit& kit);
    void writeToKit (Kit& kit) const;                  // copia i valori correnti nel kit (per salvare)

    const CellRefs& cell (int index) const { return cells[(size_t) index]; }
    const FxRefs& masterFx() const { return masterFxRefs; }
    juce::AudioParameterFloat* masterLevel() const { return masterLevelParam; }

    // Tutti i parametri (per registrare i listener)
    std::vector<juce::RangedAudioParameter*> all() const { return allParams; }

private:
    std::array<CellRefs, (size_t) kNumCells> cells;
    FxRefs masterFxRefs;
    juce::AudioParameterFloat* masterLevelParam = nullptr;
    std::vector<juce::RangedAudioParameter*> allParams;
    std::vector<std::unique_ptr<juce::AudioProcessorParameterGroup>> groups;
};

// Valore denormalizzato corrente di un parametro float (comodo per la UI e i test)
inline float valueOf (const juce::AudioParameterFloat* p) { return p->get(); }
}
