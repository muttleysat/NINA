#pragma once

#include "engine/CellParams.h"
#include <vector>

namespace sl
{
// "A1", "B7"...: riga (lettera) e colonna (numero) della cella
inline juce::String cellLabel (int cell, int cols)
{
    return juce::String::charToString ((juce::juce_wchar) ('A' + cell / cols)) + juce::String (cell % cols + 1);
}

// Assegnazione MIDI learn: un controller MIDI (CC) controlla un parametro (identificato dal suo ID stabile)
struct MidiMapping
{
    int cc = 0;
    juce::String paramId;
};

struct Kit
{
    static constexpr int kSchemaVersion = 1;
    static constexpr int kFirstNote = 36;   // C1 (convenzione C3 = 60, come Cubase)
    static constexpr int kMaxCells = 48;    // griglia 4x12: tutti i parametri sono automatizzabili (vedi ParamSet)

    juce::String name = "Untitled";
    int rows = 4, cols = 12;
    float masterDb = 0.0f;
    FxParams masterFx;                       // effetti globali del kit
    std::vector<MidiMapping> midiMap;        // assegnazioni MIDI learn (salvate nel kit)
    std::vector<CellParams> cells;
    juce::File file;                         // file .slkit (vuoto se il kit non e' stato ancora salvato)

    int numCells() const { return (int) cells.size(); }

    // Griglia vuota con mapping automatico da C1, una nota per cella, in ordine orizzontale per riga
    static Kit makeDefault (int rows = 4, int cols = 12)
    {
        Kit k;
        k.rows = rows;
        k.cols = cols;
        k.cells.resize ((size_t) (rows * cols));

        // Righe: rosso-arancio, giallo, blu, viola, poi verde, rosa, lime, azzurro
        static const juce::uint32 rowColours[] = { 0xfff0603c, 0xffe8b43c, 0xff3c8cf0, 0xffb450e6,
                                                    0xff3cc38a, 0xffe65a9b, 0xff9bd13c, 0xff3cc8e6 };
        for (int i = 0; i < (int) k.cells.size(); ++i)
        {
            auto& c = k.cells[(size_t) i];
            c.rootNote = c.lowNote = c.highNote = juce::jlimit (0, 127, kFirstNote + i);
            c.colour = rowColours[(i / cols) % 8];
        }
        return k;
    }
};
}
