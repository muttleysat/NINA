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

// Scrive il file .mid (ritorna false in caso di errore)
bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file);

// Rendering offline (SQ-10): suona il pattern dall'inizio per 'bars' battute con un motore gia' preparato
// (prepare, snapshot pubblicato, parametri live impostati) e aggiunge 'tailSeconds' per le code di delay/riverbero.
// Piu' veloce del tempo reale, senza passare dalla DAW.
juce::AudioBuffer<float> renderOffline (Engine& engine, const Pattern& pattern, int bars, double bpm, double sampleRate,
                                        double tailSeconds = 4.0);
}
