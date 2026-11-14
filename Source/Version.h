#pragma once

#include <juce_core/juce_core.h>

namespace sl
{
// Confronto di versioni "MAJOR.MINOR.PATCH" (con o senza "v" davanti, es. "v1.0.2"): true se 'latest' e' piu' recente di 'current'.
// I campi mancanti valgono 0; testo non numerico (es. "-beta") dopo il numero viene ignorato.
inline bool isNewerVersion (const juce::String& latest, const juce::String& current)
{
    auto parse = [] (juce::String v)
    {
        v = v.trim();
        if (v.startsWithIgnoreCase ("v"))
            v = v.substring (1);
        std::array<int, 3> n { 0, 0, 0 };
        const auto parts = juce::StringArray::fromTokens (v, ".", "");
        for (int i = 0; i < 3 && i < parts.size(); ++i)
            n[(size_t) i] = parts[i].retainCharacters ("0123456789").getIntValue();
        return n;
    };
    return parse (latest) > parse (current);
}
}
