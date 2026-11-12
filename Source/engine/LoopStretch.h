#pragma once

#include "SampleData.h"

namespace sl
{
// Tempo di un loop dedotto dalla durata, supponendo che duri un numero intero di battute (1, 2, 4, 8, 16 o 32 movimenti da
// 4/4): sceglie tra le possibilita' quella con BPM piu' plausibile (70..170, la piu' vicina a 120). Ritorna 0 se il campione
// e' troppo breve (meno di mezzo secondo) per essere un loop.
double estimateLoopBpm (const SampleData& d, float start = 0.0f, float end = 1.0f);

// Copia del campione allungata/accorciata con Rubber Band (motore R3, offline): timeRatio = durata_nuova / durata_originale,
// pitchScale = 1 mantiene l'intonazione (Key Lock). Da chiamare fuori dal thread audio. nullptr in caso di errore.
SampleDataPtr stretchSample (const SampleData& d, double timeRatio, double pitchScale);
}
