#pragma once

#include "Sequencer.h"
#include "engine/Engine.h"
#include "kit/Kit.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace sl::SequencerExport
{
// Pattern > clip MIDI standard (SQ-7). Ogni cella usa la sua nota (tasto "Key low" della cella).
// Ratchet, nudge, swing e probabilita' (con il seme del pattern) sono "risolti" in note fisse; reverse e parameter lock
// degli effetti non esistono nel MIDI e restano solo nel plugin (si ottengono con l'esportazione audio).
// bars: lunghezza in battute da 4/4; ticksPerQuarter: risoluzione del file.
juce::MidiFile toMidiFile (const Pattern& pattern, const Kit& kit, int bars, double bpm, int ticksPerQuarter = 960);

// Come toMidiFile, ma per una timeline di segmenti (song): 'totalBeats' e' la durata in quarti
juce::MidiFile toMidiFileSegments (const juce::String& name, const std::vector<Segment>& segs, double totalBeats, const Kit& kit,
                                   double bpm, int ticksPerQuarter = 960);

// Scrive il file .mid (ritorna false in caso di errore)
bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file);

// Copia del pattern in cui suona solo la lane indicata (per l'esportazione degli stem: un file per cella)
Pattern isolateLane (const Pattern& pattern, int lane);

// Come trattare le code di delay/riverbero nei file audio esportati
enum class TailMode
{
    Cut = 0,    // lunghezza esatta delle battute: file "quantizzato", la coda oltre la fine e' tagliata
    Loop,       // lunghezza esatta; la coda della fine rientra all'inizio (clip in loop senza stacchi)
    Keep        // le battute piu' tutta la coda (file piu' lungo)
};

// Rendering per l'esportazione audio con la gestione delle code scelta. Cut/Loop: esattamente 'bars' battute di campioni.
juce::AudioBuffer<float> renderBounce (Engine& engine, const Pattern& pattern, int bars, double bpm, double sampleRate, TailMode tail);

// Rendering offline (SQ-10): suona il pattern dall'inizio per 'bars' battute con un motore gia' preparato
// (prepare, snapshot pubblicato, parametri live impostati) e aggiunge 'tailSeconds' per le code di delay/riverbero.
// Piu' veloce del tempo reale, senza passare dalla DAW.
juce::AudioBuffer<float> renderOffline (Engine& engine, const Pattern& pattern, int bars, double bpm, double sampleRate,
                                        double tailSeconds = 4.0);

// Esportazione audio di una song (segmenti). Stesse regole sulle code di renderBounce; con Loop la song si suona due volte e si tiene la seconda.
juce::AudioBuffer<float> renderBounceSegments (Engine& engine, const std::vector<Segment>& segs, double totalBeats, double bpm,
                                               double sampleRate, TailMode tail);
juce::AudioBuffer<float> renderSegments (Engine& engine, const std::vector<Segment>& segs, double period, double renderBeats, double bpm,
                                         double sampleRate, double tailSeconds);
}
