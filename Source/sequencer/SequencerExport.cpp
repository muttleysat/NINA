#include "SequencerExport.h"

namespace sl::SequencerExport
{
juce::MidiFile toMidiFile (const Pattern& pattern, const Kit& kit, int bars, double bpm, int ticksPerQuarter)
{
    const double endPpq = (double) juce::jmax (1, bars) * 4.0;
    return toMidiFileSegments (pattern.name, { Segment { &pattern, 0.0, endPpq } }, endPpq, kit, bpm, ticksPerQuarter);
}

juce::MidiFile toMidiFileSegments (const juce::String& name, const std::vector<Segment>& segs, double endPpq, const Kit& kit, double bpm,
                                   int ticksPerQuarter)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);

    // Traccia 0: tempo e nome
    juce::MidiMessageSequence conductor;
    conductor.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / juce::jlimit (20.0, 400.0, bpm))), 0.0);
    conductor.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    conductor.addEvent (juce::MidiMessage::textMetaEvent (3, name), 0.0);
    conductor.updateMatchedPairs();
    file.addTrack (conductor);

    juce::MidiMessageSequence notes;

    Sequencer::generateSegments (segs, 0.0, 0.0, endPpq, [&] (const TimedEvent& e)
    {
        if (e.type == SeqEvent::Type::Stop || e.cell < 0 || e.cell >= kit.numCells())
            return;

        const int note = juce::jlimit (0, 127, kit.cells[(size_t) e.cell].lowNote);
        const double ticks = e.ppq * (double) ticksPerQuarter;

        if (e.type == SeqEvent::Type::NoteOn)
        {
            const float vel = e.step != nullptr ? (float) e.step->velocity / 127.0f : 0.8f;
            notes.addEvent (juce::MidiMessage::noteOn (1, note, vel), ticks);
        }
        else
            notes.addEvent (juce::MidiMessage::noteOff (1, note), ticks);
    });

    // Una nota che inizia nell'ultimo step puo' finire dopo la fine della clip: si chiude al bordo
    notes.updateMatchedPairs();
    for (int i = 0; i < notes.getNumEvents(); ++i)
        if (auto* ev = notes.getEventPointer (i); ev->message.isNoteOn() && ev->noteOffObject == nullptr)
            notes.addEvent (juce::MidiMessage::noteOff (1, ev->message.getNoteNumber()), endPpq * (double) ticksPerQuarter);

    notes.sort();
    notes.updateMatchedPairs();
    file.addTrack (notes);
    return file;
}

bool writeMidiFile (const juce::MidiFile& midi, const juce::File& file)
{
    file.deleteFile();
    juce::FileOutputStream out (file);
    return out.openedOk() && midi.writeTo (out);
}

Pattern isolateLane (const Pattern& pattern, int lane)
{
    Pattern p = pattern;
    for (int i = 0; i < (int) p.lanes.size(); ++i)
    {
        p.lanes[(size_t) i].mute = (i != lane);
        p.lanes[(size_t) i].solo = false;
    }
    return p;
}

juce::AudioBuffer<float> renderBounce (Engine& engine, const Pattern& pattern, int bars, double bpm, double sr, TailMode tail)
{
    const int oneRun = (int) std::llround ((double) juce::jmax (1, bars) * 4.0 * 60.0 / bpm * sr);   // lunghezza esatta in campioni

    if (tail == TailMode::Keep)
        return renderOffline (engine, pattern, bars, bpm, sr, 4.0);

    if (tail == TailMode::Cut)
    {
        auto full = renderOffline (engine, pattern, bars, bpm, sr, 0.0);
        juce::AudioBuffer<float> out (2, oneRun);
        out.clear();
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, 0, full, c, 0, juce::jmin (oneRun, full.getNumSamples()));
        return out;
    }

    // Loop: si suona il pattern due volte e si tiene la seconda: contiene la coda della prima all'inizio (stato "a regime")
    auto twice = renderOffline (engine, pattern, juce::jmax (1, bars) * 2, bpm, sr, 0.0);
    juce::AudioBuffer<float> out (2, oneRun);
    out.clear();
    const int available = juce::jmax (0, twice.getNumSamples() - oneRun);
    for (int c = 0; c < 2; ++c)
        out.copyFrom (c, 0, twice, c, oneRun, juce::jmin (oneRun, available));
    return out;
}

juce::AudioBuffer<float> renderOffline (Engine& engine, const Pattern& pattern, int bars, double bpm, double sr, double tailSeconds)
{
    const double songBeats = (double) juce::jmax (1, bars) * 4.0;
    return renderSegments (engine, { Segment { &pattern, 0.0, songBeats } }, 0.0, songBeats, bpm, sr, tailSeconds);
}

juce::AudioBuffer<float> renderSegments (Engine& engine, const std::vector<Segment>& segs, double period, double songBeats, double bpm,
                                         double sr, double tailSeconds)
{
    constexpr int block = 512;
    const double songSeconds = songBeats * 60.0 / bpm;
    const int totalSamples = (int) ((songSeconds + tailSeconds) * sr);

    juce::AudioBuffer<float> out (2, totalSamples);
    out.clear();

    SequencerRunner runner;
    juce::MidiBuffer noMidi;
    double ppq = 0.0;

    for (int pos = 0; pos < totalSamples; pos += block)
    {
        const int n = juce::jmin (block, totalSamples - pos);
        juce::AudioBuffer<float> blk (2, n);
        blk.clear();

        const bool playing = ppq < songBeats;               // dopo la fine restano solo le code
        runner.runBlockSong (segs, period, playing, ppq, bpm, sr, n, songBeats);     // nessun evento oltre la fine
        engine.process (blk, noMidi, &runner.getEvents());
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, pos, blk, c, 0, n);
        if (playing)
            ppq += (double) n * bpm / (60.0 * sr);
    }
    return out;
}

juce::AudioBuffer<float> renderBounceSegments (Engine& engine, const std::vector<Segment>& segs, double totalBeats, double bpm, double sr,
                                               TailMode tail)
{
    const int oneRun = (int) std::llround (totalBeats * 60.0 / bpm * sr);

    if (tail == TailMode::Keep)
        return renderSegments (engine, segs, 0.0, totalBeats, bpm, sr, 4.0);

    if (tail == TailMode::Cut)
    {
        auto full = renderSegments (engine, segs, 0.0, totalBeats, bpm, sr, 0.0);
        juce::AudioBuffer<float> out (2, oneRun);
        out.clear();
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, 0, full, c, 0, juce::jmin (oneRun, full.getNumSamples()));
        return out;
    }

    // Loop: la song suonata due volte (timeline periodica), si tiene la seconda: contiene la coda della prima all'inizio
    auto twice = renderSegments (engine, segs, totalBeats, totalBeats * 2.0, bpm, sr, 0.0);
    juce::AudioBuffer<float> out (2, oneRun);
    out.clear();
    const int available = juce::jmax (0, twice.getNumSamples() - oneRun);
    for (int c = 0; c < 2; ++c)
        out.copyFrom (c, 0, twice, c, oneRun, juce::jmin (oneRun, available));
    return out;
}
}
