#pragma once

#include "Pattern.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace sl
{
// Evento prodotto dal sequencer per il motore, con la posizione in campioni dentro il blocco.
struct SeqEvent
{
    enum class Type : int { NoteOff = 0, NoteOn = 1, Stop = 2 };

    Type type = Type::NoteOn;
    int samplePos = 0;
    int cell = -1;
    float velocity = 1.0f;                 // 0..1
    float semis = 0.0f;                    // trasposizione dello step
    std::uint32_t lockMask = 0;            // parameter lock degli effetti (NoteOn)
    std::array<float, (size_t) FxF::Count> lockValues {};
};

// Evento in tempo musicale (quarti = ppq), indipendente dalla frequenza di campionamento e dal tempo.
struct TimedEvent
{
    double ppq = 0.0;
    SeqEvent::Type type = SeqEvent::Type::NoteOn;
    int lane = 0;
    const Step* step = nullptr;            // nullptr per Stop
    int cell = -1;
};

class Sequencer
{
public:
    // Genera gli eventi del pattern nell'intervallo di tempo [ppq0, ppq1) (in quarti dall'inizio del brano) e li passa a fn.
    // Lo stesso codice serve per la riproduzione e per l'esportazione MIDI, cosi' i due risultati coincidono.
    template <typename Fn>
    static void generate (const Pattern& pattern, double ppq0, double ppq1, Fn&& fn)
    {
        const bool anySolo = pattern.anySolo();
        const float swing = juce::jlimit (0.0f, 0.5f, pattern.swing);

        for (int laneIdx = 0; laneIdx < (int) pattern.lanes.size(); ++laneIdx)
        {
            const auto& lane = pattern.lanes[(size_t) laneIdx];
            if (lane.cell < 0 || lane.mute || (anySolo && ! lane.solo))
                continue;

            const double stepBeats = (double) kDivisionBeats[juce::jlimit (0, kNumDivisions - 1, lane.division)];
            const int len = juce::jlimit (1, kMaxSteps, lane.length);

            // Gli step possono cadere prima/dopo la loro posizione nominale (nudge, swing) e le note durano fino a 1 step:
            // si parte qualche step prima del blocco per non perdere note-on e note-off a cavallo.
            const long long kFirst = juce::jmax ((long long) 0, (long long) std::floor (ppq0 / stepBeats) - 3);
            const long long kLast = (long long) std::floor (ppq1 / stepBeats) + 2;

            for (long long k = kFirst; k <= kLast; ++k)
            {
                const Step& st = lane.steps[(size_t) (k % len)];
                if (! st.on)
                    continue;

                if (st.probability < 100 && (hash (pattern.seed, (std::uint32_t) laneIdx, (std::uint64_t) k) % 100u) >= st.probability)
                    continue;

                const double t0 = (double) k * stepBeats + ((k & 1) ? (double) swing * stepBeats : 0.0)
                                  + (double) st.nudge * stepBeats;
                const int ratchet = juce::jlimit (1, 4, (int) st.ratchet);
                const double sub = stepBeats / ratchet;
                const double gate = juce::jlimit (0.05, 1.0, (double) st.gate) * sub;

                for (int j = 0; j < ratchet; ++j)
                {
                    const double on = t0 + j * sub;
                    if (on >= ppq0 && on < ppq1)
                        fn (TimedEvent { on, SeqEvent::Type::NoteOn, laneIdx, &st, lane.cell });

                    const double off = on + gate;
                    if (off >= ppq0 && off < ppq1)
                        fn (TimedEvent { off, SeqEvent::Type::NoteOff, laneIdx, &st, lane.cell });
                }
            }
        }
    }

    // Tipo e dati per il motore a partire da un evento musicale
    static SeqEvent toSeqEvent (const TimedEvent& e, int samplePos)
    {
        SeqEvent s;
        s.type = e.type;
        s.samplePos = samplePos;
        s.cell = e.cell;
        if (e.step != nullptr && e.type == SeqEvent::Type::NoteOn)
        {
            s.velocity = (float) e.step->velocity / 127.0f;
            s.semis = (float) e.step->pitch;
            s.lockMask = e.step->lockMask;
            s.lockValues = e.step->lockValues;
        }
        return s;
    }

    // Seme ripetibile: la stessa (seed, lane, step) da' sempre lo stesso risultato (riproduzione = esportazione)
    static std::uint32_t hash (std::uint32_t seed, std::uint32_t lane, std::uint64_t step) noexcept
    {
        std::uint64_t h = (std::uint64_t) seed * 0x9E3779B97F4A7C15ull ^ ((std::uint64_t) lane + 1u) * 0xC2B2AE3D27D4EB4Full
                          ^ (step + 1u) * 0x165667B19E3779F9ull;
        h ^= h >> 33;
        h *= 0xFF51AFD7ED558CCDull;
        h ^= h >> 33;
        return (std::uint32_t) (h >> 16);
    }
};

// Parte audio del sequencer: trasforma un blocco in eventi con posizione in campioni.
// Non alloca nel thread audio (il vettore e' riservato in anticipo).
class SequencerRunner
{
public:
    SequencerRunner() { events.reserve (4096); }

    // pattern puo' essere nullptr (nessun pattern). ppq0 = posizione all'inizio del blocco, in quarti.
    // endPpq: oltre questa posizione non si generano eventi (esportazione di un numero esatto di battute).
    void runBlock (const Pattern* pattern, bool playing, double ppq0, double bpm, double sampleRate, int numSamples,
                   double endPpq = std::numeric_limits<double>::infinity()) noexcept
    {
        events.clear();

        if (wasPlaying && ! playing)
            events.push_back (stopEvent());
        wasPlaying = playing;

        if (! playing || pattern == nullptr || bpm <= 0.0 || sampleRate <= 0.0)
            return;

        const double ppq1 = ppq0 + (double) numSamples * bpm / (60.0 * sampleRate);
        const double samplesPerBeat = 60.0 * sampleRate / bpm;

        Sequencer::generate (*pattern, ppq0, std::min (ppq1, endPpq), [&] (const TimedEvent& e)
        {
            if (events.size() >= events.capacity())
                return;                                  // oltre il limite si scartano gli eventi (mai allocare)
            const int pos = juce::jlimit (0, juce::jmax (0, numSamples - 1), (int) ((e.ppq - ppq0) * samplesPerBeat));
            events.push_back (Sequencer::toSeqEvent (e, pos));
        });

        // Ordine: per posizione; a parita' prima i NoteOff, cosi' un nuovo colpo non viene tagliato dallo spegnimento del precedente
        std::sort (events.begin(), events.end(), [] (const SeqEvent& a, const SeqEvent& b)
        {
            if (a.samplePos != b.samplePos) return a.samplePos < b.samplePos;
            if (a.type != b.type) return (int) a.type < (int) b.type;
            return a.cell < b.cell;
        });
    }

    const std::vector<SeqEvent>& getEvents() const noexcept { return events; }
    void reset() noexcept { wasPlaying = false; events.clear(); }

private:
    static SeqEvent stopEvent() noexcept
    {
        SeqEvent s;
        s.type = SeqEvent::Type::Stop;
        s.samplePos = 0;
        return s;
    }

    std::vector<SeqEvent> events;
    bool wasPlaying = false;
};
}
