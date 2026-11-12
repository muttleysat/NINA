#include "SequencerExport.h"

namespace sl::SequencerExport
{
juce::MidiFile toMidiFile (const Pattern& pattern, const Kit& kit, int bars, double bpm, int ticksPerQuarter)
{
    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);

    // Traccia 0: tempo e nome
    juce::MidiMessageSequence conductor;
    conductor.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / juce::jlimit (20.0, 400.0, bpm))), 0.0);
    conductor.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    conductor.addEvent (juce::MidiMessage::textMetaEvent (3, pattern.name), 0.0);
    conductor.updateMatchedPairs();
    file.addTrack (conductor);

    juce::MidiMessageSequence notes;
    const double endPpq = (double) juce::jmax (1, bars) * 4.0;

    Sequencer::generate (pattern, 0.0, endPpq, [&] (const TimedEvent& e)
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

juce::AudioBuffer<float> renderOffline (Engine& engine, const Pattern& pattern, int bars, double bpm, double sr, double tailSeconds)
{
    constexpr int block = 512;
    const double songBeats = (double) juce::jmax (1, bars) * 4.0;
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

        const bool playing = ppq < songBeats;               // dopo la fine del pattern restano solo le code
        runner.runBlock (&pattern, playing, ppq, bpm, sr, n, songBeats);     // nessun evento oltre la fine delle battute
        engine.process (blk, noMidi, &runner.getEvents());
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, pos, blk, c, 0, n);
        if (playing)
            ppq += (double) n * bpm / (60.0 * sr);
    }
    return out;
}
}
