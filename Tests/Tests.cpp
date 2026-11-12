#include "engine/Engine.h"
#include "kit/KitIO.h"

namespace
{
using namespace sl;

constexpr double kSR = 44100.0;

// Campione sinusoidale mono di test
SampleDataPtr makeSine (double freq, double seconds, float amp = 1.0f)
{
    auto d = std::make_shared<SampleData>();
    const int n = (int) (seconds * kSR);
    d->audio.setSize (1, n);
    for (int i = 0; i < n; ++i)
        d->audio.setSample (0, i, amp * (float) std::sin (2.0 * juce::MathConstants<double>::pi * freq * i / kSR));
    d->sampleRate = kSR;
    d->name = "sine";
    return d;
}

// Rampa costante 1.0 (per verificare start/end e lunghezza)
SampleDataPtr makeDc (double seconds)
{
    auto d = std::make_shared<SampleData>();
    const int n = (int) (seconds * kSR);
    d->audio.setSize (1, n);
    for (int i = 0; i < n; ++i)
        d->audio.setSample (0, i, 1.0f);
    d->sampleRate = kSR;
    return d;
}

struct Rig
{
    Engine engine;
    std::vector<SampleDataPtr> keep;

    Rig() { engine.prepare (kSR); }

    void setCells (const std::vector<std::pair<CellParams, SampleDataPtr>>& cells)
    {
        auto snap = std::make_shared<KitSnapshot>();
        int i = 0;
        for (auto& [p, d] : cells)
        {
            keep.push_back (d);
            CellRuntime r;
            r.cellIndex = i++;
            r.dsp = toDsp (p);
            r.data = d.get();
            snap->cells.push_back (r);
        }
        engine.publish (snap);
    }

    // Renderizza 'blocks' blocchi da 'bs' campioni con i MIDI dati nel primo blocco.
    juce::AudioBuffer<float> render (int totalSamples, const juce::MidiBuffer& midiFirst, int bs = 256)
    {
        juce::AudioBuffer<float> out (2, totalSamples);
        out.clear();
        for (int pos = 0; pos < totalSamples; pos += bs)
        {
            const int n = juce::jmin (bs, totalSamples - pos);
            juce::AudioBuffer<float> blk (2, n);
            blk.clear();
            // Gli eventi MIDI hanno posizioni assolute: ogni blocco riceve quelli che lo riguardano
            juce::MidiBuffer m;
            for (const auto meta : midiFirst)
                if (meta.samplePosition >= pos && meta.samplePosition < pos + n)
                    m.addEvent (meta.getMessage(), meta.samplePosition - pos);
            engine.process (blk, m);
            for (int c = 0; c < 2; ++c)
                out.copyFrom (c, pos, blk, c, 0, n);
        }
        return out;
    }
};

juce::MidiBuffer noteOn (int note, int at = 0, float vel = 1.0f)
{
    juce::MidiBuffer m;
    m.addEvent (juce::MidiMessage::noteOn (1, note, vel), at);
    return m;
}

CellParams cell (int note, TriggerMode mode = TriggerMode::OneShot)
{
    CellParams c;
    c.rootNote = c.lowNote = c.highNote = note;
    c.mode = mode;
    c.attack = 0.0f;
    c.decay = 0.0f;
    c.sustain = 1.0f;
    return c;
}

float peak (const juce::AudioBuffer<float>& b, int from, int to)
{
    float p = 0.0f;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = from; i < to; ++i)
            p = juce::jmax (p, std::abs (b.getSample (c, i)));
    return p;
}

int zeroCrossings (const juce::AudioBuffer<float>& b, int from, int to)
{
    int z = 0;
    for (int i = from + 1; i < to; ++i)
        if ((b.getSample (0, i - 1) < 0.0f) != (b.getSample (0, i) < 0.0f))
            ++z;
    return z;
}
}

class SampleLabTests final : public juce::UnitTest
{
public:
    SampleLabTests() : juce::UnitTest ("SampleLab", "SampleLab") {}

