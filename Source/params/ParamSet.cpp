#include "ParamSet.h"

namespace sl
{
namespace
{
struct FDesc
{
    const char* id;
    const char* name;
    const char* unit;
    float min, max, step, def, skewMid;
    int decimals;
};

// Parametri della cella, nello stesso ordine dell'enum CellF
const FDesc kCellF[(size_t) CellF::Count] = {
    { "start",   "Start",   "",   0.0f,   1.0f,  0.0001f, 0.0f,    0.0f, 3 },
    { "end",     "End",     "",   0.0f,   1.0f,  0.0001f, 1.0f,    0.0f, 3 },
    { "level",   "Level",   "dB", -60.0f, 12.0f, 0.1f,    0.0f,    0.0f, 1 },
    { "pan",     "Pan",     "",   -1.0f,  1.0f,  0.01f,   0.0f,    0.0f, 2 },
    { "tune",    "Tune",    "ct", -100.0f, 100.0f, 1.0f,  0.0f,    0.0f, 0 },
    { "pitch",   "Pitch",   "st", -24.0f, 24.0f, 1.0f,    0.0f,    0.0f, 0 },
    { "attack",  "Attack",  "s",  0.0f,   2.0f,  0.001f,  0.0005f, 0.1f, 3 },
    { "hold",    "Hold",    "s",  0.0f,   2.0f,  0.001f,  0.0f,    0.1f, 3 },
    { "decay",   "Decay",   "s",  0.0f,   5.0f,  0.001f,  0.1f,    0.5f, 3 },
    { "sustain", "Sustain", "",   0.0f,   1.0f,  0.01f,   1.0f,    0.0f, 2 },
    { "release", "Release", "s",  0.0f,   5.0f,  0.001f,  0.02f,   0.5f, 3 },
};
float CellParams::* const kCellFM[(size_t) CellF::Count] = {
    &CellParams::start, &CellParams::end, &CellParams::levelDb, &CellParams::pan, &CellParams::tuneCents,
    &CellParams::pitchSemis, &CellParams::attack, &CellParams::hold, &CellParams::decay, &CellParams::sustain,
    &CellParams::release,
};
// Sottogruppo di ogni parametro della cella: 0 Sample, 1 Amp, 2 Pitch
const int kCellFGroup[(size_t) CellF::Count] = { 0, 0, 1, 1, 2, 2, 1, 1, 1, 1, 1 };

// Parametri degli effetti, nello stesso ordine dell'enum FxF
const FDesc kFxF[(size_t) FxF::Count] = {
    { "drive",     "Drive",    "dB", 0.0f,   36.0f,  0.1f,  6.0f,    0.0f,   1 },
    { "mix",       "Mix",      "",   0.0f,   1.0f,   0.01f, 1.0f,    0.0f,   2 },
    { "bits",      "Bits",     "",   1.0f,   16.0f,  1.0f,  12.0f,   0.0f,   0 },
    { "rate",      "Rate",     "x",  1.0f,   40.0f,  0.1f,  4.0f,    8.0f,   1 },
    { "mix",       "Mix",      "",   0.0f,   1.0f,   0.01f, 1.0f,    0.0f,   2 },
    { "cutoff",    "Cutoff",   "Hz", 20.0f,  20000.0f, 1.0f, 8000.0f, 1000.0f, 0 },
    { "reso",      "Resonance", "",  0.3f,   10.0f,  0.01f, 0.707f,  1.0f,   2 },
    { "threshold", "Threshold", "dB", -60.0f, 0.0f,  0.1f,  -18.0f,  0.0f,   1 },
    { "ratio",     "Ratio",    ":1", 1.0f,   20.0f,  0.1f,  4.0f,    4.0f,   1 },
    { "attack",    "Attack",   "ms", 0.1f,   100.0f, 0.1f,  10.0f,   10.0f,  1 },
    { "release",   "Release",  "ms", 10.0f,  1000.0f, 1.0f, 120.0f,  150.0f, 0 },
    { "makeup",    "Makeup",   "dB", 0.0f,   24.0f,  0.1f,  0.0f,    0.0f,   1 },
    { "time",      "Time",     "ms", 1.0f,   2000.0f, 1.0f, 250.0f,  400.0f, 0 },
    { "feedback",  "Feedback", "",   0.0f,   0.95f,  0.01f, 0.35f,   0.0f,   2 },
    { "mix",       "Mix",      "",   0.0f,   1.0f,   0.01f, 0.3f,    0.0f,   2 },
    { "size",      "Size",     "",   0.0f,   1.0f,   0.01f, 0.5f,    0.0f,   2 },
    { "damp",      "Damping",  "",   0.0f,   1.0f,   0.01f, 0.5f,    0.0f,   2 },
    { "mix",       "Mix",      "",   0.0f,   1.0f,   0.01f, 0.25f,   0.0f,   2 },
    { "low",       "Low",      "dB", -15.0f, 15.0f,  0.1f,  0.0f,    0.0f,   1 },
    { "midfreq",   "Mid Freq", "Hz", 100.0f, 8000.0f, 1.0f, 1000.0f, 1000.0f, 0 },
    { "midgain",   "Mid Gain", "dB", -15.0f, 15.0f,  0.1f,  0.0f,    0.0f,   1 },
    { "midq",      "Mid Q",    "",   0.3f,   8.0f,   0.01f, 1.0f,    1.0f,   2 },
    { "high",      "High",     "dB", -15.0f, 15.0f,  0.1f,  0.0f,    0.0f,   1 },
};
float FxParams::* const kFxFM[(size_t) FxF::Count] = {
    &FxParams::drive, &FxParams::satMix, &FxParams::bits, &FxParams::downsample, &FxParams::lofiMix,
    &FxParams::cutoff, &FxParams::resonance, &FxParams::compThreshold, &FxParams::compRatio, &FxParams::compAttack,
    &FxParams::compRelease, &FxParams::compMakeup, &FxParams::delayTime, &FxParams::delayFeedback, &FxParams::delayMix,
    &FxParams::reverbSize, &FxParams::reverbDamp, &FxParams::reverbMix,
    &FxParams::eqLow, &FxParams::eqMidFreq, &FxParams::eqMidGain, &FxParams::eqMidQ, &FxParams::eqHigh,
};
// Effetto a cui appartiene ogni parametro float
const FxB kFxFOwner[(size_t) FxF::Count] = {
    FxB::Sat, FxB::Sat, FxB::Lofi, FxB::Lofi, FxB::Lofi, FxB::Filter, FxB::Filter,
    FxB::Comp, FxB::Comp, FxB::Comp, FxB::Comp, FxB::Comp, FxB::Delay, FxB::Delay, FxB::Delay,
    FxB::Reverb, FxB::Reverb, FxB::Reverb,
    FxB::Eq, FxB::Eq, FxB::Eq, FxB::Eq, FxB::Eq,
};

struct FxName { const char* id; const char* name; };
const FxName kFxNames[(size_t) FxB::Count] = {
    { "sat", "Saturation" }, { "lofi", "LoFi" }, { "filter", "Filter" },
    { "comp", "Compressor" }, { "delay", "Delay" }, { "reverb", "Reverb" }, { "eq", "EQ" },
};
bool FxParams::* const kFxBM[(size_t) FxB::Count] = {
    &FxParams::satOn, &FxParams::lofiOn, &FxParams::filterOn, &FxParams::compOn, &FxParams::delayOn, &FxParams::reverbOn,
    &FxParams::eqOn,
};

juce::NormalisableRange<float> makeRange (const FDesc& d)
{
    juce::NormalisableRange<float> r (d.min, d.max, d.step);
    if (d.skewMid > 0.0f)
        r.setSkewForCentre (d.skewMid);
    return r;
}

// Il testo include l'unita' (es. "2400 Hz"), cosi' si legge uguale nell'interfaccia e nella DAW
std::unique_ptr<juce::AudioParameterFloat> makeFloat (const juce::String& id, const juce::String& name, const FDesc& d)
{
    const juce::String unit = d.unit;
    const int dec = d.decimals;
    auto attr = juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([unit, dec] (float v, int) { return juce::String (v, dec) + (unit.isEmpty() ? juce::String() : " " + unit); })
                    .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, makeRange (d), d.def, attr);
}

std::unique_ptr<juce::AudioParameterBool> makeBool (const juce::String& id, const juce::String& name, bool def)
{
    auto attr = juce::AudioParameterBoolAttributes()
                    .withStringFromValueFunction ([] (bool v, int) { return v ? juce::String ("On") : juce::String ("Off"); });
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def, attr);
}

