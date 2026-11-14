#include "WaveStrip.h"

WaveStrip::WaveStrip (NinaProcessor& p) : processor (p)
{
    addAndMakeVisible (waveform);

    // Gesto verso la DAW: inizia quando si afferra un marker, finisce quando si rilascia
    waveform.onRegionDragStart = [this]
    {
        startParam()->beginChangeGesture();
        endParam()->beginChangeGesture();
    };
    waveform.onRegionChanged = [this] (float s, float e)
    {
        startParam()->setValueNotifyingHost (startParam()->convertTo0to1 (s));
        endParam()->setValueNotifyingHost (endParam()->convertTo0to1 (e));
        shownStart = s;
        shownEnd = e;
        repaint (readout);
    };
    waveform.onRegionCommitted = [this]
    {
        startParam()->endChangeGesture();
        endParam()->endChangeGesture();
    };

    setCell (0);
    startTimerHz (20);
}

juce::AudioParameterFloat* WaveStrip::startParam() const { return processor.getParams().cell (cell).at (sl::CellF::Start); }
juce::AudioParameterFloat* WaveStrip::endParam() const { return processor.getParams().cell (cell).at (sl::CellF::End); }

void WaveStrip::setCell (int c)
{
    cell = juce::jlimit (0, sl::kNumCells - 1, c);
    refresh();
}

void WaveStrip::refresh()
{
    const auto& c = processor.getKit().cells[(size_t) cell];
    waveform.setSample (processor.getCellData (cell), juce::Colour (c.colour));
    shownStart = startParam()->get();
    shownEnd = endParam()->get();
    waveform.setRegion (shownStart, shownEnd);
    waveform.setEnabled (c.hasSample() && ! c.missing);
    repaint();
}

void WaveStrip::timerCallback()
{
    // Start/End possono cambiare dall'automazione della DAW: si aggiornano marker e letture
    if (waveform.isDragging())
        return;

    const float s = startParam()->get(), e = endParam()->get();
    if (s != shownStart || e != shownEnd)
    {
        shownStart = s;
        shownEnd = e;
        waveform.setRegion (s, e);
        repaint (readout);
    }
}

double WaveStrip::lengthMs() const
{
    const auto data = processor.getCellData (cell);
    return data != nullptr ? 1000.0 * (double) data->numSamples() / data->sampleRate : 0.0;
}

void WaveStrip::paint (juce::Graphics& g)
{
    const float s = theme::scaleFor (*this);
    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) cell];
    const auto col = juce::Colour (c.colour);

    // ID e nome
    g.setColour (theme::panel);
    g.fillRoundedRectangle (idBox.toFloat(), 8.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (idBox.toFloat().reduced (0.5f), 8.0f * s, 1.0f);

    auto inner = idBox.reduced ((int) (10 * s), (int) (8 * s));
    g.setColour (col);
    g.setFont (juce::FontOptions (juce::jmax (12.0f, 19.0f * s), juce::Font::bold));
    g.drawText (sl::cellLabel (cell, kit.cols), inner.removeFromTop ((int) (24 * s)), juce::Justification::topLeft);
    g.setColour (theme::text.withAlpha (0.8f));
    g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s)));
    g.drawFittedText (c.hasSample() ? c.name : juce::String ("(empty)"), inner, juce::Justification::topLeft, 2);

    // Letture Start / End / Length
    if (readout.isEmpty())
        return;

    g.setColour (theme::panel);
    g.fillRoundedRectangle (readout.toFloat(), 8.0f * s);
    g.setColour (theme::border);
    g.drawRoundedRectangle (readout.toFloat().reduced (0.5f), 8.0f * s, 1.0f);

    const double len = lengthMs();
    const double st = (double) startParam()->get() * len, en = (double) endParam()->get() * len;
    auto fmt = [] (double ms) { return ms >= 1000.0 ? juce::String (ms / 1000.0, 3) + " s" : juce::String (ms, 1) + " ms"; };
    const juce::String vals[3] = { fmt (st), fmt (en), fmt (en - st) };
    const char* names[3] = { "Start", "End", "Length" };

    auto area = readout.reduced ((int) (8 * s), (int) (8 * s));
    const int w = area.getWidth() / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto r = area.removeFromLeft (w);
        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s)));
        g.drawText (names[i], r.removeFromTop (r.getHeight() / 2), juce::Justification::centredBottom);
        g.setColour (c.hasSample() ? theme::accentSoft : theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 11.5f * s), juce::Font::bold));
        g.drawText (c.hasSample() ? vals[i] : juce::String ("-"), r, juce::Justification::centredTop);
    }
}

void WaveStrip::resized()
{
    const float s = theme::scaleFor (*this);
    auto area = getLocalBounds();
    const int gap = (int) (8 * s);

    idBox = area.removeFromLeft (juce::jlimit (80, 160, (int) (110 * s)));
    area.removeFromLeft (gap);

    // Con poco spazio le letture si nascondono
    readout = {};
    if (area.getWidth() > (int) (560 * s))
    {
        readout = area.removeFromRight ((int) (270 * s));
        area.removeFromRight (gap);
    }
    waveform.setBounds (area);
}
