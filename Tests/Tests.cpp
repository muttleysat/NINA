#include "engine/Engine.h"
#include "engine/LoopStretch.h"
#include "engine/SliceDetect.h"
#include "kit/KitIO.h"
#include "params/ParamSet.h"
#include "params/MidiLearn.h"
#include "sequencer/Sequencer.h"
#include "sequencer/SequencerExport.h"
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
            r.output = p.output;
            snap->cells.push_back (r);

            engine.live.dsp[(size_t) i] = toDsp (p);     // i valori automatizzabili vanno nei parametri live
            engine.live.fx[(size_t) i] = p.fx;
            ++i;
        }
        engine.publish (snap);
    }

    // Renderizza 'blocks' blocchi da 'bs' campioni con i MIDI dati nel primo blocco.
    juce::AudioBuffer<float> render (int totalSamples, const juce::MidiBuffer& midiFirst, int bs = 256,
                                     const std::vector<SeqEvent>& seqAbs = {})
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
            // Eventi del sequencer con posizione assoluta: ogni blocco riceve i suoi, con posizione relativa
            std::vector<SeqEvent> seqBlock;
            for (auto e : seqAbs)
                if (e.samplePos >= pos && e.samplePos < pos + n)
                {
                    e.samplePos -= pos;
                    seqBlock.push_back (e);
                }
            engine.process (blk, m, seqAbs.empty() ? nullptr : &seqBlock);
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

        // ------------------------------------------------------- Sequencer

        // Pattern con una lane sulla cella 0: steps = indici accesi
        auto makePattern = [] (int division, int length, std::initializer_list<int> on)
        {
            Pattern p;
            Lane l;
            l.cell = 0;
            l.division = division;
            l.length = length;
            for (int i : on)
                l.steps[(size_t) i].on = true;
            p.lanes.push_back (l);
            return p;
        };

        struct Ev { double ppq; SeqEvent::Type type; int lane; };
        auto collect = [] (const Pattern& p, double a, double b)
        {
            std::vector<Ev> out;
            Sequencer::generate (p, a, b, [&] (const TimedEvent& e) { out.push_back ({ e.ppq, e.type, e.lane }); });
            std::sort (out.begin(), out.end(), [] (const Ev& x, const Ev& y)
                       { return x.ppq != y.ppq ? x.ppq < y.ppq : (int) x.type < (int) y.type; });
            return out;
        };
        auto ons = [] (const std::vector<Ev>& v, int lane = -1)
        {
            std::vector<double> t;
            for (auto& e : v)
                if (e.type == SeqEvent::Type::NoteOn && (lane < 0 || e.lane == lane))
                    t.push_back (e.ppq);
            return t;
        };

        beginTest ("Sequencer: steps fire at the right musical time and the lane repeats");
        {
            auto p = makePattern (4, 4, { 0, 2 });                  // 1/4 = 1 beat, 4 step: step 0 e 2
            auto times = ons (collect (p, 0.0, 8.0));
            expectEquals ((int) times.size(), 4);
            const double expected[] = { 0.0, 2.0, 4.0, 6.0 };
            for (size_t i = 0; i < times.size(); ++i)
                expectWithinAbsoluteError (times[i], expected[i], 1e-9);

            auto sixteenths = ons (collect (makePattern (10, 16, { 3 }), 0.0, 4.0));   // 1/16: step 3 = 0.75
            expectEquals ((int) sixteenths.size(), 1);
            expectWithinAbsoluteError (sixteenths[0], 0.75, 1e-9);
        }

        beginTest ("Sequencer: recording snaps hits to the lane grid; quantize removes the nudge");
        {
            sl::Lane lane;                                          // 1/16, 16 step: uno step = 0.25 quarti
            const int i1 = sl::recordHit (lane, 1.02, 90, 0, 0.0f);          // vicino allo step 4 (1.0): quantizzato
            expectEquals (i1, 4);
            expect (lane.steps[4].on);
            expectEquals ((int) lane.steps[4].velocity, 90);
            expectWithinAbsoluteError (lane.steps[4].nudge, 0.0f, 1e-6f);

            const int i2 = sl::recordHit (lane, 1.30, 100, 3, 1.0f);         // 1.30 = step 5 (1.25) + 0.2 step: resta il micro-timing
            expectEquals (i2, 5);
            expectWithinAbsoluteError (lane.steps[5].nudge, 0.2f, 1e-4f);
            expectEquals ((int) lane.steps[5].pitch, 3);

            const int i3 = sl::recordHit (lane, 4.0 + 0.5, 100, 0, 0.0f);    // oltre il ciclo (16 step = 4 quarti): rientra a 0.5 = step 2
            expectEquals (i3, 2);

            sl::quantizeLane (lane, 0.5f);
            expectWithinAbsoluteError (lane.steps[5].nudge, 0.1f, 1e-4f);
            sl::quantizeLane (lane, 1.0f);
            expectWithinAbsoluteError (lane.steps[5].nudge, 0.0f, 1e-6f);
        }

        beginTest ("Song: blocks play in sequence, each pattern restarts its phase, held notes are released at the boundary");
        {
            std::vector<Pattern> pats (kNumPatterns);
            pats[0] = makePattern (4, 4, { 0 });                    // 1/4 x 4 step = 4 quarti, step 0
            pats[1] = makePattern (4, 4, { 2 });                    // step 2
            Song song;
            song.blocks = { { 0, 2 }, { 1, 1 } };                  // 8 quarti + 4 quarti
            expectWithinAbsoluteError (song.totalBeats (pats), 12.0, 1e-9);
            expectEquals (song.blockAt (pats, 7.9), 0);
            expectEquals (song.blockAt (pats, 8.0), 1);
            expectEquals (song.blockAt (pats, 12.0), -1);

            auto segs = makeSegments (song, pats);
            expectEquals ((int) segs.size(), 2);

            std::vector<double> onT, offT;
            Sequencer::generateSegments (segs, 0.0, 0.0, 12.0, [&] (const TimedEvent& e)
            {
                if (e.type == SeqEvent::Type::NoteOn) onT.push_back (e.ppq);
                else offT.push_back (e.ppq);
            });
            std::sort (onT.begin(), onT.end());
            expectEquals ((int) onT.size(), 3);                     // 0 e 4 (pattern 0 due giri), poi 8 + 2 = 10 (pattern 1 riparte da zero)
            expectWithinAbsoluteError (onT[0], 0.0, 1e-9);
            expectWithinAbsoluteError (onT[1], 4.0, 1e-9);
            expectWithinAbsoluteError (onT[2], 10.0, 1e-9);
            expect (std::find_if (offT.begin(), offT.end(), [] (double t) { return std::abs (t - 8.0) < 1e-9; }) != offT.end());   // NoteOff a fine blocco

            // Loop: con period = 12 la timeline si ripete
            int n = 0;
            Sequencer::generateSegments (segs, 12.0, 12.0, 24.0, [&] (const TimedEvent& e) { n += e.type == SeqEvent::Type::NoteOn ? 1 : 0; });
            expectEquals (n, 3);

            // Un intervallo a cavallo tra due blocchi non perde ne' duplica eventi
            int split = 0;
            Sequencer::generateSegments (segs, 0.0, 0.0, 7.9, [&] (const TimedEvent& e) { split += e.type == SeqEvent::Type::NoteOn ? 1 : 0; });
            Sequencer::generateSegments (segs, 0.0, 7.9, 12.0, [&] (const TimedEvent& e) { split += e.type == SeqEvent::Type::NoteOn ? 1 : 0; });
            expectEquals (split, 3);
        }

        beginTest ("Song: saved with the kit and as a .slsong text");
        {
            Kit kit = Kit::makeDefault();
            kit.song.name = "My song";
            kit.song.loop = true;
            kit.song.blocks = { { 3, 2 }, { 0, 7 }, { 15, 1 } };
            Kit loaded; juce::String err;
            expect (KitIO::fromJson (KitIO::toJson (kit, true), juce::File(), loaded, err), err);
            expect (loaded.song == kit.song);

            Song s2; juce::String err2;
            expect (KitIO::songFromJson (KitIO::songToJson (kit.song), s2, err2), err2);
            expect (s2 == kit.song);
            expect (! KitIO::songFromJson ("{\"type\":\"other\"}", s2, err2));
        }

        beginTest ("Sequencer: every note-on has a note-off after the gate length");
        {
            auto p = makePattern (4, 4, { 0 });
            p.lanes[0].steps[0].gate = 0.5f;
            auto ev = collect (p, 0.0, 4.0);
            expectEquals ((int) ev.size(), 2);
            expect (ev[0].type == SeqEvent::Type::NoteOn);
            expect (ev[1].type == SeqEvent::Type::NoteOff);
            expectWithinAbsoluteError (ev[1].ppq, 0.5, 1e-9);
        }

        beginTest ("Sequencer: each lane has its own length and resolution (polyrhythm)");
        {
            Pattern p;
            for (int i = 0; i < 2; ++i)
            {
                Lane l; l.cell = i; l.division = 10;                // 1/16
                l.length = i == 0 ? 16 : 12;
                l.steps[0].on = true;
                p.lanes.push_back (l);
            }
            auto ev = collect (p, 0.0, 12.0);                       // 12 beat = 48 sedicesimi
            auto a = ons (ev, 0), b = ons (ev, 1);
            expectEquals ((int) a.size(), 3);                       // ogni 16 sedicesimi (4 beat): 0, 4, 8
            expectEquals ((int) b.size(), 4);                       // ogni 12 sedicesimi (3 beat): 0, 3, 6, 9
            expectWithinAbsoluteError (b[1], 3.0, 1e-9);

            p.lanes[1].division = 12;                               // 1/16T: ogni step = 1/6 di beat
            auto tri = ons (collect (p, 0.0, 4.0), 1);
            expectWithinAbsoluteError (tri[1] - tri[0], 12.0 / 6.0, 1e-9);   // 12 step da 1/6 beat = 2 beat
        }

        beginTest ("Sequencer: swing, nudge and ratchet shape the timing");
        {
            auto p = makePattern (7, 8, { 0, 1 });                  // 1/8 = 0.5 beat
            p.swing = 0.5f;
            auto swung = ons (collect (p, 0.0, 1.0));
            expectWithinAbsoluteError (swung[0], 0.0, 1e-9);
            expectWithinAbsoluteError (swung[1], 0.5 + 0.25, 1e-9);   // lo step dispari e' ritardato di mezzo step

            auto q = makePattern (4, 4, { 0, 1 });
            q.lanes[0].steps[0].nudge = 0.25f;
            q.lanes[0].steps[1].nudge = -0.25f;
            auto nudged = ons (collect (q, 0.0, 2.0));
            expectWithinAbsoluteError (nudged[0], 0.25, 1e-9);
            expectWithinAbsoluteError (nudged[1], 0.75, 1e-9);

            auto r = makePattern (4, 4, { 0 });
            r.lanes[0].steps[0].ratchet = 3;
            auto rat = ons (collect (r, 0.0, 1.0));
            expectEquals ((int) rat.size(), 3);
            expectWithinAbsoluteError (rat[1], 1.0 / 3.0, 1e-9);
            expectWithinAbsoluteError (rat[2], 2.0 / 3.0, 1e-9);
        }

        beginTest ("Sequencer: probability is repeatable (same seed = same result) and differs with the seed");
        {
            auto p = makePattern (10, 16, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
            for (auto& s : p.lanes[0].steps) s.probability = 50;
            auto a = ons (collect (p, 0.0, 16.0)), b = ons (collect (p, 0.0, 16.0));
            expect (a == b, "same seed must give the same notes");
            expect (a.size() > 15 && a.size() < 49, "about half of 64 hits: " + juce::String ((int) a.size()));

            p.seed = 99;
            expect (ons (collect (p, 0.0, 16.0)) != a, "another seed must give another pattern");

            for (auto& s : p.lanes[0].steps) s.probability = 0;
            expect (ons (collect (p, 0.0, 16.0)).empty());
            for (auto& s : p.lanes[0].steps) s.probability = 100;
            expectEquals ((int) ons (collect (p, 0.0, 16.0)).size(), 64);
        }

        beginTest ("Sequencer: result does not depend on how the timeline is split into blocks");
        {
            auto p = makePattern (10, 16, { 0, 3, 6, 9, 12 });
            p.swing = 0.25f;
            p.lanes[0].steps[3].nudge = -0.3f;
            p.lanes[0].steps[6].ratchet = 2;
            p.lanes[0].steps[9].probability = 60;
            auto whole = collect (p, 0.0, 16.0);

            std::vector<Ev> pieces;
            for (double a = 0.0; a < 16.0; a += 0.37)
            {
                auto part = collect (p, a, juce::jmin (16.0, a + 0.37));
                pieces.insert (pieces.end(), part.begin(), part.end());
            }
            std::sort (pieces.begin(), pieces.end(), [] (const Ev& x, const Ev& y)
                       { return x.ppq != y.ppq ? x.ppq < y.ppq : (int) x.type < (int) y.type; });
            expectEquals ((int) pieces.size(), (int) whole.size());
            for (size_t i = 0; i < whole.size() && i < pieces.size(); ++i)
                expectWithinAbsoluteError (pieces[i].ppq, whole[i].ppq, 1e-9);
        }

        beginTest ("Sequencer: mute and solo");
        {
            Pattern p;
            for (int i = 0; i < 3; ++i)
            {
                Lane l; l.cell = i; l.division = 4; l.length = 4; l.steps[0].on = true;
                p.lanes.push_back (l);
            }
            expectEquals ((int) ons (collect (p, 0.0, 1.0)).size(), 3);
            p.lanes[1].mute = true;
            expectEquals ((int) ons (collect (p, 0.0, 1.0)).size(), 2);
            p.lanes[2].solo = true;                                 // con un solo attivo suona solo quella lane
            auto solo = collect (p, 0.0, 1.0);
            expectEquals ((int) ons (solo).size(), 1);
            expectEquals (solo[0].lane, 2);
        }

        beginTest ("Sequencer runner: converts time to samples and stops cleanly");
        {
            SequencerRunner run;
            auto p = makePattern (4, 4, { 0, 1 });                  // colpi a 0 e 1 beat
            run.runBlock (&p, true, 0.0, 120.0, 44100.0, 44100);    // 1 secondo = 2 beat a 120 BPM
            auto& ev = run.getEvents();
            std::vector<int> onPos;
            for (auto& e : ev) if (e.type == SeqEvent::Type::NoteOn) onPos.push_back (e.samplePos);
            expectEquals ((int) onPos.size(), 2);
            expectEquals (onPos[0], 0);
            expectEquals (onPos[1], 22050);

            run.runBlock (&p, false, 2.0, 120.0, 44100.0, 512);     // stop: un evento Stop, poi silenzio
            expectEquals ((int) run.getEvents().size(), 1);
            expect (run.getEvents()[0].type == SeqEvent::Type::Stop);
            run.runBlock (&p, false, 2.0, 120.0, 44100.0, 512);
            expect (run.getEvents().empty());

            // Posizioni in ordine, note-off prima dei note-on alla stessa posizione
            auto q = makePattern (4, 4, { 0, 1 });
            q.lanes[0].steps[0].gate = 1.0f;                        // il note-off cade esattamente sul colpo successivo
            run.runBlock (&q, true, 0.0, 120.0, 44100.0, 44100);
            for (size_t i = 1; i < run.getEvents().size(); ++i)
                expect (run.getEvents()[i - 1].samplePos <= run.getEvents()[i].samplePos);
            const auto& e = run.getEvents();
            expect (e[1].type == SeqEvent::Type::NoteOff && e[2].type == SeqEvent::Type::NoteOn && e[1].samplePos == e[2].samplePos);
        }

        beginTest ("Sequencer + engine: step triggers the cell, gate releases, one-shot keeps going, pitch transposes");
        {
            auto makeEv = [] (SeqEvent::Type t, int pos, float vel = 1.0f, float semis = 0.0f)
            {
                SeqEvent e; e.type = t; e.samplePos = pos; e.cell = 0; e.velocity = vel; e.semis = semis; return e;
            };

            Rig oneShot, gate;
            oneShot.setCells ({ { cell (36, TriggerMode::OneShot), makeSine (440, 2.0) } });
            gate.setCells ({ { cell (36, TriggerMode::Gate), makeSine (440, 2.0) } });
            std::vector<SeqEvent> evs { makeEv (SeqEvent::Type::NoteOn, 100), makeEv (SeqEvent::Type::NoteOff, 3000) };
            auto a = oneShot.render (8192, {}, 256, evs);
            auto b = gate.render (8192, {}, 256, evs);
            expect (peak (a, 200, 2000) > 0.3f, "step must trigger the cell");
            expect (peak (a, 6000, 8000) > 0.3f, "one-shot ignores the step note-off");
            expect (peak (b, 5000, 8000) < 0.001f, "gate cell is released by the step note-off");
            expectEquals (peak (a, 0, 100), 0.0f);

            // Trasposizione per step: +12 semitoni = doppia frequenza
            Rig base, up;
            base.setCells ({ { cell (36), makeSine (200, 1.0) } });
            up.setCells ({ { cell (36), makeSine (200, 1.0) } });
            auto ba = base.render (22050, {}, 256, { makeEv (SeqEvent::Type::NoteOn, 0) });
            auto ua = up.render (22050, {}, 256, { makeEv (SeqEvent::Type::NoteOn, 0, 1.0f, 12.0f) });
            const double ratio = (double) zeroCrossings (ua, 100, 10000) / (double) zeroCrossings (ba, 100, 10000);
            expect (std::abs (ratio - 2.0) < 0.05, "pitch ratio " + juce::String (ratio));

            // Velocita' dello step
            Rig vel;
            vel.setCells ({ { cell (36), makeDc (1.0) } });
            auto va = vel.render (2048, {}, 256, { makeEv (SeqEvent::Type::NoteOn, 0, 0.5f) });
            expectWithinAbsoluteError (va.getSample (0, 1000), 0.5f, 0.02f);

            // Un note-off MIDI non spegne una voce avviata dal sequencer
            Rig midiOff;
            midiOff.setCells ({ { cell (36, TriggerMode::Gate), makeSine (440, 2.0) } });
            juce::MidiBuffer off;
            off.addEvent (juce::MidiMessage::noteOff (1, 36), 500);
            auto m = midiOff.render (4096, off, 256, { makeEv (SeqEvent::Type::NoteOn, 0) });
            expect (peak (m, 3000, 4000) > 0.3f);
        }

        beginTest ("Sequencer + engine: parameter lock overrides the cell's filter only for that step");
        {
            auto p = cell (36, TriggerMode::Loop);
            p.fx.filterOn = true; p.fx.filterType = 0; p.fx.cutoff = 20000.0f;     // base: passa tutto
            Rig r;
            r.setCells ({ { p, makeSine (8000, 4.0) } });

            SeqEvent locked;                                    // step con cutoff bloccato a 200 Hz
            locked.type = SeqEvent::Type::NoteOn; locked.samplePos = 0; locked.cell = 0;
            locked.lockMask = 1u << (int) FxF::Cutoff;
            locked.lockValues[(size_t) FxF::Cutoff] = 200.0f;

            SeqEvent plain = locked;                            // step successivo senza lock
            plain.samplePos = 20000;
            plain.lockMask = 0;

            auto out = r.render (40000, {}, 256, { locked, plain });
            expect (peak (out, 8000, 19000) < 0.1f, "locked step: 8 kHz must be filtered");
            expect (peak (out, 30000, 40000) > 0.5f, "next step without lock goes back to the cell's own cutoff");
        }

        beginTest ("Sequencer + engine: a step can reverse the cell and override its trigger mode");
        {
            auto ramp = [] ()
            {
                auto d = std::make_shared<SampleData>();
                d->audio.setSize (1, 4000);
                for (int i = 0; i < 4000; ++i) d->audio.setSample (0, i, (float) i / 4000.0f);   // rampa crescente
                d->sampleRate = kSR;
                return SampleDataPtr (d);
            };
            auto ev = [] (SeqEvent::Type t, int pos, bool rev = false, int mode = -1)
            {
                SeqEvent e; e.type = t; e.samplePos = pos; e.cell = 0; e.reverse = rev; e.mode = mode; return e;
            };

            Rig forward, flipped, cellReverse, cellReverse2;
            forward.setCells ({ { cell (36), ramp() } });
            flipped.setCells ({ { cell (36), ramp() } });
            auto rc = cell (36); rc.reverse = true;
            cellReverse.setCells ({ { rc, ramp() } });
            cellReverse2.setCells ({ { rc, ramp() } });

            auto f = forward.render (1024, {}, 256, { ev (SeqEvent::Type::NoteOn, 0) });
            auto r = flipped.render (1024, {}, 256, { ev (SeqEvent::Type::NoteOn, 0, true) });
            auto c = cellReverse.render (1024, {}, 256, { ev (SeqEvent::Type::NoteOn, 0) });
            auto both = cellReverse2.render (1024, {}, 256, { ev (SeqEvent::Type::NoteOn, 0, true) });
            expect (f.getSample (0, 500) > f.getSample (0, 50), "forward ramp rises");
            expect (r.getSample (0, 500) < r.getSample (0, 50), "step reverse plays the ramp backwards");
            expect (c.getSample (0, 500) < c.getSample (0, 50), "cell reverse plays backwards");
            expect (both.getSample (0, 500) > both.getSample (0, 50), "cell reverse + step reverse cancel out");

            // La modalita' dello step vince su quella della cella: cella one-shot, step gate -> il note-off spegne
            Rig oneShotCell;
            oneShotCell.setCells ({ { cell (36, TriggerMode::OneShot), makeSine (440, 2.0) } });
            auto a = oneShotCell.render (8192, {}, 256, { ev (SeqEvent::Type::NoteOn, 0, false, 1), ev (SeqEvent::Type::NoteOff, 2000) });
            expect (peak (a, 1000, 1900) > 0.3f);
            expect (peak (a, 4000, 8000) < 0.001f, "step in gate mode is released by the note-off");

            // E viceversa: cella gate, step one-shot -> il note-off e' ignorato
            Rig gateCell;
            gateCell.setCells ({ { cell (36, TriggerMode::Gate), makeSine (440, 2.0) } });
            auto b = gateCell.render (8192, {}, 256, { ev (SeqEvent::Type::NoteOn, 0, false, 0), ev (SeqEvent::Type::NoteOff, 2000) });
            expect (peak (b, 5000, 8000) > 0.3f, "step in one-shot mode ignores the note-off");
        }

        beginTest ("Sequencer + engine: locking an effect parameter switches that effect on for the step");
        {
            auto impulse3 = [] ()
            {
                auto d = std::make_shared<SampleData>();
                d->audio.setSize (1, 100); d->audio.clear(); d->audio.setSample (0, 0, 1.0f); d->sampleRate = kSR;
                return SampleDataPtr (d);
            };
            auto run = [&] (std::function<void (Step&)> setup)
            {
                Pattern pat;
                Lane l; l.cell = 0; l.division = 4; l.length = 4; l.steps[0].on = true;
                setup (l.steps[0]);
                pat.lanes.push_back (l);
                SequencerRunner r;
                r.runBlock (&pat, true, 0.0, 120.0, kSR, 44100, 1.0);
                std::vector<SeqEvent> ons;
                for (auto& e : r.getEvents()) if (e.type == SeqEvent::Type::NoteOn) ons.push_back (e);
                auto c = cell (36);                             // delay SPENTO sulla cella; tempo e feedback sono della cella
                c.fx.delayTime = 100.0f; c.fx.delayFeedback = 0.0f;
                Rig rig;
                rig.setCells ({ { c, impulse3() } });
                return rig.render (8192, {}, 256, ons);
            };

            // Il lock della mandata (delay send) accende il delay per quel colpo anche se sulla cella e' spento
            auto withLocks = run ([] (Step& s) { s.setLock (FxF::DelayMix, 1.0f); });
            expect (peak (withLocks, 4300, 4520) > 0.7f, "a send lock must switch the delay on for that hit");

            // Senza lock niente eco
            auto none = run ([] (Step&) {});
            expect (peak (none, 4300, 4520) < 0.01f);

            // Un lock esplicito On/Off = spento vince sul valore bloccato
            auto forcedOff = run ([] (Step& s)
            {
                s.setLock (FxF::DelayMix, 1.0f);
                s.setLock (lockIndex (FxB::Delay), 0.0f);
            });
            expect (peak (forcedOff, 4300, 4520) < 0.01f, "explicit Off lock wins over the implicit switch-on");
        }

        beginTest ("Per-hit effects: a lock changes only its own note, never the notes already playing");
        {
            // Due colpi della stessa cella, il secondo con cutoff bloccato a 200 Hz: il primo (sinusoide a 8 kHz in loop) deve
            // continuare identico, il secondo suonare filtrato. Prima il lock cambiava la catena condivisa e toccava anche il primo.
            auto p = cell (36, TriggerMode::Loop);
            p.fx.filterOn = true; p.fx.filterType = 0; p.fx.cutoff = 20000.0f;
            Rig r;
            r.setCells ({ { p, makeSine (8000, 4.0) } });

            SeqEvent first;  first.type = SeqEvent::Type::NoteOn; first.samplePos = 0; first.cell = 0;
            SeqEvent second = first; second.samplePos = 20000;
            second.lockMask = 1u << (int) FxF::Cutoff;
            second.lockValues[(size_t) FxF::Cutoff] = 200.0f;

            auto base = r.render (40000, {}, 256, { first });
            Rig r2;
            r2.setCells ({ { p, makeSine (8000, 4.0) } });
            auto withSecond = r2.render (40000, {}, 256, { first, second });

            // Fino a 20000 identici
            float maxDiff = 0.0f;
            for (int i = 0; i < 19900; ++i)
                maxDiff = juce::jmax (maxDiff, std::abs (base.getSample (0, i) - withSecond.getSample (0, i)));
            expectEquals (maxDiff, 0.0f);

            // Dopo il secondo colpo il primo voce suona sempre uguale: il segnale totale = primo + secondo(filtrato ~ 0)
            float after = 0.0f;
            for (int i = 25000; i < 40000; ++i)
                after = juce::jmax (after, std::abs (base.getSample (0, i) - withSecond.getSample (0, i)));
            expect (after < 0.1f, "the first voice must be untouched by the second hit's lock: diff " + juce::String (after));
        }

        beginTest ("Per-hit effects: a delay send echoes after the step ends and does not cut at the next hit");
        {
            auto p = cell (36);
            p.fx.delayTime = 100.0f; p.fx.delayFeedback = 0.0f;          // delay spento sulla cella
            auto impulse4 = [] ()
            {
                auto d = std::make_shared<SampleData>();
                d->audio.setSize (1, 100); d->audio.clear(); d->audio.setSample (0, 0, 1.0f); d->sampleRate = kSR;
                return SampleDataPtr (d);
            };
            Rig r;
            r.setCells ({ { p, impulse4() } });

            SeqEvent sent;  sent.type = SeqEvent::Type::NoteOn; sent.samplePos = 0; sent.cell = 0;
            sent.lockMask = (1u << (int) FxF::DelayMix) | (1u << lockIndex (FxB::Delay));
            sent.lockValues[(size_t) FxF::DelayMix] = 1.0f;
            sent.lockValues[(size_t) lockIndex (FxB::Delay)] = 1.0f;
            SeqEvent next;  next.type = SeqEvent::Type::NoteOn; next.samplePos = 2000; next.cell = 0;   // colpo successivo senza lock

            auto out = r.render (8192, {}, 256, { sent, next });
            expect (peak (out, 4350, 4470) > 0.7f, "the echo of the first hit still sounds at 100 ms, after the next hit started");
            expect (peak (out, 2000, 2050) > 0.7f, "the next hit itself is there");
            // Il secondo colpo non ha mandata: la sua eco (a 2000 + 4410 = 6410) non esiste
            expect (peak (out, 6350, 6480) < 0.01f, "the second hit has no send, so no echo of it");
        }

        beginTest ("Per-hit effects: filter state is clean at every new note (no leftover ringing)");
        {
            auto p = cell (36);
            p.fx.filterOn = true; p.fx.filterType = 0; p.fx.cutoff = 500.0f; p.fx.resonance = 8.0f;
            Rig a, b;
            auto sine = makeSine (440, 1.0);
            a.setCells ({ { p, sine } });
            b.setCells ({ { p, sine } });
            auto o1 = a.render (4096, noteOn (36));
            // Stesso colpo dopo una nota precedente completamente finita: risultato identico
            b.render (44100, noteOn (36));
            auto o2 = b.render (4096, noteOn (36));
            float d = 0.0f;
            for (int i = 0; i < 4096; ++i) d = juce::jmax (d, std::abs (o1.getSample (0, i) - o2.getSample (0, i)));
            expect (d < 1.0e-5f, "second note must start from a clean filter state, diff " + juce::String (d));
        }

        beginTest ("Normalize: gain brings the peak to the target; silence is left alone");
        {
            auto quiet = makeSine (440, 0.5, 0.25f);                          // picco 0.25 = -12.04 dB
            const float gain = normalizeGainDb (samplePeak (*quiet), -1.0f);
            expectWithinAbsoluteError (gain, 11.04f, 0.05f);
            expectWithinAbsoluteError (samplePeak (*quiet) * dbToGain (gain), dbToGain (-1.0f), 0.01f);

            auto loud = makeSine (440, 0.5, 1.0f);
            expectWithinAbsoluteError (normalizeGainDb (samplePeak (*loud), -1.0f), -1.0f, 0.05f);   // si abbassa a -1 dB

            auto silent = std::make_shared<SampleData>();
            silent->audio.setSize (1, 100); silent->audio.clear();
            expectEquals (normalizeGainDb (samplePeak (*silent), -1.0f), 0.0f);

            // La regione conta: il picco e' misurato solo tra start ed end
            auto two = std::make_shared<SampleData>();
            two->audio.setSize (1, 1000); two->audio.clear();
            two->audio.setSample (0, 100, 0.2f);
            two->audio.setSample (0, 900, 1.0f);
            expectWithinAbsoluteError (samplePeak (*two, 0.0f, 0.5f), 0.2f, 1e-6f);
            expectWithinAbsoluteError (samplePeak (*two, 0.5f, 1.0f), 1.0f, 1e-6f);
        }

        beginTest ("Sequencer + engine: a dry lane skips the cell's effects; a step lock can switch one back on");
        {
            auto p = cell (36);
            p.fx.delayOn = true; p.fx.delayTime = 100.0f; p.fx.delayFeedback = 0.0f; p.fx.delayMix = 1.0f;
            auto impulse2 = [] ()
            {
                auto d = std::make_shared<SampleData>();
                d->audio.setSize (1, 100); d->audio.clear(); d->audio.setSample (0, 0, 1.0f); d->sampleRate = kSR;
                return SampleDataPtr (d);
            };

            // Costruzione degli eventi attraverso il vero generatore, come in riproduzione
            auto makeEvents = [] (bool dry, bool lockDelayOn)
            {
                Pattern pat;
                Lane l; l.cell = 0; l.division = 4; l.length = 4; l.dry = dry;
                l.steps[0].on = true;
                if (lockDelayOn)
                    l.steps[0].setLock (lockIndex (FxB::Delay), 1.0f);
                pat.lanes.push_back (l);
                SequencerRunner run;
                run.runBlock (&pat, true, 0.0, 120.0, kSR, 44100, 1.0);
                std::vector<SeqEvent> out;
                for (auto& e : run.getEvents()) if (e.type == SeqEvent::Type::NoteOn) out.push_back (e);
                return out;
            };

            auto normal = Rig();  normal.setCells ({ { p, impulse2() } });
            auto outNormal = normal.render (8192, {}, 256, { SeqEvent { SeqEvent::Type::NoteOn, 0, 0 } });
            expect (peak (outNormal, 4300, 4520) > 0.8f, "baseline: the cell's delay echoes at 100 ms");

            Rig dryRig;  dryRig.setCells ({ { p, impulse2() } });
            auto outDry = dryRig.render (8192, {}, 256, makeEvents (true, false));
            expect (peak (outDry, 0, 50) > 0.7f, "the hit is still there");
            expect (peak (outDry, 4300, 4520) < 0.01f, "dry lane: no echo from the cell's delay");

            Rig back;  back.setCells ({ { p, impulse2() } });
            auto outBack = back.render (8192, {}, 256, makeEvents (true, true));
            expect (peak (outBack, 4300, 4520) > 0.7f, "a step lock switches the delay back on for that hit");

            // Il lock di accensione funziona anche su una cella senza effetti
            auto plain = cell (36);
            plain.fx.delayTime = 100.0f; plain.fx.delayFeedback = 0.0f; plain.fx.delayMix = 1.0f;     // delay spento
            Rig off;  off.setCells ({ { plain, impulse2() } });
            auto outOff = off.render (8192, {}, 256, makeEvents (false, true));
            expect (peak (outOff, 4300, 4520) > 0.7f, "step lock turns an effect on that the cell had off");
        }

        beginTest ("Sequencer: patterns round-trip through the kit file (only lit steps are stored)");
        {
            auto kit = Kit::makeDefault();
            auto& pat = kit.patterns[2];
            pat.name = "Funk";
            pat.swing = 0.25f;
            pat.seed = 777;
            Lane l; l.cell = 5; l.division = 11; l.length = 12; l.mute = true;
            auto& s = l.steps[3];
            s.on = true; s.velocity = 90; s.probability = 75; s.ratchet = 3; s.nudge = -0.1f; s.gate = 0.8f; s.pitch = -5;
            s.setLock (FxF::Cutoff, 1500.0f);
            s.setLock (FxF::DelayMix, 0.6f);
            s.setLock (lockIndex (FxB::Reverb), 1.0f);
            s.reverse = true;
            s.mode = 1;
            l.dry = true;
            l.steps[7].on = true;
            pat.lanes.push_back (l);
            kit.activePattern = 2;

            auto json = KitIO::toJson (kit, true);
            expect (json.contains ("\"patterns\""));

            Kit loaded; juce::String err;
            expect (KitIO::fromJson (json, juce::File(), loaded, err), err);
            expectEquals ((int) loaded.patterns.size(), kNumPatterns);
            expectEquals (loaded.activePattern, 2);
            const auto& lp = loaded.patterns[2];
            expectEquals (lp.name, juce::String ("Funk"));
            expectWithinAbsoluteError (lp.swing, 0.25f, 0.001f);
            expectEquals ((int) lp.seed, 777);
            expectEquals ((int) lp.lanes.size(), 1);
            const auto& ll = lp.lanes[0];
            expectEquals (ll.cell, 5); expectEquals (ll.division, 11); expectEquals (ll.length, 12); expect (ll.mute);
            const auto& ls = ll.steps[3];
            expect (ls.on);
            expectEquals ((int) ls.velocity, 90); expectEquals ((int) ls.probability, 75); expectEquals ((int) ls.ratchet, 3);
            expectWithinAbsoluteError (ls.nudge, -0.1f, 0.001f); expectWithinAbsoluteError (ls.gate, 0.8f, 0.001f);
            expectEquals ((int) ls.pitch, -5);
            expect (ls.hasLock (FxF::Cutoff) && ls.hasLock (FxF::DelayMix) && ! ls.hasLock (FxF::Drive));
            expect (ls.hasLock (lockIndex (FxB::Reverb)), "effect on/off lock is saved");
            expect (ls.reverse && ls.mode == 1 && ll.dry);
            expect (! ll.steps[7].reverse && ll.steps[7].mode == -1);
            expectWithinAbsoluteError (ls.lockValues[(size_t) FxF::Cutoff], 1500.0f, 0.01f);
            expect (ll.steps[7].on && ! ll.steps[8].on);
            expect (loaded.patterns[3].lanes.empty());
        }

        beginTest ("Export: pattern becomes a standard MIDI clip with the right notes, velocities and tempo");
        {
            auto kit = Kit::makeDefault();
            kit.cells[0].lowNote = kit.cells[0].highNote = 36;
            kit.cells[1].lowNote = kit.cells[1].highNote = 38;

            Pattern p;
            p.name = "Beat";
            Lane kick; kick.cell = 0; kick.division = 4; kick.length = 4;           // 1/4: un colpo a beat sugli step accesi
            kick.steps[0].on = true; kick.steps[0].velocity = 110;
            kick.steps[2].on = true;
            Lane snare; snare.cell = 1; snare.division = 4; snare.length = 4;
            snare.steps[1].on = true; snare.steps[1].velocity = 80; snare.steps[1].ratchet = 2;
            p.lanes = { kick, snare };

            auto midi = SequencerExport::toMidiFile (p, kit, 2, 100.0);              // 2 battute = 8 beat
            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("samplelab_exp_" + juce::String (juce::Random::getSystemRandom().nextInt (1000000)) + ".mid");
            expect (SequencerExport::writeMidiFile (midi, tmp));

            juce::MidiFile back;
            juce::FileInputStream in (tmp);
            expect (in.openedOk() && back.readFrom (in));
            expectEquals ((int) back.getTimeFormat(), 960);
            expectEquals (back.getNumTracks(), 2);

            auto* notes = back.getTrack (1);
            int kicks = 0, snares = 0;
            double firstKickTick = -1, secondSnareHit = -1;
            for (int i = 0; i < notes->getNumEvents(); ++i)
            {
                auto& m = notes->getEventPointer (i)->message;
                if (! m.isNoteOn()) continue;
                if (m.getNoteNumber() == 36) { ++kicks; if (firstKickTick < 0) { firstKickTick = m.getTimeStamp(); expectEquals ((int) m.getVelocity(), 110); } }
                if (m.getNoteNumber() == 38) { if (++snares == 2) secondSnareHit = m.getTimeStamp(); }
            }
            expectEquals (kicks, 4);                                 // step 0 e 2, ripetuti nelle 2 battute (8 beat)
            expectEquals (snares, 4);                                // step 1 con ratchet 2, due volte
            expectWithinAbsoluteError (firstKickTick, 0.0, 0.5);
            expectWithinAbsoluteError (secondSnareHit, 1.5 * 960.0, 1.0);   // 2o colpo del ratchet: beat 1 + mezzo beat

            int noteOns = 0, noteOffs = 0;
            for (int i = 0; i < notes->getNumEvents(); ++i)
            {
                auto& m = notes->getEventPointer (i)->message;
                if (m.isNoteOn()) ++noteOns;
                if (m.isNoteOff()) ++noteOffs;
            }
            expectEquals (noteOns, noteOffs);                        // nessuna nota rimasta aperta

            // Il tempo e' nel file
            bool foundTempo = false;
            auto* conductor = back.getTrack (0);
            for (int i = 0; i < conductor->getNumEvents(); ++i)
                if (conductor->getEventPointer (i)->message.isTempoMetaEvent())
                {
                    foundTempo = true;
                    expectWithinAbsoluteError (60.0 / conductor->getEventPointer (i)->message.getTempoSecondsPerQuarterNote(), 100.0, 0.01);
                }
            expect (foundTempo);
            tmp.deleteFile();
        }

        beginTest ("Export: offline render plays the pattern, keeps the effect tail, and is repeatable");
        {
            Pattern p;
            Lane l; l.cell = 0; l.division = 4; l.length = 4; l.steps[0].on = true;   // un colpo all'inizio della battuta
            l.steps[0].gate = 0.2f;
            p.lanes.push_back (l);

            auto make = [&] (bool delay)
            {
                auto c = cell (36, TriggerMode::OneShot);
                if (delay)
                {
                    c.fx.delayOn = true; c.fx.delaySync = true; c.fx.delayDivision = 4;   // 1/4
                    c.fx.delayFeedback = 0.0f; c.fx.delayMix = 1.0f;
                }
                auto r = std::make_unique<Rig>();
                r->setCells ({ { c, impulse() } });
                r->engine.live.bpm = 120.0;
                return r;
            };

            auto dry = make (false);
            auto a = SequencerExport::renderOffline (dry->engine, p, 1, 120.0, kSR, 2.0);
            // La battuta dura 2 s; con tempo 120 BPM il colpo e' al campione 0
            // la velocity predefinita dello step e' 100/127 = 0.787
            expect (peak (a, 0, 50) > 0.7f, "hit at the start");
            expect (peak (a, 1000, a.getNumSamples()) < 0.001f, "no tail without effects");

            auto wet = make (true);
            auto b = SequencerExport::renderOffline (wet->engine, p, 1, 120.0, kSR, 2.0);
            expect (peak (b, 22000, 22100) > 0.7f, "delayed echo one beat (500 ms) later, still rendered");

            auto wet2 = make (true);
            auto b2 = SequencerExport::renderOffline (wet2->engine, p, 1, 120.0, kSR, 2.0);
            expectEquals (b.getNumSamples(), b2.getNumSamples());
            float maxDiff = 0.0f;
            for (int i = 0; i < b.getNumSamples(); ++i)
                maxDiff = juce::jmax (maxDiff, std::abs (b.getSample (0, i) - b2.getSample (0, i)));
            expectEquals (maxDiff, 0.0f);                            // render identico a ogni esecuzione
        }

        beginTest ("Export: each lane as its own file (equal length) adds up to the full mix");
        {
            auto impulseS = [] (float amp)
            {
                auto d = std::make_shared<SampleData>();
                d->audio.setSize (1, 100); d->audio.clear(); d->audio.setSample (0, 0, amp); d->sampleRate = kSR;
                return SampleDataPtr (d);
            };

            Pattern p;
            for (int i = 0; i < 2; ++i)
            {
                Lane l; l.cell = i; l.division = 4; l.length = 4;
                l.steps[i].on = true;                              // lane 0 sul beat 1, lane 1 sul beat 2
                p.lanes.push_back (l);
            }

            auto makeRig = [&] ()
            {
                auto c0 = cell (36);
                c0.fx.delayOn = true; c0.fx.delaySync = true; c0.fx.delayDivision = 4;      // 1/4: l'effetto resta nel suo stem
                c0.fx.delayFeedback = 0.0f; c0.fx.delayMix = 1.0f;
                auto c1 = cell (37);
                c1.fx.satOn = true; c1.fx.drive = 12.0f;
                auto r = std::make_unique<Rig>();
                r->setCells ({ { c0, impulseS (0.8f) }, { c1, impulseS (0.5f) } });
                r->engine.live.bpm = 120.0;
                return r;
            };

            auto full = makeRig();
            auto mix = SequencerExport::renderOffline (full->engine, p, 1, 120.0, kSR, 2.0);

            auto r0 = makeRig(), r1 = makeRig();
            auto s0 = SequencerExport::renderOffline (r0->engine, SequencerExport::isolateLane (p, 0), 1, 120.0, kSR, 2.0);
            auto s1 = SequencerExport::renderOffline (r1->engine, SequencerExport::isolateLane (p, 1), 1, 120.0, kSR, 2.0);

            expectEquals (s0.getNumSamples(), mix.getNumSamples());                         // stessa lunghezza: si allineano nella DAW
            expectEquals (s1.getNumSamples(), mix.getNumSamples());
            expect (peak (s0, 0, 50) > 0.5f && peak (s1, 0, 50) < 0.001f, "stem 0 has only lane 0 at beat 1");
            expect (peak (s0, 22000, 22100) > 0.5f, "stem 0 keeps its own delay echo");       // eco del colpo a 500 ms
            expect (peak (s1, 22000, 22100) > 0.1f, "stem 1 has lane 1 at beat 2 (saturated by its own effect)");

            float maxDiff = 0.0f;
            for (int i = 0; i < mix.getNumSamples(); ++i)
                maxDiff = juce::jmax (maxDiff, std::abs (mix.getSample (0, i) - (s0.getSample (0, i) + s1.getSample (0, i))));
            expect (maxDiff < 1.0e-5f, "stems must sum to the full mix, diff " + juce::String (maxDiff));
        }

        // ------------------------------------------------------- MIDI learn

        auto ccBuffer = [] (int cc, int value)
        {
            juce::MidiBuffer b;
            b.addEvent (juce::MidiMessage::controllerEvent (1, cc, value), 0);
            return b;
        };

        beginTest ("MIDI learn: the next CC is assigned to the learning parameter");
        {
            ParamRig pr;
            MidiLearn ml (pr.set);
            auto* cutoff = pr.byId ("c00.filter.cutoff");

            expectEquals (ml.ccFor (cutoff), -1);
            ml.startLearn (cutoff);
            expect (ml.isLearning (cutoff));

            ml.process (ccBuffer (74, 10));                    // il CC viene catturato, non muove il parametro
            expectWithinAbsoluteError (cutoff->getValue(), cutoff->convertTo0to1 (8000.0f), 0.0001f);

            expect (ml.poll());
            expectEquals (ml.ccFor (cutoff), 74);
            expect (! ml.isLearning (cutoff));
            expect (! ml.poll());                              // niente da fare la seconda volta
        }

        beginTest ("MIDI learn: CC drives the parameter over its whole range");
        {
            ParamRig pr;
            MidiLearn ml (pr.set);
            auto* level = pr.byId ("c01.level");               // -60..+12 dB
            ml.startLearn (level);
            ml.process (ccBuffer (7, 1));
            ml.poll();

            ml.process (ccBuffer (7, 0));
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (level)->get(), -60.0f, 0.1f);
            ml.process (ccBuffer (7, 127));
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (level)->get(), 12.0f, 0.1f);
            ml.process (ccBuffer (7, 64));
            const float mid = static_cast<juce::AudioParameterFloat*> (level)->get();
            expect (mid > -25.0f && mid < -23.0f, "mid value: " + juce::String (mid));

            // I messaggi che non sono CC (note) non muovono nulla
            juce::MidiBuffer notes;
            notes.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
            ml.process (notes);
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (level)->get(), mid, 0.001f);
        }

        beginTest ("MIDI learn: toggles switch at 64, choices follow the CC");
        {
            ParamRig pr;
            MidiLearn ml (pr.set);
            auto* power = static_cast<juce::AudioParameterBool*> (pr.byId ("c00.delay.on"));
            auto* mode = static_cast<juce::AudioParameterChoice*> (pr.byId ("c00.mode"));

            ml.startLearn (power); ml.process (ccBuffer (20, 0)); ml.poll();
            ml.startLearn (mode);  ml.process (ccBuffer (21, 0)); ml.poll();

            ml.process (ccBuffer (20, 63)); expect (! power->get());
            ml.process (ccBuffer (20, 64)); expect (power->get());
            ml.process (ccBuffer (20, 0));  expect (! power->get());

            ml.process (ccBuffer (21, 0));   expectEquals (mode->getIndex(), 0);
            ml.process (ccBuffer (21, 64));  expectEquals (mode->getIndex(), 1);
            ml.process (ccBuffer (21, 127)); expectEquals (mode->getIndex(), 2);
        }

        beginTest ("MIDI learn: one parameter has one CC, one CC can drive several parameters");
        {
            ParamRig pr;
            MidiLearn ml (pr.set);
            auto* a = pr.byId ("c00.level");
            auto* b = pr.byId ("c01.level");

            ml.startLearn (a); ml.process (ccBuffer (11, 0)); ml.poll();
            ml.startLearn (b); ml.process (ccBuffer (11, 0)); ml.poll();       // stesso CC su due parametri
            ml.process (ccBuffer (11, 127));
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (a)->get(), 12.0f, 0.1f);
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (b)->get(), 12.0f, 0.1f);

            ml.startLearn (a); ml.process (ccBuffer (12, 0)); ml.poll();       // riassegnazione di a: ora CC 12
            expectEquals (ml.ccFor (a), 12);
            ml.process (ccBuffer (11, 0));
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (a)->get(), 12.0f, 0.1f);   // a non segue piu' il CC 11
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (b)->get(), -60.0f, 0.1f);

            ml.clear (b);
            expectEquals (ml.ccFor (b), -1);
            ml.process (ccBuffer (11, 127));
            expectWithinAbsoluteError (static_cast<juce::AudioParameterFloat*> (b)->get(), -60.0f, 0.1f);

            ml.clearAll();
            expectEquals (ml.ccFor (a), -1);
        }

        beginTest ("MIDI learn: learning can be cancelled; mappings round-trip through the kit file");
        {
            ParamRig pr;
            MidiLearn ml (pr.set);
            auto* cutoff = pr.byId ("c00.filter.cutoff");
            ml.startLearn (cutoff);
            ml.cancelLearn();
            ml.process (ccBuffer (30, 100));
            expect (! ml.poll());
            expectEquals (ml.ccFor (cutoff), -1);

            ml.startLearn (cutoff); ml.process (ccBuffer (74, 0)); ml.poll();
            ml.startLearn (pr.byId ("master.level")); ml.process (ccBuffer (7, 0)); ml.poll();

            auto kit = Kit::makeDefault();
            kit.midiMap = ml.mappings();
            expectEquals ((int) kit.midiMap.size(), 2);

            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("samplelab_ml_" + juce::String (juce::Random::getSystemRandom().nextInt (1000000)));
            tmp.createDirectory();
            juce::String err;
            auto f = tmp.getChildFile ("Learn.slkit");
            expect (KitIO::save (kit, f, err), err);
            Kit loaded;
            expect (KitIO::load (f, loaded, err), err);
            expectEquals ((int) loaded.midiMap.size(), 2);

            MidiLearn other (pr.set);                          // nuova sessione: le assegnazioni tornano dal kit
            other.setMappings (loaded.midiMap);
            expectEquals (other.ccFor (cutoff), 74);
            expectEquals (other.ccFor (pr.byId ("master.level")), 7);

            // ID non piu' esistenti (kit di un'altra versione) vengono ignorati senza errori
            other.setMappings ({ { 5, "c99.does.not.exist" }, { 6, "c00.pan" } });
            expectEquals (other.ccFor (pr.byId ("c00.pan")), 6);
            expectEquals ((int) other.mappings().size(), 1);
            tmp.deleteRecursively();
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

        beginTest ("Loop BPM estimate from duration (whole bars)");
        {
            // 2 battute a 120 BPM = 4 s; 1 battuta a 90 BPM = 2.667 s; troppo breve = sconosciuto
            expect (std::abs (estimateLoopBpm (*makeDc (4.0)) - 120.0) < 0.01);
            expect (std::abs (estimateLoopBpm (*makeDc (4.0 * 60.0 / 90.0)) - 90.0) < 0.5);
            expect (std::abs (estimateLoopBpm (*makeDc (8.0)) - 120.0) < 0.01);   // 4 battute a 120 (anche 60 BPM x2, ma 120 e' piu' vicino)
            expectEquals (estimateLoopBpm (*makeDc (0.2)), 0.0);
            // Regione Start..End: meta' di 8 s = 4 s = 120 BPM
            expect (std::abs (estimateLoopBpm (*makeDc (8.0), 0.0f, 0.5f) - 120.0) < 0.01);
        }

        beginTest ("Time-stretch changes length by the ratio and keeps the pitch (Key Lock)");
        {
            auto src = makeSine (440.0, 2.0);
            auto out = stretchSample (*src, 0.5, 1.0);            // il doppio piu' veloce
            expect (out != nullptr);
            if (out != nullptr)
            {
                expect (std::abs (out->numSamples() - src->numSamples() / 2) < 2000, "len " + juce::String (out->numSamples()));
                // Frequenza dalla parte centrale (evita le code): conta i passaggi per lo zero in mezzo secondo
                const int from = out->numSamples() / 4, to = from + (int) (kSR * 0.5);
                int crossings = 0;
                for (int i = from + 1; i < to; ++i)
                    if ((out->audio.getSample (0, i - 1) < 0.0f) != (out->audio.getSample (0, i) < 0.0f))
                        ++crossings;
                const double freq = crossings / 2.0 / 0.5;
                expect (std::abs (freq - 440.0) < 8.0, "freq " + juce::String (freq));
            }
        }

        beginTest ("Stretch without Key Lock (varispeed) changes the pitch like a resample");
        {
            auto src = makeSine (440.0, 2.0);
            auto out = stretchSample (*src, 0.5, 2.0);            // metà durata e un'ottava sopra
            expect (out != nullptr);
            if (out != nullptr)
            {
                const int from = out->numSamples() / 4, to = from + (int) (kSR * 0.25);
                int crossings = 0;
                for (int i = from + 1; i < to; ++i)
                    if ((out->audio.getSample (0, i - 1) < 0.0f) != (out->audio.getSample (0, i) < 0.0f))
                        ++crossings;
                const double freq = crossings / 2.0 / 0.25;
                expect (std::abs (freq - 880.0) < 16.0, "freq " + juce::String (freq));
            }
        }

        beginTest ("Loop sync fields survive a kit save and load");
        {
            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("sl_loop_test");
            tmp.deleteRecursively();
            tmp.createDirectory();
            Kit k = Kit::makeDefault();
            k.cells[0].tempoSync = true;
            k.cells[0].keyLock = false;
            k.cells[0].loopBpm = 93.5f;
            juce::String err;
            const auto f = tmp.getChildFile ("Loop.slkit");
            expect (KitIO::save (k, f, err), err);
            Kit back;
            expect (KitIO::load (f, back, err), err);
            expect (back.cells[0].tempoSync);
            expect (! back.cells[0].keyLock);
            expectWithinAbsoluteError (back.cells[0].loopBpm, 93.5f, 0.001f);
            expect (! back.cells[1].tempoSync && back.cells[1].keyLock);   // le altre celle: valori predefiniti
            tmp.deleteRecursively();
        }

        beginTest ("Multiple outputs: a cell routed to Out 2 plays there, not in the Main mix");
        {
            auto run = [&] (int outputMapCh, int cellOutput, float& mainPeak, float& aux2Peak, float& masterGainCheck)
            {
                Rig r;
                auto p0 = cell (36);
                p0.output = 0;
                auto p1 = cell (38);
                p1.output = cellOutput;
                r.setCells ({ { p0, makeSine (440, 1.0) }, { p1, makeSine (880, 1.0) } });
                r.engine.outputMap[1] = outputMapCh;
                r.engine.live.masterGain = 0.5f;
                juce::AudioBuffer<float> blk (4, 4096);
                blk.clear();
                juce::MidiBuffer m;
                m.addEvent (juce::MidiMessage::noteOn (1, 38, 1.0f), 0);      // suona solo la cella 2
                r.engine.process (blk, m, nullptr);
                mainPeak = juce::jmax (blk.getMagnitude (0, 0, 4096), blk.getMagnitude (1, 0, 4096));
                aux2Peak = juce::jmax (blk.getMagnitude (2, 0, 4096), blk.getMagnitude (3, 0, 4096));
                masterGainCheck = 0.0f;
            };

            float mainPeak = 0, auxPeak = 0, dummy = 0;
            run (2, 1, mainPeak, auxPeak, dummy);                // Out 2 attivo: il suono e' solo li'
            expect (auxPeak > 0.1f, "aux peak " + juce::String (auxPeak));
            expectEquals (mainPeak, 0.0f);

            run (-1, 1, mainPeak, auxPeak, dummy);               // Out 2 spento dalla DAW: ripiega sul Main
            expect (mainPeak > 0.1f, "fallback main peak " + juce::String (mainPeak));
            expectEquals (auxPeak, 0.0f);

            // Il volume master vale solo per il Main: l'uscita singola e' pre-master
            run (2, 1, mainPeak, auxPeak, dummy);
            Rig ref;
            auto p = cell (38);
            ref.setCells ({ { p, makeSine (880, 1.0) } });
            auto full = ref.render (4096, noteOn (38));
            expect (std::abs (auxPeak - peak (full, 0, 4096)) < 0.02f, "aux " + juce::String (auxPeak) + " vs " + juce::String (peak (full, 0, 4096)));
        }

        beginTest ("Cell output assignment survives a kit save and load");
        {
            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("sl_out_test");
            tmp.deleteRecursively();
            tmp.createDirectory();
            Kit k = Kit::makeDefault();
            k.cells[3].output = 7;
            juce::String err;
            const auto f = tmp.getChildFile ("Out.slkit");
            expect (KitIO::save (k, f, err), err);
            Kit back;
            expect (KitIO::load (f, back, err), err);
            expectEquals (back.cells[3].output, 7);
            expectEquals (back.cells[0].output, 0);
            tmp.deleteRecursively();
        }

        beginTest ("Audio export tail modes: Cut and Loop are exactly N bars, Keep is longer");
        {
            Pattern p;
            Lane l; l.cell = 0; l.division = 4; l.length = 4; l.steps[3].on = true;      // un colpo sull'ultimo quarto
            p.lanes.push_back (l);

            auto make = [&]
            {
                auto c = cell (36, TriggerMode::OneShot);
                c.fx.delayOn = true; c.fx.delaySync = true; c.fx.delayDivision = 1;   // 1/2 = 1 s
                c.fx.delayFeedback = 0.0f; c.fx.delayMix = 1.0f;
                auto r = std::make_unique<Rig>();
                auto imp = std::make_shared<SampleData>();
                imp->audio.setSize (1, 200);
                imp->audio.clear();
                imp->audio.setSample (0, 0, 1.0f);
                imp->sampleRate = kSR;
                r->setCells ({ { c, imp } });
                r->engine.live.bpm = 120.0;
                return r;
            };

            const int bar = (int) std::llround (4.0 * 60.0 / 120.0 * kSR);       // 1 battuta a 120 BPM = 2 s
            auto cutRig = make(), loopRig = make(), keepRig = make();
            auto cut = SequencerExport::renderBounce (cutRig->engine, p, 1, 120.0, kSR, SequencerExport::TailMode::Cut);
            auto loop = SequencerExport::renderBounce (loopRig->engine, p, 1, 120.0, kSR, SequencerExport::TailMode::Loop);
            auto keep = SequencerExport::renderBounce (keepRig->engine, p, 1, 120.0, kSR, SequencerExport::TailMode::Keep);
            expectEquals (cut.getNumSamples(), bar);
            expectEquals (loop.getNumSamples(), bar);
            expect (keep.getNumSamples() > bar + (int) kSR, "keep is longer: " + juce::String (keep.getNumSamples()));
            // Colpo sul 4o quarto (1.5 s) + eco a 1/2 (1 s dopo) = 2.5 s: oltre la fine nel Cut, a 0.5 s dall'inizio nel Loop
            expect (peak (cut, 0, (int) cut.getNumSamples()) < 0.001f || peak (cut, 22000, 22100) < 0.001f, "cut: no wrapped echo");
            expect (peak (loop, 22000, 22100) > 0.3f, "loop: the echo wraps to the start, peak " + juce::String (peak (loop, 22000, 22100)));
            expect (peak (keep, 110000, 110500) > 0.3f, "keep: the echo is after the end");
        }

        beginTest ("Pattern length in beats is the longest lane; switch inside a block is sample-accurate");
        {
            Pattern a, b;
            Lane la; la.cell = 0; la.division = 10; la.length = 16; la.steps[0].on = true;           // 16 x 1/16 = 4 quarti
            Lane lb; lb.cell = 1; lb.division = 10; lb.length = 16; lb.steps[0].on = true;
            a.lanes.push_back (la);
            b.lanes.push_back (lb);
            expectWithinAbsoluteError (a.lengthBeats(), 4.0, 1.0e-9);

            Pattern longer = a;
            Lane l2; l2.cell = 2; l2.division = 10; l2.length = 64; longer.lanes.push_back (l2);       // 64 x 1/16 = 16 quarti
            expectWithinAbsoluteError (longer.lengthBeats(), 16.0, 1.0e-9);
            expectWithinAbsoluteError (Pattern().lengthBeats(), 4.0, 1.0e-9);                         // vuoto: una battuta

            // Blocco di 1 quarto (22050 campioni a 120 BPM) da 3.5 a 4.5 con cambio a 4.0: il colpo del pattern B cade a meta' blocco
            SequencerRunner run1;
            run1.runBlockSwitch (&a, &b, 4.0, true, 3.5, 120.0, kSR, 22050);
            int onCell0 = 0, onCell1 = 0, offCell0 = 0, posOn1 = -1;
            for (const auto& e : run1.getEvents())
            {
                if (e.type == SeqEvent::Type::NoteOn && e.cell == 0) ++onCell0;
                if (e.type == SeqEvent::Type::NoteOn && e.cell == 1) { ++onCell1; posOn1 = e.samplePos; }
                if (e.type == SeqEvent::Type::NoteOff && e.cell == 0) ++offCell0;
            }
            expectEquals (onCell0, 0);                      // il vecchio pattern non suona piu' il suo colpo a 4.0
            expectEquals (onCell1, 1);
            expectEquals (posOn1, 11025);                   // (4.0 - 3.5) quarti = 11025 campioni
            expect (offCell0 >= 1, "the old pattern's notes are released at the switch");

            // Senza cambio dentro il blocco (switch oltre la fine) suona ancora il vecchio pattern
            SequencerRunner r2;
            r2.runBlockSwitch (&a, &b, 5.0, true, 3.5, 120.0, kSR, 22050);
            int c1 = 0, c0 = 0;
            for (const auto& e : r2.getEvents())
            {
                if (e.type == SeqEvent::Type::NoteOn && e.cell == 1) ++c1;
                if (e.type == SeqEvent::Type::NoteOn && e.cell == 0) ++c0;
            }
            expectEquals (c1, 0);
            expectEquals (c0, 1);
        }

        beginTest ("Slices: onset detection finds the hits, even divisions are exact");
        {
            // 4 colpi (rumore a decadimento rapido) a 0, 0.5, 1.0 e 1.5 s in 2 s di silenzio altrimenti
            auto d = std::make_shared<SampleData>();
            const int n = (int) (2.0 * kSR);
            d->audio.setSize (1, n);
            d->audio.clear();
            d->sampleRate = kSR;
            juce::Random rnd (7);
            for (const double t0 : { 0.0, 0.5, 1.0, 1.5 })
            {
                const int s0 = (int) (t0 * kSR);
                for (int i = 0; i < (int) (0.2 * kSR) && s0 + i < n; ++i)
                    d->audio.setSample (0, s0 + i, (rnd.nextFloat() * 2.0f - 1.0f) * std::exp (-(float) i / 2000.0f));
            }

            const auto m = detectSlices (*d, 0.6f);
            expectEquals ((int) m.size(), 4);
            if (m.size() == 4)
            {
                expectWithinAbsoluteError (m[0], 0.0f, 1.0e-6f);
                for (int k = 1; k < 4; ++k)
                    expect (std::abs (m[(size_t) k] * 2.0f - 0.5f * (float) k) < 0.012f, "marker " + juce::String (k) + " at " + juce::String (m[(size_t) k] * 2.0f));
            }

            // Regione: tra 0.25 e 0.75 della lunghezza (0.5 s .. 1.5 s): il primo marcatore e' la partenza della regione
            const auto r = detectSlices (*d, 0.6f, 0.25f, 0.75f);
            expect (r.size() >= 2 && std::abs (r[0] - 0.25f) < 1.0e-6f, "region starts at 0.25");
            expect (r.size() == 2 || r.size() == 3, "region markers: " + juce::String ((int) r.size()));

            // Silenzio: solo la partenza
            auto silent = std::make_shared<SampleData>();
            silent->audio.setSize (1, (int) kSR);
            silent->audio.clear();
            silent->sampleRate = kSR;
            expectEquals ((int) detectSlices (*silent, 0.9f).size(), 1);

            const auto ev = evenSlices (0.0f, 1.0f, 8);
            expectEquals ((int) ev.size(), 8);
            expectWithinAbsoluteError (ev[3], 0.375f, 1.0e-6f);
            const auto ev2 = evenSlices (0.25f, 0.75f, 4);
            expectWithinAbsoluteError (ev2[2], 0.5f, 1.0e-6f);
        }

        beginTest ("Tempo from onsets: accented click track at 128 and 140 BPM, with a ragged start");
        {
            for (const double bpm : { 128.0, 140.0, 92.0 })
            {
                auto d = std::make_shared<SampleData>();
                const double secs = 12.0;
                const int n = (int) (secs * kSR);
                d->audio.setSize (1, n);
                d->audio.clear();
                d->sampleRate = kSR;
                juce::Random rnd (11);
                const double beat = 60.0 / bpm;
                const double offset = 0.173;                       // il brano non parte sul primo colpo
                int k = 0;
                for (double t = offset; t < secs - 0.2; t += beat, ++k)
                {
                    const float amp = (k % 4 == 0) ? 1.0f : 0.55f;
                    const int s0 = (int) (t * kSR);
                    for (int i = 0; i < (int) (0.12 * kSR) && s0 + i < n; ++i)
                        d->audio.setSample (0, s0 + i, amp * (rnd.nextFloat() * 2.0f - 1.0f) * std::exp (-(float) i / 1500.0f));
                    // un colpo intermedio piu' debole, sulla croma
                    const int s1 = (int) ((t + beat * 0.5) * kSR);
                    for (int i = 0; i < (int) (0.05 * kSR) && s1 + i < n; ++i)
                        d->audio.setSample (0, s1 + i, 0.25f * (rnd.nextFloat() * 2.0f - 1.0f) * std::exp (-(float) i / 800.0f));
                }
                const double est = estimateBpmFromOnsets (*d);
                expect (std::abs (est - bpm) < bpm * 0.012, "expected " + juce::String (bpm) + " got " + juce::String (est));
            }

            auto silent = std::make_shared<SampleData>();
            silent->audio.setSize (1, (int) (4.0 * kSR));
            silent->audio.clear();
            silent->sampleRate = kSR;
            expectEquals (estimateBpmFromOnsets (*silent), 0.0);
        }

        beginTest ("Slice markers survive a kit save and load");
        {
            auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("sl_slice_test");
            tmp.deleteRecursively();
            tmp.createDirectory();
            Kit k = Kit::makeDefault();
            k.cells[2].slices = { 0.0f, 0.25f, 0.5f };
            juce::String err;
            const auto f = tmp.getChildFile ("Slice.slkit");
            expect (KitIO::save (k, f, err), err);
            Kit back;
            expect (KitIO::load (f, back, err), err);
            expectEquals ((int) back.cells[2].slices.size(), 3);
            expect (back.cells[0].slices.empty());
            tmp.deleteRecursively();
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