std::unique_ptr<juce::AudioParameterChoice> makeChoice (const juce::String& id, const juce::String& name,
                                                        const juce::StringArray& items, int def)
{
    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def);
}

void setValue (juce::RangedAudioParameter* p, float plain)
{
    p->setValueNotifyingHost (p->convertTo0to1 (plain));
}
}   // namespace

// ------------------------------------------------------------------------------------------
// Costruzione dei parametri: gruppi per cella (Sample / Amp / Pitch / effetti) e per il master.

ParamSet::ParamSet()
{
    // Crea i parametri degli effetti dentro il gruppo 'parent' (un sottogruppo per effetto)
    auto buildFx = [this] (juce::AudioProcessorParameterGroup& parent, FxRefs& refs, const juce::String& idPrefix, const juce::String& namePrefix)
    {
        std::array<std::unique_ptr<juce::AudioProcessorParameterGroup>, (size_t) FxB::Count> sub;
        for (size_t b = 0; b < (size_t) FxB::Count; ++b)
        {
            const auto& n = kFxNames[b];
            sub[b] = std::make_unique<juce::AudioProcessorParameterGroup> (idPrefix + "." + n.id, namePrefix + n.name, "|");

            auto on = makeBool (idPrefix + "." + n.id + ".on", namePrefix + n.name + " On", false);
            refs.on[b] = on.get();
            allParams.push_back (on.get());
            sub[b]->addChild (std::move (on));

            if (b == (size_t) FxB::Filter)
            {
                auto type = makeChoice (idPrefix + ".filter.type", namePrefix + "Filter Type", { "Lowpass", "Highpass", "Bandpass" }, 0);
                refs.filterType = type.get();
                allParams.push_back (type.get());
                sub[b]->addChild (std::move (type));
            }

            if (b == (size_t) FxB::Delay)
            {
                // Sync al tempo: i valori musicali compaiono per nome nella DAW (1/8, 1/16T...)
                auto sync = makeBool (idPrefix + ".delay.sync", namePrefix + "Delay Sync", false);
                refs.delaySync = sync.get();
                allParams.push_back (sync.get());
                sub[b]->addChild (std::move (sync));

                juce::StringArray divisions;
                for (const char* name : kDivisionNames)
                    divisions.add (name);
                auto division = makeChoice (idPrefix + ".delay.division", namePrefix + "Delay Division", divisions, 7);
                refs.delayDivision = division.get();
                allParams.push_back (division.get());
                sub[b]->addChild (std::move (division));
            }
        }

        for (size_t f = 0; f < (size_t) FxF::Count; ++f)
        {
            const auto owner = (size_t) kFxFOwner[f];
            const auto& n = kFxNames[owner];
            auto p = makeFloat (idPrefix + "." + n.id + "." + kFxF[f].id, namePrefix + n.name + " " + kFxF[f].name, kFxF[f]);
            refs.f[f] = p.get();
            allParams.push_back (p.get());
            sub[owner]->addChild (std::move (p));
        }

        for (auto& s : sub)
            parent.addChild (std::move (s));
    };

    for (int c = 0; c < kNumCells; ++c)
    {
        auto& refs = cells[(size_t) c];
        const juce::String idPrefix = "c" + juce::String (c).paddedLeft ('0', 2);
        const juce::String label = cellLabel (c, 12);
        const juce::String namePrefix = label + " ";

        auto cellGroup = std::make_unique<juce::AudioProcessorParameterGroup> (idPrefix, "Cell " + label, "|");

        const char* subNames[3] = { "Sample", "Amp", "Pitch" };
        std::array<std::unique_ptr<juce::AudioProcessorParameterGroup>, 3> subs;
        for (int i = 0; i < 3; ++i)
            subs[(size_t) i] = std::make_unique<juce::AudioProcessorParameterGroup> (idPrefix + "." + juce::String (subNames[i]).toLowerCase(),
                                                                                    namePrefix + subNames[i], "|");

        for (size_t f = 0; f < (size_t) CellF::Count; ++f)
        {
            auto p = makeFloat (idPrefix + "." + kCellF[f].id, namePrefix + kCellF[f].name, kCellF[f]);
            refs.f[f] = p.get();
            allParams.push_back (p.get());
            subs[(size_t) kCellFGroup[f]]->addChild (std::move (p));
        }

        auto rev = makeBool (idPrefix + ".reverse", namePrefix + "Reverse", false);
        refs.reverse = rev.get();
        allParams.push_back (rev.get());
        subs[0]->addChild (std::move (rev));

        auto mute = makeBool (idPrefix + ".mute", namePrefix + "Mute", false);
        refs.mute = mute.get();
        allParams.push_back (mute.get());
        subs[0]->addChild (std::move (mute));

        auto solo = makeBool (idPrefix + ".solo", namePrefix + "Solo", false);
        refs.solo = solo.get();
        allParams.push_back (solo.get());
        subs[0]->addChild (std::move (solo));

        auto mode = makeChoice (idPrefix + ".mode", namePrefix + "Trigger Mode", { "One-shot", "Gate", "Loop" }, 0);
        refs.mode = mode.get();
        allParams.push_back (mode.get());
        subs[0]->addChild (std::move (mode));

        for (auto& s : subs)
            cellGroup->addChild (std::move (s));

        buildFx (*cellGroup, refs.fx, idPrefix, namePrefix);
        groups.push_back (std::move (cellGroup));
    }

    // Master
    auto master = std::make_unique<juce::AudioProcessorParameterGroup> ("master", "Master", "|");
    auto level = makeFloat ("master.level", "Master Level", FDesc { "level", "Level", "dB", -60.0f, 12.0f, 0.1f, 0.0f, 0.0f, 1 });
    masterLevelParam = level.get();
    allParams.push_back (level.get());
    master->addChild (std::move (level));
    buildFx (*master, masterFxRefs, "master", "Master ");
    groups.push_back (std::move (master));

    for (auto* p : allParams)
        byId[p->getParameterID()] = p;
}

