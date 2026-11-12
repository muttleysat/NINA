#pragma once

#include "engine/CellParams.h"
#include <vector>

namespace sl
{
struct Kit
{
    static constexpr int kSchemaVersion = 1;
    static constexpr int kFirstNote = 36;   // C1 (convenzione C3 = 60, come Cubase)
    static constexpr int kMaxCells = 128;

    juce::String name = "Untitled";
    int rows = 4, cols = 12;
    float masterDb = 0.0f;
    FxParams masterFx;                       // effetti globali del kit
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
