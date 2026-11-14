#include "ModulesBar.h"

ModulesBar::ModulesBar (NinaProcessor& p)
    : processor (p), meter ([this] { return processor.getMasterLevels(); })
{
    using C = sl::CellF;
    addKnob (Amp, "Attack", C::Attack);
    addKnob (Amp, "Hold", C::Hold);
    addKnob (Amp, "Decay", C::Decay);
    addKnob (Amp, "Sustain", C::Sustain);
    addKnob (Amp, "Release", C::Release);
    addKnob (Amp, "Level", C::Level);
    addKnob (Amp, "Pan", C::Pan);
    addKnob (Pitch, "Tune", C::Tune);
    addKnob (Pitch, "Pitch", C::Pitch);

    addAndMakeVisible (envelope);
    addAndMakeVisible (masterKnob);
    addAndMakeVisible (meter);

    // MIDI learn sul volume master
    auto masterParam = [this] { return (juce::RangedAudioParameter*) processor.getParams().masterLevel(); };
    masterKnob.ccQuery = [this, masterParam] { return processor.getMidiLearn().ccFor (masterParam()); };
    masterKnob.learningQuery = [this, masterParam] { return processor.getMidiLearn().isLearning (masterParam()); };
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, masterKnob, masterParam));

    setCell (0);
    startTimerHz (15);
}

void ModulesBar::addKnob (int module, const juce::String& title, sl::CellF which)
{
    Param p;
    p.box = std::make_unique<KnobBox> (title);
    p.module = module;
    p.which = which;
    addAndMakeVisible (*p.box);

    // MIDI learn: il parametro dipende dalla cella selezionata, quindi si risolve al momento dell'uso
    auto param = [this, which] { return (juce::RangedAudioParameter*) processor.getParams().cell (cell).at (which); };
    p.box->ccQuery = [this, param] { return processor.getMidiLearn().ccFor (param()); };
    p.box->learningQuery = [this, param] { return processor.getMidiLearn().isLearning (param()); };
    learnMenus.push_back (std::make_unique<LearnMenu> (processor, *p.box, param));

    params.push_back (std::move (p));
}

void ModulesBar::setCell (int c)
{
    cell = juce::jlimit (0, sl::kNumCells - 1, c);
    refresh();
}

void ModulesBar::refresh()
{
    const auto& refs = processor.getParams().cell (cell);

    // Attachment: il valore segue il parametro (automazione inclusa) e ogni gesto e' segnalato alla DAW
    for (auto& p : params)
    {
        p.attachment.reset();
        p.attachment = std::make_unique<juce::SliderParameterAttachment> (*refs.at (p.which), p.box->slider);
    }

    masterAttachment.reset();
    masterAttachment = std::make_unique<juce::SliderParameterAttachment> (*processor.getParams().masterLevel(), masterKnob.slider);

    updateEnvelope();
    repaint();
}

void ModulesBar::timerCallback()
{
    updateEnvelope();       // il disegno dell'inviluppo segue anche i valori automatizzati
    for (auto& p : params)
        p.box->refreshIndicator();
    masterKnob.refreshIndicator();
}

void ModulesBar::updateEnvelope()
{
    const auto& r = processor.getParams().cell (cell);
    const float v[5] = { r.at (sl::CellF::Attack)->get(), r.at (sl::CellF::Hold)->get(), r.at (sl::CellF::Decay)->get(),
                         r.at (sl::CellF::Sustain)->get(), r.at (sl::CellF::Release)->get() };
    bool changed = false;
    for (int i = 0; i < 5; ++i)
        if (v[i] != lastEnv[i]) { lastEnv[i] = v[i]; changed = true; }

    if (changed)
        envelope.setParams (v[0], v[1], v[2], v[3], v[4]);
}

void ModulesBar::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const char* titles[numModules] = { "Amp", "Pitch", "Master" };
    for (int i = 0; i < numModules; ++i)
        widgets::drawModule (g, moduleBounds[i].toFloat(), titles[i], s);
}

void ModulesBar::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds();
    const int gap = juce::jmax (6, (int) (8 * s));

    // Larghezze proporzionali al numero di manopole di ogni modulo
    const float units[numModules] = { 7.0f, 2.4f, 2.6f };
    float total = 0.0f;
    for (float u : units) total += u;
    const float perUnit = (float) (area.getWidth() - gap * (numModules - 1)) / total;

    int x = area.getX();
    for (int i = 0; i < numModules; ++i)
    {
        const int w = i == numModules - 1 ? area.getRight() - x : (int) (units[i] * perUnit);
        moduleBounds[i] = { x, area.getY(), w, area.getHeight() };
        x += w + gap;
    }

    const int titleH = (int) (24 * s);
    auto inner = [&] (int m) { return moduleBounds[m].reduced ((int) (6 * s), 0).withTrimmedTop (titleH).withTrimmedBottom ((int) (6 * s)); };

    for (int m = 0; m < Master; ++m)
    {
        std::vector<KnobBox*> list;
        for (auto& p : params)
            if (p.module == m)
                list.push_back (p.box.get());

        auto r = inner (m);
        if (m == Amp)
        {
            // Sotto le manopole, il disegno dell'inviluppo
            const int graphH = juce::jmax (0, (int) (r.getHeight() * 0.30f));
            envelope.setBounds (r.removeFromBottom (graphH).reduced ((int) (6 * s), (int) (2 * s)));
        }
        const int w = r.getWidth() / (int) list.size();
        for (auto* k : list)
            k->setBounds (r.removeFromLeft (w));
    }

    // Master: manopola + meter
    auto mr = inner (Master);
    meter.setBounds (mr.removeFromRight (juce::jmax (18, (int) (30 * s))).reduced (0, (int) (6 * s)));
    masterKnob.setBounds (mr);
}