std::vector<std::unique_ptr<juce::AudioProcessorParameterGroup>> ParamSet::releaseGroups()
{
    return std::move (groups);
}

// ------------------------------------------------------------------------------------------
// Lettura nel thread audio

namespace
{
void fillFx (const FxRefs& r, FxParams& out) noexcept
{
    for (size_t f = 0; f < (size_t) FxF::Count; ++f)
        out.*kFxFM[f] = r.f[f]->get();
    for (size_t b = 0; b < (size_t) FxB::Count; ++b)
        out.*kFxBM[b] = r.on[b]->get();
    out.filterType = r.filterType->getIndex();
    out.delaySync = r.delaySync->get();
    out.delayDivision = r.delayDivision->getIndex();
}
}

void ParamSet::fillLive (Engine::LiveParams& live) const noexcept
{
    for (int c = 0; c < kNumCells; ++c)
    {
        const auto& r = cells[(size_t) c];
        auto& d = live.dsp[(size_t) c];
        d.start = juce::jlimit (0.0f, 1.0f, r.at (CellF::Start)->get());
        d.end = juce::jlimit (0.0f, 1.0f, r.at (CellF::End)->get());
        d.gain = dbToGain (r.at (CellF::Level)->get());
        d.pan = r.at (CellF::Pan)->get();
        d.tuneCents = r.at (CellF::Tune)->get();
        d.pitchSemis = r.at (CellF::Pitch)->get();
        d.attack = r.at (CellF::Attack)->get();
        d.hold = r.at (CellF::Hold)->get();
        d.decay = r.at (CellF::Decay)->get();
        d.sustain = r.at (CellF::Sustain)->get();
        d.release = r.at (CellF::Release)->get();
        d.reverse = r.reverse->get();
        d.mode = (TriggerMode) r.mode->getIndex();
        live.mute[(size_t) c] = r.mute->get();
        live.solo[(size_t) c] = r.solo->get();
        fillFx (r.fx, live.fx[(size_t) c]);
    }

    fillFx (masterFxRefs, live.masterFx);
    live.masterGain = dbToGain (masterLevelParam->get());
}

