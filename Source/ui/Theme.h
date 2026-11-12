#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace theme
{
inline const juce::Colour background { 0xff0d0e11 };
inline const juce::Colour panel      { 0xff16181c };
inline const juce::Colour panelHi    { 0xff21242b };
inline const juce::Colour border     { 0xff2c3038 };
inline const juce::Colour accent     { 0xffff7a1f };   // arancione
inline const juce::Colour accentSoft { 0xffffa45e };
inline const juce::Colour text       { 0xffe9eaee };
inline const juce::Colour textDim    { 0xff8b909a };
inline const juce::Colour danger     { 0xffe0556a };

// Dimensione di riferimento: tutti i componenti scalano rispetto alla larghezza della finestra
constexpr float baseWidth = 1280.0f;

inline float scaleFor (const juce::Component& c) { return juce::jmax (0.6f, (float) c.getTopLevelComponent()->getWidth() / baseWidth); }

// I pulsanti non prendono il focus da tastiera: cosi' la barra spaziatrice resta il Play/Stop del sequencer
// e non "preme" l'ultimo pulsante cliccato.
inline void noButtonFocus (juce::Component& c)
{
    for (auto* child : c.getChildren())
    {
        if (auto* b = dynamic_cast<juce::Button*> (child))
        {
            b->setWantsKeyboardFocus (false);
            b->setMouseClickGrabsKeyboardFocus (false);
        }
        noButtonFocus (*child);
    }
}

inline juce::String noteName (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 3); }   // C3 = 60 (Cubase)

inline juce::String cellId (int cell, int cols)
{
    return juce::String::charToString ((juce::juce_wchar) ('A' + cell / cols)) + juce::String (cell % cols + 1);
}
}
