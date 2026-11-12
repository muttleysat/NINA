#include "engine/Engine.h"
#include "kit/KitIO.h"
#include "params/ParamSet.h"
#include <set>

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

    void setCells (const std::vector<std::pair<CellParams, SampleDataPtr>>& cells, const FxParams& masterFx = {})
    {
        auto snap = std::make_shared<KitSnapshot>();
        engine.live = Engine::LiveParams {};
        engine.live.masterFx = masterFx;
        int i = 0;
        for (auto& [p, d] : cells)
        {
            keep.push_back (d);
            CellRuntime r;
            r.cellIndex = i;
            r.data = d.get();
            r.rootNote = p.rootNote;
            r.lowNote = p.lowNote;
            r.highNote = p.highNote;
            snap->cells.push_back (r);

            engine.live.dsp[(size_t) i] = toDsp (p);     // i valori automatizzabili vanno nei parametri live
            engine.live.fx[(size_t) i] = p.fx;
            ++i;
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

        // ------------------------------------------------------------ Effetti

        auto impulse = [] (int length = 100)
        {
            auto d = std::make_shared<SampleData>();
            d->audio.setSize (1, length);
            d->audio.clear();
            d->audio.setSample (0, 0, 1.0f);
            d->sampleRate = kSR;
            return SampleDataPtr (d);
        };

        beginTest ("FX: effects are bypassed by default (output identical)");
        {
            Rig a, b;
            auto s = makeSine (440, 1.0);
            a.setCells ({ { cell (36), s } });
            auto p = cell (36);
            p.fx.filterOn = false;
            b.setCells ({ { p, s } });
            auto oa = a.render (4096, noteOn (36)), ob = b.render (4096, noteOn (36));
            float maxDiff = 0.0f;
            for (int i = 0; i < 4096; ++i)
                maxDiff = juce::jmax (maxDiff, std::abs (oa.getSample (0, i) - ob.getSample (0, i)));
            expectEquals (maxDiff, 0.0f);
        }

        beginTest ("FX: low-pass filter attenuates highs, high-pass attenuates lows");
        {
            auto lp = cell (36);
            lp.fx.filterOn = true; lp.fx.filterType = 0; lp.fx.cutoff = 300.0f;
            Rig r1; r1.setCells ({ { lp, makeSine (8000, 1.0) } });
            expect (peak (r1.render (8192, noteOn (36)), 4096, 8192) < 0.1f, "LP should kill 8 kHz");

            auto hp = cell (36);
            hp.fx.filterOn = true; hp.fx.filterType = 1; hp.fx.cutoff = 8000.0f;
            Rig r2; r2.setCells ({ { hp, makeSine (100, 1.0) } });
            expect (peak (r2.render (8192, noteOn (36)), 4096, 8192) < 0.1f, "HP should kill 100 Hz");

            Rig r3; r3.setCells ({ { lp, makeSine (100, 1.0) } });
            expect (peak (r3.render (8192, noteOn (36)), 4096, 8192) > 0.8f, "LP should pass 100 Hz");
        }

        beginTest ("FX: saturation raises low-level signals toward full scale");
        {
            auto p = cell (36);
            p.fx.satOn = true; p.fx.drive = 24.0f; p.fx.satMix = 1.0f;
            Rig r; r.setCells ({ { p, makeSine (200, 1.0, 0.1f) } });
            expect (peak (r.render (4096, noteOn (36)), 1000, 4096) > 0.8f);
        }

        beginTest ("FX: LoFi quantises to few distinct levels");
        {
            auto p = cell (36);
            p.fx.lofiOn = true; p.fx.bits = 2.0f; p.fx.downsample = 1.0f; p.fx.lofiMix = 1.0f;
            Rig r; r.setCells ({ { p, makeSine (200, 1.0, 0.9f) } });
            auto out = r.render (4096, noteOn (36));
            std::set<int> levels;
            for (int i = 100; i < 4096; ++i)
                levels.insert ((int) std::round (out.getSample (0, i) * 1000.0f));
            expect (levels.size() <= 5, "distinct levels: " + juce::String ((int) levels.size()));
        }

        beginTest ("FX: compressor reduces a loud signal");
        {
            auto p = cell (36);
            p.fx.compOn = true; p.fx.compThreshold = -30.0f; p.fx.compRatio = 20.0f; p.fx.compAttack = 1.0f; p.fx.compRelease = 50.0f;
            Rig r; r.setCells ({ { p, makeSine (200, 1.0, 1.0f) } });
            expect (peak (r.render (22050, noteOn (36)), 15000, 22050) < 0.3f);
        }

        beginTest ("FX: cell delay echoes and keeps sounding after the voice ended (tail)");
        {
            auto p = cell (36);
            p.fx.delayOn = true; p.fx.delayTime = 100.0f; p.fx.delayFeedback = 0.0f; p.fx.delayMix = 1.0f;
            Rig r; r.setCells ({ { p, impulse() } });          // la voce finisce dopo 100 campioni
            auto out = r.render (8192, noteOn (36));
            expect (peak (out, 200, 4000) < 0.05f, "silence between dry hit and echo");
            expect (peak (out, 4350, 4470) > 0.8f, "echo at 100 ms");
        }

        beginTest ("FX: delay time in Sync follows the tempo (1/16 at 120 and 60 BPM)");
        {
            auto p = cell (36);
            p.fx.delayOn = true; p.fx.delaySync = true; p.fx.delayDivision = 10;      // 1/16
            p.fx.delayFeedback = 0.0f; p.fx.delayMix = 1.0f;

            Rig a; a.setCells ({ { p, impulse() } });
            a.engine.live.bpm = 120.0;                      // 1/16 = 125 ms = 5512 campioni
            auto oa = a.render (16000, noteOn (36));
            // 5512.5 campioni: l'impulso di un campione si divide a meta' tra due campioni (interpolazione) -> picco ~0.5
            expect (peak (oa, 5450, 5580) > 0.4f, "echo at 125 ms");
            expect (peak (oa, 200, 5300) < 0.05f);

            Rig b; b.setCells ({ { p, impulse() } });
            b.engine.live.bpm = 60.0;                       // 250 ms = 11025 campioni
            auto ob = b.render (16000, noteOn (36));
            expect (peak (ob, 10960, 11090) > 0.8f, "echo at 250 ms");
            expect (peak (ob, 5450, 5580) < 0.05f, "no echo at the 120 BPM position");
        }

        beginTest ("FX: delay time helper (divisions, tempo, clamp)");
        {
            FxParams f;
            f.delaySync = true;
            f.delayDivision = 4;                            // 1/4
            expectWithinAbsoluteError (FxChain::delayMs (f, 120.0), 500.0f, 0.01f);
            f.delayDivision = 9;                            // 1/8T = 1/3 beat
            expectWithinAbsoluteError (FxChain::delayMs (f, 90.0), 222.2f, 0.2f);
            f.delayDivision = 0;                            // 1/1 a 20 BPM = 12 s: limitato al massimo
            expectEquals (FxChain::delayMs (f, 20.0), FxChain::kMaxDelayMs);
            f.delaySync = false;
            f.delayTime = 333.0f;
            expectEquals (FxChain::delayMs (f, 120.0), 333.0f);
        }

        beginTest ("FX: reverb tail outlasts the sample");
        {
            auto p = cell (36);
            p.fx.reverbOn = true; p.fx.reverbSize = 0.8f; p.fx.reverbMix = 1.0f;
            Rig r; r.setCells ({ { p, impulse() } });
            auto out = r.render (30000, noteOn (36));
            expect (peak (out, 15000, 30000) > 0.0005f, "reverb tail should still be audible");
        }

        beginTest ("FX: master chain processes the whole mix");
        {
            FxParams m;
            m.filterOn = true; m.filterType = 0; m.cutoff = 300.0f;
            Rig r; r.setCells ({ { cell (36), makeSine (8000, 1.0) } }, m);
            expect (peak (r.render (8192, noteOn (36)), 4096, 8192) < 0.1f);
        }

        beginTest ("FX: parameters round-trip through the kit file");
        {
            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("samplelab_fx_" + juce::String (juce::Random::getSystemRandom().nextInt (1000000)));
            tmp.createDirectory();
            auto kit = Kit::makeDefault();
            kit.cells[2].fx.filterOn = true;
            kit.cells[2].fx.cutoff = 1234.0f;
            kit.cells[2].fx.filterType = 2;
            kit.cells[2].fx.delayOn = true;
            kit.cells[2].fx.delayTime = 333.0f;
            kit.masterFx.reverbOn = true;
            kit.masterFx.reverbSize = 0.9f;

            juce::String err;
            auto f = tmp.getChildFile ("Fx.slkit");
            expect (KitIO::save (kit, f, err), err);
            Kit loaded;
            expect (KitIO::load (f, loaded, err), err);
            expect (loaded.cells[2].fx.filterOn);
            expectEquals (loaded.cells[2].fx.filterType, 2);
            expectWithinAbsoluteError (loaded.cells[2].fx.cutoff, 1234.0f, 0.01f);
            expect (loaded.cells[2].fx.delayOn);
            expectWithinAbsoluteError (loaded.cells[2].fx.delayTime, 333.0f, 0.01f);
            expect (! loaded.cells[3].fx.anyOn());
            expect (loaded.masterFx.reverbOn);
            expectWithinAbsoluteError (loaded.masterFx.reverbSize, 0.9f, 0.001f);
            tmp.deleteRecursively();
        }

        // ------------------------------------------------- Parametri / automazione

        // Un ParamSet con i suoi gruppi, tenuti in vita come farebbe il processore
        struct ParamRig
        {
            ParamSet set;
            juce::AudioProcessorParameterGroup root;
            ParamRig()
            {
                for (auto& g : set.releaseGroups())
                    root.addChild (std::move (g));
            }
            juce::RangedAudioParameter* byId (const juce::String& id)
            {
                for (auto* p : root.getParameters (true))
                    if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p); r != nullptr && r->getParameterID() == id)
                        return r;
                return nullptr;
            }
        };

        beginTest ("Params: every control is exposed with a unique stable ID");
        {
            ParamRig pr;
            auto all = pr.root.getParameters (true);
            expectEquals ((int) all.size(), 48 * 40 + 28);     // 48 celle x 40 + master 28

            std::set<juce::String> ids;
            for (auto* p : all)
                if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                    ids.insert (r->getParameterID());
            expectEquals ((int) ids.size(), (int) all.size());

            // ID che non devono mai cambiare tra le versioni (retrocompatibilita' dei progetti)
            for (auto* id : { "c00.level", "c00.pan", "c00.start", "c00.end", "c00.attack", "c00.reverse", "c00.mode",
                              "c00.filter.cutoff", "c00.filter.type", "c00.filter.on", "c00.delay.time", "c47.reverb.mix",
                              "c11.comp.threshold", "master.level", "master.filter.cutoff",
                              "c00.delay.sync", "c00.delay.division" })
                expect (pr.byId (id) != nullptr, juce::String ("missing id ") + id);
        }

        beginTest ("Params: names are readable (no CC numbers) and values carry units");
        {
            ParamRig pr;
            auto* cutoff = pr.byId ("c00.filter.cutoff");
            expectEquals (cutoff->getName (100), juce::String ("A1 Filter Cutoff"));
            expectEquals (pr.byId ("c13.level")->getName (100), juce::String ("B2 Level"));
            expectEquals (pr.byId ("c00.mode")->getName (100), juce::String ("A1 Trigger Mode"));
            expectEquals (pr.byId ("master.reverb.mix")->getName (100), juce::String ("Master Reverb Mix"));

            cutoff->setValueNotifyingHost (cutoff->convertTo0to1 (2400.0f));
            expectEquals (cutoff->getText (cutoff->getValue(), 20), juce::String ("2400 Hz"));
            expectEquals (pr.byId ("c00.mode")->getText (1.0f / 2.0f, 20), juce::String ("Gate"));
            expectEquals (pr.byId ("c00.delay.on")->getText (1.0f, 20), juce::String ("On"));
        }

        beginTest ("Params: parameters are grouped by cell and effect");
        {
            ParamRig pr;
            // Il gruppo radice contiene un gruppo per cella piu' il master
            int groups = 0;
            for (auto* node : pr.root)
                if (node->getGroup() != nullptr)
                    ++groups;
            expectEquals (groups, 49);

            // Il cutoff del filtro della cella A1 sta in "Cell A1" > "A1 Filter"
            juce::StringArray path;
            for (auto* g : pr.root.getGroupsForParameter (pr.byId ("c00.filter.cutoff")))
                path.add (g->getName());
            expect (path.contains ("Cell A1"), "path: " + path.joinIntoString (" > "));
            expect (path.contains ("A1 Filter"), "path: " + path.joinIntoString (" > "));
        }

        beginTest ("Params: audio thread sees automated values (live params)");
        {
            ParamRig pr;
            auto set = [&] (const char* id, float v) { auto* p = pr.byId (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
            set ("c02.level", -6.0f);
            set ("c02.pan", 0.5f);
            set ("c02.filter.cutoff", 1000.0f);
            set ("c02.filter.on", 1.0f);
            set ("c02.filter.type", 2.0f);
            set ("c02.mode", 1.0f);
            set ("c02.reverse", 1.0f);
            set ("c02.delay.on", 1.0f);
            set ("master.level", -12.0f);
            set ("master.reverb.on", 1.0f);

            Engine::LiveParams live;
            pr.set.fillLive (live);
            expectWithinAbsoluteError (live.dsp[2].gain, std::pow (10.0f, -6.0f / 20.0f), 0.001f);
            expectWithinAbsoluteError (live.dsp[2].pan, 0.5f, 0.01f);
            expect (live.dsp[2].mode == TriggerMode::Gate);
            expect (live.dsp[2].reverse);
            expect (live.fx[2].filterOn);
            expectEquals (live.fx[2].filterType, 2);
            expectWithinAbsoluteError (live.fx[2].cutoff, 1000.0f, 1.0f);
            expect (live.fx[2].delayOn);
            expect (! live.fx[3].filterOn);                     // le altre celle non cambiano
            expect (live.masterFx.reverbOn);
            expectWithinAbsoluteError (live.masterGain, std::pow (10.0f, -12.0f / 20.0f), 0.001f);
        }

        beginTest ("Params: a live parameter change is heard (cutoff automation on a playing cell)");
        {
            ParamRig pr;
            Rig r;
            auto sine = makeSine (8000, 2.0);
            auto p = cell (36, TriggerMode::Loop);
            r.setCells ({ { p, sine } });

            pr.set.fillLive (r.engine.live);                    // tutto spento: passa
            // Dopo setCells i valori live sono quelli del test; riporta i parametri nel motore
            r.engine.live.dsp[0] = toDsp (p);
            r.render (4096, noteOn (36));
            expect (peak (r.render (4096, {}), 0, 4096) > 0.5f);

            auto set = [&] (const char* id, float v) { auto* q = pr.byId (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
            set ("c00.filter.on", 1.0f);
            set ("c00.filter.cutoff", 200.0f);
            Engine::LiveParams live = r.engine.live;
            pr.set.fillLive (live);
            r.engine.live.fx[0] = live.fx[0];                   // come farebbe il processore a ogni blocco
            r.render (2048, {});
            expect (peak (r.render (4096, {}), 0, 4096) < 0.1f, "8 kHz must be filtered after automating the cutoff");
        }

        beginTest ("Params: kit values round-trip through the parameters");
        {
            ParamRig pr;
            auto kit = Kit::makeDefault();
            kit.cells[5].levelDb = -9.0f;
            kit.cells[5].pitchSemis = 7.0f;
            kit.cells[5].mode = TriggerMode::Loop;
            kit.cells[5].reverse = true;
            kit.cells[5].fx.delayOn = true;
            kit.cells[5].fx.delayTime = 333.0f;
            kit.cells[5].fx.delaySync = true;
            kit.cells[5].fx.delayDivision = 10;
            kit.cells[5].fx.filterType = 1;
            kit.masterDb = -3.0f;
            kit.masterFx.compOn = true;
            kit.masterFx.compRatio = 8.0f;

            pr.set.readFromKit (kit);

            auto fresh = Kit::makeDefault();
            pr.set.writeToKit (fresh);
            expectWithinAbsoluteError (fresh.cells[5].levelDb, -9.0f, 0.06f);
            expectWithinAbsoluteError (fresh.cells[5].pitchSemis, 7.0f, 0.01f);
            expect (fresh.cells[5].mode == TriggerMode::Loop);
            expect (fresh.cells[5].reverse);
            expect (fresh.cells[5].fx.delayOn);
            expectWithinAbsoluteError (fresh.cells[5].fx.delayTime, 333.0f, 1.0f);
            expectEquals (fresh.cells[5].fx.filterType, 1);
            expect (fresh.cells[5].fx.delaySync);
            expectEquals (fresh.cells[5].fx.delayDivision, 10);
            expectWithinAbsoluteError (fresh.masterDb, -3.0f, 0.06f);
            expect (fresh.masterFx.compOn);
            expectWithinAbsoluteError (fresh.masterFx.compRatio, 8.0f, 0.06f);
            expect (! fresh.cells[6].fx.anyOn());
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