// ------------------------------------------------------------------------------------------
// Kit <-> parametri

namespace
{
void applyFx (const FxRefs& r, const FxParams& fx)
{
    for (size_t f = 0; f < (size_t) FxF::Count; ++f)
        setValue (r.f[f], fx.*kFxFM[f]);
    for (size_t b = 0; b < (size_t) FxB::Count; ++b)
        setValue (r.on[b], fx.*kFxBM[b] ? 1.0f : 0.0f);
    setValue (r.filterType, (float) fx.filterType);
    setValue (r.delaySync, fx.delaySync ? 1.0f : 0.0f);
    setValue (r.delayDivision, (float) fx.delayDivision);
}

void applyCell (const CellRefs& r, const CellParams& c)
{
    for (size_t f = 0; f < (size_t) CellF::Count; ++f)
        setValue (r.f[f], c.*kCellFM[f]);
    setValue (r.reverse, c.reverse ? 1.0f : 0.0f);
    setValue (r.mute, c.mute ? 1.0f : 0.0f);
    setValue (r.solo, c.solo ? 1.0f : 0.0f);
    setValue (r.mode, (float) (int) c.mode);
    applyFx (r.fx, c.fx);
}
}

void ParamSet::readCellFromKit (int cell, const Kit& kit)
{
    if (cell < 0 || cell >= kNumCells || cell >= kit.numCells())
        return;
    applyCell (cells[(size_t) cell], kit.cells[(size_t) cell]);
}

