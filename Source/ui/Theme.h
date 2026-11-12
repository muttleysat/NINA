#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{
inline const juce::Colour background { 0xff101216 };
inline const juce::Colour panel      { 0xff1a1d23 };
inline const juce::Colour panelHi    { 0xff242830 };
inline const juce::Colour accent     { 0xff3ec1d3 };
inline const juce::Colour text       { 0xffdde2e8 };
inline const juce::Colour textDim    { 0xff8b94a0 };
inline const juce::Colour danger     { 0xffe0556a };

// Dimensione di riferimento: tutti i componenti scalano rispetto alla larghezza della finestra
constexpr float baseWidth = 1280.0f;

inline float scaleFor (const juce::Component& c) { return juce::jmax (0.6f, (float) c.getTopLevelComponent()->getWidth() / baseWidth); }

inline juce::String noteName (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 3); }   // C3 = 60 (Cubase)
}