    void runTest() override
    {
        beginTest ("Silence without a note");
        {
            Rig r;
            r.setCells ({ { cell (36), makeSine (440, 1.0) } });
            auto out = r.render (4096, {});
            expectEquals (peak (out, 0, 4096), 0.0f);
        }

        beginTest ("Note on in range plays, out of range is silent");
        {
            Rig r;
            r.setCells ({ { cell (36), makeSine (440, 1.0) } });
            expect (peak (r.render (4096, noteOn (36)), 0, 4096) > 0.3f);

            Rig r2;
            r2.setCells ({ { cell (36), makeSine (440, 1.0) } });
            expectEquals (peak (r2.render (4096, noteOn (37)), 0, 4096), 0.0f);
        }

        beginTest ("Level in dB scales the output");
        {
            auto p = cell (36);
            p.levelDb = -6.0f;
            p.pan = 0.0f;
            Rig r;
            r.setCells ({ { p, makeDc (1.0) } });
            auto out = r.render (4096, noteOn (36));
            expectWithinAbsoluteError (out.getSample (0, 2000), std::pow (10.0f, -6.0f / 20.0f), 0.01f);
        }

        beginTest ("Velocity scales the output");
        {
            Rig r;
            r.setCells ({ { cell (36), makeDc (1.0) } });
            auto out = r.render (4096, noteOn (36, 0, 0.5f));
            expectWithinAbsoluteError (out.getSample (0, 2000), 0.5f, 0.02f);
        }

        beginTest ("Start/end trim the played region (one-shot length)");
        {
            auto p = cell (36);
            p.start = 0.25f;
            p.end = 0.5f;                       // 0.25 s di un campione da 1 s
            Rig r;
            r.setCells ({ { p, makeDc (1.0) } });
            auto out = r.render ((int) kSR, noteOn (36));
            const int expected = (int) (0.25 * kSR);
            expect (peak (out, 0, expected - 200) > 0.9f);
            expectEquals (peak (out, expected + 100, (int) kSR), 0.0f);
        }

        beginTest ("Pitch +12 semitones doubles the frequency");
        {
            Rig r0, r1;
            r0.setCells ({ { cell (36), makeSine (200, 1.0) } });
            auto p = cell (36);
            p.pitchSemis = 12.0f;
            r1.setCells ({ { p, makeSine (200, 1.0) } });
            auto a = r0.render (22050, noteOn (36));
            auto b = r1.render (22050, noteOn (36));
            const int za = zeroCrossings (a, 100, 10000), zb = zeroCrossings (b, 100, 10000);
            expect (std::abs ((double) zb / (double) za - 2.0) < 0.05, "zc ratio " + juce::String ((double) zb / za));
        }

        beginTest ("Browser preview plays without a kit and stops on demand");
        {
            Rig r;                                          // nessun kit pubblicato
            auto s = makeSine (440, 1.0);
            r.engine.playPreview (s.get(), 1.0f);
            expect (peak (r.render (2048, {}), 0, 2048) > 0.3f);

            r.engine.stopPreview();
            r.render (256, {});                             // il comando viene recepito
            expectEquals (peak (r.render (2048, {}), 0, 2048), 0.0f);
        }

        beginTest ("Tune +100 cents equals pitch +1 semitone");
        {
            auto p1 = cell (36);
            p1.tuneCents = 100.0f;
            auto p2 = cell (36);
            p2.pitchSemis = 1.0f;
            Rig a, b, base;
            a.setCells ({ { p1, makeSine (200, 1.0) } });
            b.setCells ({ { p2, makeSine (200, 1.0) } });
            base.setCells ({ { cell (36), makeSine (200, 1.0) } });
            auto oa = a.render (22050, noteOn (36));
            auto ob = b.render (22050, noteOn (36));
            auto o0 = base.render (22050, noteOn (36));
            const int za = zeroCrossings (oa, 100, 20000), zb = zeroCrossings (ob, 100, 20000), z0 = zeroCrossings (o0, 100, 20000);
            expect (std::abs (za - zb) <= 1, "tune zc " + juce::String (za) + " vs pitch zc " + juce::String (zb));
            expect (za > z0, "tune should raise the frequency");
        }

        beginTest ("Changing a parameter while a voice plays updates it live");
        {
            Rig r;
            auto p = cell (36, TriggerMode::Loop);
            auto sine = makeSine (200, 1.0);
            r.setCells ({ { p, sine } });
            r.render (4096, noteOn (36));                       // la voce e' attiva
            const int before = zeroCrossings (r.render (8192, {}), 100, 8000);

            auto p2 = p;
            p2.pitchSemis = 12.0f;                              // +1 ottava mentre suona
            r.setCells ({ { p2, sine } });
            r.render (512, {});                                 // il cambio viene recepito
            const int after = zeroCrossings (r.render (8192, {}), 100, 8000);
            expect (after > before * 3 / 2, "live pitch: before " + juce::String (before) + " after " + juce::String (after));
        }

        beginTest ("Key range transposes relative to root");
        {
            auto p = cell (36);
            p.lowNote = 36;
            p.highNote = 48;
            Rig r;
            r.setCells ({ { p, makeSine (200, 1.0) } });
            auto a = r.render (22050, noteOn (36));
            Rig r2;
            r2.setCells ({ { p, makeSine (200, 1.0) } });
            auto b = r2.render (22050, noteOn (48));   // +12
            const double ratio = (double) zeroCrossings (b, 100, 10000) / (double) zeroCrossings (a, 100, 10000);
            expect (std::abs (ratio - 2.0) < 0.05);
        }

        beginTest ("Reverse plays the sample backwards");
        {
            auto d = std::make_shared<SampleData>();
            d->audio.setSize (1, 1000);
            for (int i = 0; i < 1000; ++i)
                d->audio.setSample (0, i, (float) i / 1000.0f);   // rampa crescente
            d->sampleRate = kSR;

            auto p = cell (36);
            p.reverse = true;
            Rig r;
            r.setCells ({ { p, d } });
            auto out = r.render (512, noteOn (36));
            expect (out.getSample (0, 5) > out.getSample (0, 300), "reverse should be decreasing");
        }

        beginTest ("One-shot ignores note off, gate releases");
        {
            Rig oneShot, gate;
            oneShot.setCells ({ { cell (36, TriggerMode::OneShot), makeSine (440, 2.0) } });
            gate.setCells ({ { cell (36, TriggerMode::Gate), makeSine (440, 2.0) } });

            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), 0);
            m.addEvent (juce::MidiMessage::noteOff (1, 36), 100);

