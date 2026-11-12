#include "MidiLearn.h"

namespace sl
{
void MidiLearn::clearTable()
{
    for (auto& slots : table)
        for (auto& s : slots)
            s.store (nullptr, std::memory_order_relaxed);
}

void MidiLearn::rebuildTable()
{
    // Il thread audio legge la tabella senza lock: ogni cella e' un atomico, quindi al massimo vede
    // per un istante una versione intermedia (mai un puntatore non valido).
    clearTable();
    for (const auto& e : entries)
        for (auto& slot : table[(size_t) e.cc])
            if (slot.load (std::memory_order_relaxed) == nullptr)
            {
                slot.store (e.param, std::memory_order_release);
                break;
            }
}

void MidiLearn::startLearn (juce::RangedAudioParameter* p)
{
    pendingParam = p;
    learnedCc.store (-1);
    learnTarget.store (p);
}

void MidiLearn::cancelLearn()
{
    learnTarget.store (nullptr);
    learnedCc.store (-1);
    pendingParam = nullptr;
}

int MidiLearn::ccFor (const juce::RangedAudioParameter* p) const
{
    for (const auto& e : entries)
        if (e.param == p)
            return e.cc;
    return -1;
}

void MidiLearn::clear (const juce::RangedAudioParameter* p)
{
    entries.erase (std::remove_if (entries.begin(), entries.end(), [p] (const Entry& e) { return e.param == p; }), entries.end());
    rebuildTable();
}

void MidiLearn::clearAll()
{
    entries.clear();
    rebuildTable();
}

bool MidiLearn::poll()
{
    const int cc = learnedCc.exchange (-1);
    if (cc < 0 || pendingParam == nullptr)
        return false;

    // Un parametro ha un solo CC: la nuova assegnazione sostituisce la precedente
    entries.erase (std::remove_if (entries.begin(), entries.end(),
                                   [this] (const Entry& e) { return e.param == pendingParam; }), entries.end());
    entries.push_back ({ cc, pendingParam });
    pendingParam = nullptr;
    rebuildTable();
    return true;
}

std::vector<MidiMapping> MidiLearn::mappings() const
{
    std::vector<MidiMapping> out;
    for (const auto& e : entries)
        out.push_back ({ e.cc, e.param->getParameterID() });
    return out;
}

void MidiLearn::setMappings (const std::vector<MidiMapping>& list)
{
    cancelLearn();
    entries.clear();
    for (const auto& m : list)
        if (auto* p = params.find (m.paramId); p != nullptr && m.cc >= 0 && m.cc < 128)
            entries.push_back ({ m.cc, p });
    rebuildTable();
}

void MidiLearn::process (const juce::MidiBuffer& midi) noexcept
{
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isController())
            continue;

        const int cc = m.getControllerNumber();
        const int value = m.getControllerValue();

        // Durante l'apprendimento il primo CC ricevuto viene catturato e non muove nulla
        if (learnTarget.load (std::memory_order_acquire) != nullptr)
        {
            learnedCc.store (cc);
            learnTarget.store (nullptr);
            continue;
        }

        for (auto& slot : table[(size_t) cc])
            if (auto* p = slot.load (std::memory_order_acquire))
            {
                // Interruttori: acceso da 64 in su; gli altri seguono il CC (0..127) sull'intero intervallo
                const float norm = p->isBoolean() ? (value >= 64 ? 1.0f : 0.0f) : (float) value / 127.0f;
                p->setValueNotifyingHost (norm);
            }
    }
}
}
