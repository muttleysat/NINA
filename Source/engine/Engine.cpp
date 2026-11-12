#include "Engine.h"

namespace sl
{
void Engine::prepare (double sr)
{
    sampleRate = sr;
    previewVoice.kill();
    for (auto& v : voices)
        v.kill();
}

void Engine::playPreview (const SampleData* data, float gain)
{
    auto snap = std::make_shared<KitSnapshot>();
    if (data != nullptr)
    {
        CellRuntime r;
        r.cellIndex = -1;
        r.data = data;
        r.dsp.gain = gain;
        r.dsp.rootNote = r.dsp.lowNote = r.dsp.highNote = 60;
        r.dsp.attack = 0.001f;
        r.dsp.release = 0.02f;
        snap->cells.push_back (r);
    }
    previewExchange.publish (std::move (snap));
}

void Engine::stopPreview()
{
    playPreview (nullptr, 1.0f);
}

void Engine::noteOn (int note, float velocity) noexcept
{
    if (current == nullptr)
        return;

    for (const auto& cell : current->cells)
    {
        if (cell.data == nullptr || note < cell.dsp.lowNote || note > cell.dsp.highNote)
            continue;

        // Voce libera, altrimenti si ruba la piu' vecchia
        Voice* target = nullptr;
        for (auto& v : voices)
            if (! v.isActive()) { target = &v; break; }

        if (target == nullptr)
        {
            target = &voices[0];
            for (auto& v : voices)
                if (v.getAge() < target->getAge())
                    target = &v;
        }

        target->start (cell, note, velocity, sampleRate, ++ageCounter);
    }
}

void Engine::noteOff (int note) noexcept
{
    for (auto& v : voices)
        if (v.isActive() && v.getNote() == note && v.getMode() != TriggerMode::OneShot)
            v.release();
}

void Engine::renderVoices (float* l, float* r, int start, int count) noexcept
{
    if (count <= 0)
        return;

    for (auto& v : voices)
        if (v.isActive())
            v.render (l + start, r + start, count);
}

void Engine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi) noexcept
{
    if (exchange.tryAcquire (current) && current != nullptr)
    {
        if (current->generation != lastGeneration)
        {
            for (auto& v : voices)
                v.kill();
            lastGeneration = current->generation;
        }
        else
        {
            // Stesso kit con parametri modificati: le voci in corso seguono subito i nuovi valori
            for (auto& v : voices)
                if (v.isActive() && v.getCellIndex() >= 0 && v.getCellIndex() < (int) current->cells.size())
                    v.updateParams (current->cells[(size_t) v.getCellIndex()]);
        }
    }

    if (buffer.getNumChannels() < 1)
        return;

    const int numSamples = buffer.getNumSamples();
    float* l = buffer.getWritePointer (0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;

    // Anteprima del browser: una voce dedicata, indipendente dal kit
    if (previewExchange.tryAcquire (previewCurrent) && previewCurrent != nullptr)
    {
        previewVoice.kill();
        if (! previewCurrent->cells.empty())
            previewVoice.start (previewCurrent->cells[0], 60, 1.0f, sampleRate, 0);
    }
    previewVoice.render (l, r, numSamples);

    if (current == nullptr)
        return;

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int at = juce::jlimit (pos, numSamples, meta.samplePosition);
        renderVoices (l, r, pos, at - pos);
        pos = at;

        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            for (auto& v : voices)
                v.release();
    }
    renderVoices (l, r, pos, numSamples - pos);

    if (current->masterGain != 1.0f)
        buffer.applyGain (current->masterGain);

    for (auto& a : activity)
        a.store (0, std::memory_order_relaxed);
    for (const auto& v : voices)
        if (v.isActive() && v.getCellIndex() >= 0 && v.getCellIndex() < kMaxCells)
            activity[(size_t) v.getCellIndex()].fetch_add (1, std::memory_order_relaxed);
}
}
