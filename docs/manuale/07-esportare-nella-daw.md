# 7. Esportare nella DAW

Dalla scheda **Seq** puoi portare il pattern in Cubase in tre modi. Tutti usano **il tempo e la lunghezza correnti**: il BPM mostrato in alto e il menu **1/2/4/8 bars**.

## Cosa esporta ogni opzione

| Opzione | Contenuto | Effetti inclusi? |
|---------|-----------|------------------|
| **MIDI** | Le note del pattern come clip MIDI. Ogni cella suona la sua nota (**Key low** della cella), con velocity, ratchet, swing e probabilità già "risolti" in note fisse | **No** (il MIDI non può contenerli) |
| **Audio** (Bounce WAV) | L'intera sequenza in **un solo file** WAV a 24 bit | **Sì**: effetti delle celle, lock degli step, delay sincronizzato, code, effetti master |
| **Righe** (Export lanes) | **Un file WAV per ogni riga** in una cartella, tutti **della stessa lunghezza** | Sì: gli effetti di ogni cella (con delay sincronizzato e code). **Senza** gli effetti master |

I file delle righe sommati danno esattamente l'audio intero (senza gli effetti master). Usali per **rimixare** ogni suono sulla sua traccia.

Reverse e lock degli effetti non esistono nel MIDI: per averli nella DAW usa l'audio.

## Il metodo più veloce: trascinare nella DAW

Sopra le righe, in **DRAG TO DAW**, ci sono tre maniglie:

- **Drag MIDI** – trascinala su una traccia MIDI o strumento di Cubase: crea la clip.
- **Drag audio** – trascinala su una traccia audio: l'intera sequenza con gli effetti.
- **Drag lanes** – trascinala nel progetto: arrivano tutti i file, uno per riga.

I file vengono generati **nel momento in cui inizi il trascinamento**, con il tempo e le battute del momento (l'audio può impiegare un istante). Passano da una cartella temporanea del sistema.

> **Stato della verifica:** il trascinamento verso Cubase non è ancora stato provato. Nello standalone usa la funzione di trascinamento file del sistema operativo; nel plugin dentro Cubase dipende da come la DAW accetta il trascinamento dai plugin. Se non parte, usa i pulsanti per salvare i file.

## Salvare i file (pulsanti)

- **Export MIDI** – apre la finestra di salvataggio per il file `.mid`.
- **Bounce WAV** – salva il `.wav` dell'intera sequenza.
- **Export lanes** – chiede una cartella e vi crea una sotto-cartella con un file per riga.

Il nome proposto contiene kit, pattern, BPM e battute (per esempio `MioKit_Pattern 1_120bpm_2bar.wav`). Importandoli in Cubase ricordati di impostare il progetto allo stesso tempo.

## Un flusso consigliato in Cubase

1. Imposta il tempo del progetto e prepara il pattern in Sample Lab (stesso BPM).
2. Se vuoi poter **modificare le note**: *Drag MIDI* su una traccia che suona Sample Lab (o un altro strumento).
3. Se vuoi **il suono finito**: *Drag audio* su una traccia audio.
4. Se vuoi **rimixare** ogni suono: *Drag lanes*, poi bilancia le tracce.
5. Se cambi il pattern o il tempo, **riesporta**: l'export è un'istantanea, non un collegamento.

## Limiti

- L'export non include l'**automazione** della DAW, ma i valori correnti dei controlli.
- Il delay sincronizzato usa il BPM **del momento dell'export**.
- Un'esportazione **non fa il tempo reale**: è più veloce, e non dipende dalla DAW.
- Un file audio dell'intera sequenza è tagliato dopo la fine delle code per non avere silenzio inutile; i file delle righe **non** sono tagliati, per restare allineati.