void ParamSet::readFromKit (const Kit& kit)
{
    const CellParams defaults;      // celle non presenti nel kit: valori predefiniti
    for (int c = 0; c < kNumCells; ++c)
        applyCell (cells[(size_t) c], c < kit.numCells() ? kit.cells[(size_t) c] : defaults);

    setValue (masterLevelParam, kit.masterDb);
    applyFx (masterFxRefs, kit.masterFx);
}

void ParamSet::writeToKit (Kit& kit) const
{
    for (int c = 0; c < juce::jmin (kNumCells, kit.numCells()); ++c)
    {
        auto& out = kit.cells[(size_t) c];
        const auto& r = cells[(size_t) c];
        for (size_t f = 0; f < (size_t) CellF::Count; ++f)
            out.*kCellFM[f] = r.f[f]->get();
        out.reverse = r.reverse->get();
        out.mute = r.mute->get();
        out.solo = r.solo->get();
        out.mode = (TriggerMode) r.mode->getIndex();

        for (size_t f = 0; f < (size_t) FxF::Count; ++f)
            out.fx.*kFxFM[f] = r.fx.f[f]->get();
        for (size_t b = 0; b < (size_t) FxB::Count; ++b)
            out.fx.*kFxBM[b] = r.fx.on[b]->get();
        out.fx.filterType = r.fx.filterType->getIndex();
        out.fx.delaySync = r.fx.delaySync->get();
        out.fx.delayDivision = r.fx.delayDivision->getIndex();
    }

    kit.masterDb = masterLevelParam->get();
    for (size_t f = 0; f < (size_t) FxF::Count; ++f)
        kit.masterFx.*kFxFM[f] = masterFxRefs.f[f]->get();
    for (size_t b = 0; b < (size_t) FxB::Count; ++b)
        kit.masterFx.*kFxBM[b] = masterFxRefs.on[b]->get();
    kit.masterFx.filterType = masterFxRefs.filterType->getIndex();
    kit.masterFx.delaySync = masterFxRefs.delaySync->get();
    kit.masterFx.delayDivision = masterFxRefs.delayDivision->getIndex();
}
}
