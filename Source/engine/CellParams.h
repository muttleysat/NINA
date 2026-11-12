#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace sl
{
enum class TriggerMode : int
{
    OneShot = 0,   // suona fino in fondo, il note off e' ignorato
    Gate    = 1,   // suona finche' il tasto e' premuto, poi release
    Loop    = 2    // ripete finche' il tasto e' premuto
};

// Parametri di una cella (thread messaggi/UI). Contiene stringhe: non usare nel thread audio.
struct CellParams
{
    juce::String name;
    juce::String samplePath;           // percorso assoluto in memoria; relativo alla cartella del kit su disco
    juce::uint32 colour = 0xff3ec1d3;

    float start = 0.0f, end = 1.0f;    // normalizzati 0..1 sulla lunghezza del campione
    float levelDb = 0.0f;              // -inf..+12 dB (min -60 = muto)
    float pan = 0.0f;                  // -1..+1
    float tuneCents = 0.0f;            // -100..+100
    float pitchSemis = 0.0f;           // -24..+24
    bool reverse = false;
    TriggerMode mode = TriggerMode::OneShot;

    // Inviluppo AHDSR (secondi, sustain 0..1)
    float attack = 0.0005f, hold = 0.0f, decay = 0.1f, sustain = 1.0f, release = 0.02f;

    int rootNote = 36, lowNote = 36, highNote = 36;

    bool missing = false;              // campione non trovato al caricamento del kit (non serializzato)

    bool hasSample() const { return samplePath.isNotEmpty(); }
};

// Versione "pura" dei parametri, utilizzabile in sicurezza nel thread audio (nessuna stringa).
struct CellDsp
{
    float start = 0.0f, end = 1.0f;
    float gain = 1.0f;                 // lineare (da levelDb)
    float pan = 0.0f;
    float tuneCents = 0.0f, pitchSemis = 0.0f;
    bool reverse = false;
    TriggerMode mode = TriggerMode::OneShot;
    float attack = 0.0005f, hold = 0.0f, decay = 0.1f, sustain = 1.0f, release = 0.02f;
    int rootNote = 36, lowNote = 36, highNote = 36;
};

inline float dbToGain (float db) { return db <= -60.0f ? 0.0f : std::pow (10.0f, db / 20.0f); }

inline CellDsp toDsp (const CellParams& p)
{
    CellDsp d;
    d.start = juce::jlimit (0.0f, 1.0f, p.start);
    d.end = juce::jlimit (0.0f, 1.0f, p.end);
    d.gain = dbToGain (p.levelDb);
    d.pan = juce::jlimit (-1.0f, 1.0f, p.pan);
    d.tuneCents = p.tuneCents;
    d.pitchSemis = p.pitchSemis;
    d.reverse = p.reverse;
    d.mode = p.mode;
    d.attack = juce::jmax (0.0f, p.attack);
    d.hold = juce::jmax (0.0f, p.hold);
    d.decay = juce::jmax (0.0f, p.decay);
    d.sustain = juce::jlimit (0.0f, 1.0f, p.sustain);
    d.release = juce::jmax (0.0f, p.release);
    d.rootNote = p.rootNote;
    d.lowNote = juce::jmin (p.lowNote, p.highNote);
    d.highNote = juce::jmax (p.lowNote, p.highNote);
    return d;
}
}