            auto a = oneShot.render (8192, m);
            auto b = gate.render (8192, m);
            expect (peak (a, 6000, 8000) > 0.3f, "one-shot should keep playing");
            expectEquals (peak (b, 6000, 8000), 0.0f);   // release 20 ms = ~880 campioni
        }

        beginTest ("Loop keeps playing past the sample end until note off");
        {
            Rig r;
            r.setCells ({ { cell (36, TriggerMode::Loop), makeSine (440, 0.05) } });   // 2205 campioni
            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), 0);
            m.addEvent (juce::MidiMessage::noteOff (1, 36), 20000);
            auto out = r.render (22050, m);
            expect (peak (out, 10000, 19000) > 0.3f);
            expectEquals (peak (out, 21500, 22050), 0.0f);
        }

        beginTest ("Layers: two cells on the same note both sound");
        {
            Rig r;
            r.setCells ({ { cell (36), makeDc (1.0) }, { cell (36), makeDc (1.0) } });
            auto out = r.render (2048, noteOn (36));
            expectWithinAbsoluteError (out.getSample (0, 1000), 2.0f, 0.05f);
        }

        beginTest ("Activity reports active voices per cell");
        {
            Rig r;
            r.setCells ({ { cell (36), makeSine (440, 1.0) }, { cell (37), makeSine (440, 1.0) } });
            r.render (1024, noteOn (37));
            expectEquals (r.engine.getCellActivity (0), 0);
            expectEquals (r.engine.getCellActivity (1), 1);
        }

        beginTest ("Voice stealing never exceeds the pool");
        {
            Rig r;
            r.setCells ({ { cell (36, TriggerMode::Loop), makeSine (440, 1.0) } });
            juce::MidiBuffer m;
            for (int i = 0; i < 100; ++i)
                m.addEvent (juce::MidiMessage::noteOn (1, 36, 1.0f), i);
            r.render (1024, m);
            expectEquals (r.engine.getCellActivity (0), (int) Engine::kMaxVoices);
        }

        beginTest ("Default kit maps C1 upwards in order");
        {
            auto k = Kit::makeDefault();
            expectEquals (k.numCells(), 48);
            expectEquals (k.cells[0].lowNote, 36);
            expectEquals (k.cells[11].lowNote, 47);
            expectEquals (k.cells[12].lowNote, 48);   // inizio seconda riga
            expectEquals (k.cells[47].lowNote, 83);
        }

        beginTest ("Kit save/load round trip with same-name folder and sample copy");
        {
            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("samplelab_test_" + juce::String (juce::Random::getSystemRandom().nextInt (1000000)));
            tmp.createDirectory();

            // Crea un WAV sorgente fuori dalla cartella del kit
            auto src = tmp.getChildFile ("source").getChildFile ("Kick Funk 7.wav");
            src.getParentDirectory().createDirectory();
            {
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::OutputStream> os (src.createOutputStream());
                std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), kSR, 1, 16, {}, 0));
                if (w != nullptr)
                {
                    os.release();
                    auto s = makeSine (100, 0.1);
                    w->writeFromAudioSampleBuffer (s->audio, 0, s->numSamples());
                }
            }
            expect (src.existsAsFile());

            auto kit = Kit::makeDefault();
            kit.cells[3].samplePath = src.getFullPathName();
            kit.cells[3].name = "Kick";
            kit.cells[3].levelDb = -3.0f;
            kit.cells[3].start = 0.1f;
            kit.cells[3].mode = TriggerMode::Gate;
            kit.cells[3].lowNote = 40;
            kit.cells[3].highNote = 44;

            juce::String err;
            auto kitFile = tmp.getChildFile ("kits").getChildFile ("MioKit.slkit");
            kitFile.getParentDirectory().createDirectory();
            expect (KitIO::save (kit, kitFile, err), err);

            expect (kitFile.existsAsFile());
            expect (kitFile.getSiblingFile ("MioKit").isDirectory());                       // cartella con lo stesso nome
            expect (kitFile.getSiblingFile ("MioKit").getChildFile ("Kick Funk 7.wav").existsAsFile());
            expectEquals (kit.name, juce::String ("MioKit"));

            Kit loaded;
            expect (KitIO::load (kitFile, loaded, err), err);
            expectEquals (loaded.name, juce::String ("MioKit"));
            expectEquals (loaded.numCells(), 48);
            expectEquals (loaded.cells[3].name, juce::String ("Kick"));
            expectWithinAbsoluteError (loaded.cells[3].levelDb, -3.0f, 0.001f);
            expectWithinAbsoluteError (loaded.cells[3].start, 0.1f, 0.001f);
            expect (loaded.cells[3].mode == TriggerMode::Gate);
            expectEquals (loaded.cells[3].lowNote, 40);
            expectEquals (loaded.cells[3].highNote, 44);
            expect (! loaded.cells[3].missing);
            expect (juce::File (loaded.cells[3].samplePath).getParentDirectory() == kitFile.getSiblingFile ("MioKit"));
            expect (juce::JSON::parse (kitFile.loadFileAsString())["cells"][3]["sample"].toString() == "Kick Funk 7.wav");

            // Se la cartella dei sample viene spostata, il sample risulta mancante
            kitFile.getSiblingFile ("MioKit").getChildFile ("Kick Funk 7.wav").deleteFile();
            Kit missing;
            expect (KitIO::load (kitFile, missing, err), err);
            expect (missing.cells[3].missing);

            // Un kit di una versione futura viene rifiutato
            auto future = tmp.getChildFile ("future.slkit");
            future.replaceWithText ("{\"schemaVersion\": 99, \"cells\": []}");
            Kit k2;
            expect (! KitIO::load (future, k2, err));

            tmp.deleteRecursively();
        }

        beginTest ("File names are sanitised");
        {
            expect (KitIO::sanitiseName ("a/b:c*").containsOnly ("abc_-") || ! KitIO::sanitiseName ("a/b:c*").containsAnyOf ("/:*"));
            expectEquals (KitIO::sanitiseName ("   "), juce::String ("Untitled"));
        }
    }
};

int main()
{
    struct StdoutLogger final : juce::Logger
    {
        void logMessage (const juce::String& m) override { std::printf ("%s\n", m.toRawUTF8()); }
    } logger;
    juce::Logger::setCurrentLogger (&logger);

    juce::ScopedJuceInitialiser_GUI init;
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    static SampleLabTests tests;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    std::printf ("\n%s (%d failures)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", failures);
    return failures == 0 ? 0 : 1;
}
