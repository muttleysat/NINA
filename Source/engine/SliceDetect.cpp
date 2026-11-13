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

double estimateBpmFromOnsets (const SampleData& d, float start, float end)
{
    const int total = d.numSamples();
    start = juce::jlimit (0.0f, 1.0f, start);
    end = juce::jlimit (start, 1.0f, end);
    const int a = (int) std::floor (start * (float) total), b = (int) std::floor (end * (float) total);
    const int len = b - a;
    const double sr = juce::jmax (8000.0, d.sampleRate);
    if (total <= 0 || len < (int) (2.0 * sr))
        return 0.0;

    const int hop = juce::jmax (16, (int) (sr * 0.003));          // 3 ms
    const int frame = hop * 4;
    const int channels = d.numChannels();
    const int frames = (len - frame) / hop + 1;
    if (frames < 200)
        return 0.0;

    // Energia (log) della differenza prima del segnale, poi flusso positivo (aumenti di energia)
    std::vector<float> e ((size_t) frames, 0.0f);
    std::vector<float> mono ((size_t) len, 0.0f);
    for (int ch = 0; ch < channels; ++ch)
    {
        const float* src = d.audio.getReadPointer (ch) + a;
        for (int i = 0; i < len; ++i)
            mono[(size_t) i] += src[i] / (float) channels;
    }
    for (int k = 0; k < frames; ++k)
    {
        double sum = 0.0;
        for (int i = k * hop; i < k * hop + frame; ++i)
        {
            const double df = (double) mono[(size_t) i] - (i > 0 ? (double) mono[(size_t) i - 1] : 0.0);
            sum += df * df;
        }
        e[(size_t) k] = (float) std::sqrt (sum / (double) frame);          // ampiezza (non log): i colpi forti pesano piu' di quelli deboli
    }
    std::vector<float> flux ((size_t) frames, 0.0f);
    double mean = 0.0;
    for (int k = 1; k < frames; ++k)
    {
        flux[(size_t) k] = std::max (0.0f, e[(size_t) k] - e[(size_t) k - 1]);
        mean += flux[(size_t) k];
    }
    mean /= (double) frames;
    double energy = 0.0;
    for (auto& f : flux)
    {
        f = std::max (0.0f, f - (float) mean);                      // senza la componente continua: contano i picchi
        energy += (double) f * f;
    }
    if (energy < 1.0e-9)
        return 0.0;

    // Autocorrelazione nell'intervallo 60..200 BPM
    const double frameSec = (double) hop / sr;
    const int lagMin = juce::jmax (2, (int) std::floor (60.0 / 200.0 / frameSec));
    const int lagMax = juce::jmin (frames / 2 - 1, (int) std::ceil (60.0 / 60.0 / frameSec));
    if (lagMax <= lagMin + 2)
        return 0.0;
    const int lagTop = juce::jmin (frames - 2, lagMax * 3 + 2);
    std::vector<double> acf ((size_t) lagTop + 2, 0.0);
    for (int L = lagMin - 1; L <= lagTop; ++L)
    {
        double s = 0.0;
        for (int k = 0; k + L < frames; ++k)
            s += (double) flux[(size_t) k] * (double) flux[(size_t) k + (size_t) L];
        acf[(size_t) L] = s / (double) (frames - L);
    }

    // Punteggio: il ritardo del battito piu' i suoi multipli (2x, 3x), pesato da una preferenza per i tempi vicini a 120 BPM
    std::vector<double> score ((size_t) lagMax + 2, 0.0);
    double best = 0.0;
    int bestLag = 0;
    for (int L = lagMin; L <= lagMax; ++L)
    {
        const double bpm = 60.0 / ((double) L * frameSec);
        double s = acf[(size_t) L];
        if (2 * L <= lagTop) s += 0.5 * acf[(size_t) (2 * L)];
        if (3 * L <= lagTop) s += 0.25 * acf[(size_t) (3 * L)];
        const double oct = std::log2 (bpm / 120.0);
        s *= std::exp (-0.5 * oct * oct / (0.7 * 0.7));
        score[(size_t) L] = s;
        if (s > best)
        {
            best = s;
            bestLag = L;
        }
    }
    if (bestLag == 0 || best <= 0.0)
        return 0.0;

    // Interpolazione parabolica attorno al massimo
    double lag = (double) bestLag;
    if (bestLag > lagMin && bestLag < lagMax)
    {
        const double y0 = score[(size_t) bestLag - 1], y1 = score[(size_t) bestLag], y2 = score[(size_t) bestLag + 1];
        const double den = y0 - 2.0 * y1 + y2;
        if (std::abs (den) > 1.0e-18)
            lag += juce::jlimit (-1.0, 1.0, 0.5 * (y0 - y2) / den);
    }
    double bpm = 60.0 / (lag * frameSec);
    while (bpm < 70.0) bpm *= 2.0;
    while (bpm > 170.0) bpm *= 0.5;
    return bpm;
}
}
