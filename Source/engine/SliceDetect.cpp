#include "SliceDetect.h"
#include <algorithm>
#include <cmath>

namespace sl
{
std::vector<float> evenSlices (float start, float end, int count)
{
    std::vector<float> out;
    count = std::max (1, count);
    start = juce::jlimit (0.0f, 1.0f, start);
    end = juce::jlimit (start, 1.0f, end);
    for (int i = 0; i < count; ++i)
        out.push_back (start + (end - start) * (float) i / (float) count);
    return out;
}

std::vector<float> detectSlices (const SampleData& d, float sensitivity, float start, float end)
{
    const int total = d.numSamples();
    start = juce::jlimit (0.0f, 1.0f, start);
    end = juce::jlimit (start, 1.0f, end);
    std::vector<float> markers { start };

    const int a = (int) std::floor (start * (float) total), b = (int) std::floor (end * (float) total);
    const int len = b - a;
    const double sr = juce::jmax (8000.0, d.sampleRate);
    const int hop = juce::jmax (32, (int) (sr * 0.004));            // 4 ms
    const int frame = hop * 4;
    if (total <= 0 || len < frame * 3)
        return markers;

    // Segnale mono della regione
    std::vector<float> x ((size_t) len, 0.0f);
    const int channels = d.numChannels();
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* src = d.audio.getReadPointer (ch) + a;
        for (int i = 0; i < len; ++i)
            x[(size_t) i] += src[i] / (float) channels;
    }

    // Energia (in log10) della differenza prima del segnale: enfatizza gli attacchi rispetto al corpo del suono
    const int frames = (len - frame) / hop + 1;
    std::vector<float> e ((size_t) frames, 0.0f);
    float eMax = -100.0f;
    for (int k = 0; k < frames; ++k)
    {
        double sum = 0.0;
        const int from = k * hop;
        for (int i = from; i < from + frame; ++i)
        {
            const double df = (double) x[(size_t) i] - (i > 0 ? (double) x[(size_t) i - 1] : 0.0);
            sum += df * df;
        }
        e[(size_t) k] = (float) std::log10 (1.0e-10 + sum / (double) frame);
        eMax = std::max (eMax, e[(size_t) k]);
    }

    // Flusso: aumento positivo di energia da un frame al successivo
    std::vector<float> flux ((size_t) frames, 0.0f);
    for (int k = 1; k < frames; ++k)
        flux[(size_t) k] = std::max (0.0f, e[(size_t) k] - e[(size_t) k - 1]);

    const float sens = juce::jlimit (0.0f, 1.0f, sensitivity);
    const float threshold = juce::jmap (sens, 1.0f, 0.12f);                                  // aumento minimo (log10 dell'energia)
    const double minGapSeconds = juce::jmap (sens, 0.16f, 0.045f);
    const int minGapFrames = juce::jmax (2, (int) (minGapSeconds * sr / (double) hop));
    const float floorLevel = eMax - 5.0f;                                                     // niente colpi nel rumore di fondo

    // Picchi del flusso sopra soglia, con distanza minima (tra due vicini si tiene il piu' forte)
    std::vector<int> peaks;
    for (int k = 2; k < frames - 1; ++k)
    {
        const float f = flux[(size_t) k];
        if (f < threshold || e[(size_t) k] < floorLevel)
            continue;
        if (f < flux[(size_t) k - 1] || f <= flux[(size_t) k + 1])
            continue;                                                                          // non e' un massimo locale
        if (! peaks.empty() && k - peaks.back() < minGapFrames)
        {
            if (f > flux[(size_t) peaks.back()])
                peaks.back() = k;
            continue;
        }
        peaks.push_back (k);
    }

    // Posizione precisa: il primo punto in cui l'ampiezza sale davvero vicino al picco, poi indietro fino a un passaggio per lo zero
    const int minFromStart = (int) (sr * 0.03);                                               // il primo marcatore e' gia' 'start'
    for (int k : peaks)
    {
        const int center = k * hop + frame - hop;
        const int w0 = juce::jmax (1, center - 2 * hop), w1 = juce::jmin (len - 2, center + 2 * hop);
        if (w1 <= w0)
            continue;

        float mx = 0.0f;
        for (int i = w0; i <= w1; ++i)
            mx = std::max (mx, std::abs (x[(size_t) i]));
        int pos = w0;
        for (int i = w0; i <= w1; ++i)
            if (std::abs (x[(size_t) i]) >= 0.25f * mx)
            {
                pos = i;
                break;
            }
        for (int back = 0; back < 256 && pos > 1; ++back, --pos)
            if ((x[(size_t) pos - 1] <= 0.0f) != (x[(size_t) pos] <= 0.0f))
                break;

        if (pos < minFromStart)
            continue;
        const float norm = (float) (a + pos) / (float) total;
        if (norm > markers.back() && norm < end)
            markers.push_back (norm);
    }
    return markers;
}
}
