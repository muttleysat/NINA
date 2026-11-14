#pragma once

#include "Pattern.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <vector>

namespace sl
{
// Un blocco della song: un pattern ripetuto 'repeats' volte (un "giro" = la durata del ciclo del pattern, Pattern::lengthBeats)
struct SongBlock
{
    int pattern = 0;                   // indice 0..kNumPatterns-1
    int repeats = 1;                   // 1..99
    bool operator== (const SongBlock&) const = default;
};

// La song e' una catena di blocchi in ordine (Pattern Sequencer). I pattern restano nel kit: modificandoli cambia anche la song.
struct Song
{
    static constexpr int kMaxBlocks = 128;
    static constexpr int kMaxRepeats = 99;

    juce::String name = "Song";
    bool loop = false;                 // alla fine ricomincia dall'inizio
    std::vector<SongBlock> blocks;

    bool operator== (const Song&) const = default;
    bool empty() const noexcept { return blocks.empty(); }

    static double blockBeats (const std::vector<Pattern>& patterns, const SongBlock& b)
    {
        const int idx = std::clamp (b.pattern, 0, (int) patterns.size() - 1);
        return patterns.empty() ? 0.0 : patterns[(size_t) idx].lengthBeats() * (double) std::clamp (b.repeats, 1, kMaxRepeats);
    }

    double totalBeats (const std::vector<Pattern>& patterns) const
    {
        double t = 0.0;
        for (const auto& b : blocks)
            t += blockBeats (patterns, b);
        return t;
    }

    // Inizio del blocco 'index' in quarti dall'inizio della song
    double blockStart (const std::vector<Pattern>& patterns, int index) const
    {
        double t = 0.0;
        for (int i = 0; i < index && i < (int) blocks.size(); ++i)
            t += blockBeats (patterns, blocks[(size_t) i]);
        return t;
    }

    // Blocco che suona alla posizione 'beats' (dopo la fine: -1)
    int blockAt (const std::vector<Pattern>& patterns, double beats) const
    {
        double t = 0.0;
        for (int i = 0; i < (int) blocks.size(); ++i)
        {
            t += blockBeats (patterns, blocks[(size_t) i]);
            if (beats < t)
                return i;
        }
        return -1;
    }
};

// Un pezzo della timeline: 'pattern' suona da 'start' a 'end' (quarti), con la sua fase che riparte da zero in 'start'
struct Segment
{
    const Pattern* pattern = nullptr;
    double start = 0.0, end = 0.0;
};

// Segmenti della song, uno per blocco. I puntatori puntano dentro 'patterns' (che deve restare vivo e immutato).
inline std::vector<Segment> makeSegments (const Song& song, const std::vector<Pattern>& patterns)
{
    std::vector<Segment> segs;
    double t = 0.0;
    for (const auto& b : song.blocks)
    {
        const double len = Song::blockBeats (patterns, b);
        if (len > 0.0)
            segs.push_back ({ &patterns[(size_t) std::clamp (b.pattern, 0, (int) patterns.size() - 1)], t, t + len });
        t += len;
    }
    return segs;
}
}
