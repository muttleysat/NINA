#pragma once

#include "SampleData.h"
#include <vector>

namespace sl
{
// Rilevamento dei colpi (onset) di un loop e divisioni uguali. Tutte le posizioni sono frazioni 0..1 della lunghezza del campione.
// Da chiamare fuori dal thread audio.

// Marcatori di slice (inizio di ogni slice, in ordine) nella regione [start, end]: il primo e' sempre 'start', poi un marcatore
// per ogni colpo trovato. sensitivity 0..1: piu' alta = piu' colpi (anche piu' deboli e ravvicinati).
std::vector<float> detectSlices (const SampleData& d, float sensitivity, float start = 0.0f, float end = 1.0f);

// Tempo (BPM) stimato dalla regolarita' dei colpi nella regione [start, end]: autocorrelazione del flusso di energia, con una
// preferenza per i tempi vicini a 120 BPM (70-170 BPM). Non conta sulla durata: funziona anche se i bordi non sono a battuta esatta.
// Ritorna 0 se la regione e' troppo corta (< 2 s) o senza ritmo riconoscibile.
double estimateBpmFromOnsets (const SampleData& d, float start = 0.0f, float end = 1.0f);

// 'count' slice di uguale durata nella regione [start, end] (count marcatori, il primo a 'start').
std::vector<float> evenSlices (float start, float end, int count);
}
