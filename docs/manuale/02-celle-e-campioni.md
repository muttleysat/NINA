# 2. Celle e campioni

## La griglia

La griglia ha **48 celle** (righe A–D, colonne 1–12). Ogni cella contiene un campione e tutte le sue impostazioni. Ogni pad mostra l'ID (A1…D12), il nome del campione, una mini forma d'onda e, se la cella è nel sequencer, un **pallino arancione** in basso a destra. Un pad si illumina quando suona.

### Caricare un campione

| Modo | Come |
|------|------|
| Dal browser (trascinamento) | Trascina il file dal browser a sinistra su un pad |
| Da Esplora risorse | Trascina uno o più file direttamente su un pad. Con più file, riempiono i pad successivi in ordine |
| Doppio clic | Doppio clic su un file nel browser: va nel pad selezionato |
| Menu del pad | Tasto destro sul pad → **Load sample...** |

Formati letti: WAV, AIFF, FLAC, OGG, MP3.

### Menu del pad (tasto destro)

- **Add to sequencer / Remove from sequencer** – aggiunge o toglie la cella dal sequencer.
- **Load sample...** – carica un file.
- **Rename...** – rinomina la cella.
- **Clear cell** – svuota la cella.

### Selezionare e ascoltare

Un clic sul pad lo **seleziona** e lo fa suonare (finché tieni premuto, per le modalità Gate e Loop). Il pad selezionato ha il bordo arancione, e il pannello a destra, la forma d'onda e i moduli in basso mostrano **le sue** impostazioni.

## Il browser dei file

- Il menu in alto sceglie dove cercare: **Home**, **Desktop**, **Documents**, **Music** e tutti i dischi del computer.
- Il campo **Search samples...** filtra i file per nome nelle cartelle aperte.
- **Preview** (in basso) attiva l'anteprima: selezionando un file lo senti. Il cursore accanto regola il volume dell'anteprima.

## Il pannello della cella (a destra)

| Elemento | Cosa fa |
|----------|---------|
| Titolo con **< >** | Mostra la cella selezionata; le frecce passano alla cella precedente/successiva |
| Scheda **Sample** / **Load** | Il campione assegnato; Load apre la scelta di un file |
| **Trigger mode** | **One-shot**: suona fino in fondo e ignora il rilascio del tasto. **Gate**: suona finché tieni premuto il tasto, poi si spegne con il Release. **Loop**: si ripete finché tieni premuto |
| **Reverse** | Suona il campione al contrario |
| **Normalize** | Imposta il Level della cella in modo che il picco più alto del campione arrivi a −1 dBFS (il file audio non cambia) |
| **Loop / tempo** | **Sync to tempo**, **Key lock**, **Loop BPM** e **Detect** (vedi sotto) |
| **MIDI mapping** | **Root note**: la nota a cui il campione suona alla sua intonazione originale. **Key low / Key high**: l'intervallo di tasti che fa suonare la cella |
| **Sample info** | Formato e durata |

### Mapping MIDI

Di default il pad **A1 suona C1** (nota MIDI 36) e ogni pad successivo la nota dopo, riga per riga (A1…A12, B1…). Con le 48 celle si coprono le note da C1 a B4.

- Per cambiare la nota di una cella, cambia **Key low** e **Key high** (uguali = una nota sola).
- Se **Key low** e **Key high** sono diversi, la cella suona su più tasti e il campione viene **trasposto** rispetto a **Root note**: è il modo per suonare un campione in modo melodico.
- Più celle possono rispondere alla stessa nota: suonano insieme (layer).

**Cosa si vede sui pad:**
- In basso a sinistra di ogni pad compare il **tasto** della cella (per esempio `C1`) oppure l'**intervallo** (`C1 - C2`).
- Un pad **vuoto** la cui nota rientra nell'intervallo di un'altra cella si vede con **strisce** del colore di quella cella e la scritta `← A1`: significa che premendo quel tasto suona la cella A1, e che caricando un campione in quel pad le due celle suonerebbero insieme.
- Un triangolo arancione **!** in alto a destra segnala che due celle **con campione** condividono dei tasti. Passando il mouse sul pad, il tooltip elenca quali.

Nota: le note dei tasti MIDI valgono anche per il sequencer, ma ogni riga suona la sua cella solo con la nota **Key low**; gli altri tasti dell'intervallo servono per suonare dalla tastiera.

## Loop e tempo (Sync, Key lock)

Per far seguire a un loop (per esempio un breakbeat o un giro di batteria) il **BPM del progetto**:

1. **Loop BPM** è il tempo originale del loop. Caricando un campione di almeno mezzo secondo viene **stimato dalla durata**, supponendo che duri un numero intero di battute (1, 2, 4, 8, 16 o 32 movimenti) e scegliendo il tempo più plausibile (70–170 BPM, il più vicino a 120). Il valore è sempre modificabile: se la stima è sbagliata (per esempio un loop da 8 movimenti letto come 16) correggilo a mano o cambia **Start/End** e premi **Detect**. Con il valore **--** (zero) il tempo è sconosciuto e la sincronizzazione non fa nulla.
2. Accendi **Sync to tempo**: il campione viene **allungato o accorciato** per il BPM del progetto (quello della DAW nel plugin, quello del riquadro **BPM** in alto nello standalone). Il file originale non cambia: la copia elaborata si prepara in **background** con Rubber Band (alta qualità) e si sente dopo un istante. Se il BPM cambia, si rielabora da solo dopo una piccola pausa.
3. **Key lock** acceso: il tempo cambia **senza cambiare l'intonazione**. Spento: cambiano insieme tempo e altezza, come su un giradischi.

Pitch e Tune continuano a funzionare sopra la sincronizzazione. Start ed End restano nelle stesse posizioni relative. L'esportazione (MIDI, audio, righe) usa il campione sincronizzato al BPM del momento.

> **Nota:** la stima dalla durata funziona bene per loop tagliati a battute esatte. Un campione con silenzio in coda o non tagliato a misura dà un tempo sbagliato: in quel caso imposta Start/End sul loop esatto e premi **Detect**, oppure scrivi il BPM.

## La forma d'onda (sotto la griglia)

Mostra il campione della cella selezionata.

- Trascina il marcatore sinistro per impostare **Start** e il destro per **End**: il campione suona solo tra i due. Le zone escluse sono scurite.
- A destra leggi **Start**, **End** e **Length** in millisecondi.
- La modifica è **non distruttiva**: il file audio non viene mai cambiato.

## I moduli della scheda Main

### AMP – inviluppo e livello

| Controllo | Valore | Effetto |
|-----------|--------|---------|
| Attack | 0–2 s | Tempo di entrata |
| Hold | 0–2 s | Tempo a pieno livello |
| Decay | 0–5 s | Discesa fino al Sustain |
| Sustain | 0–1 | Livello mantenuto |
| Release | 0–5 s | Tempo di uscita al rilascio |
| Level | −60…+12 dB | Volume della cella |
| Pan | −1…+1 | Posizione stereo |

Sotto le manopole il **grafico dell'inviluppo** mostra la forma che stai disegnando.

### PITCH

- **Tune:** intonazione fine, ±100 cent.
- **Pitch:** trasposizione, ±24 semitoni.

La modifica si sente subito, anche sulle note che stanno già suonando.

### MASTER

- **Volume:** volume generale del kit, da −60 a +12 dB.
- Il **meter** a destra mostra il livello di uscita.

## Normalizzare per avere lo stesso volume

I campioni spesso hanno volumi diversi. Il pulsante **Normalize** del pannello della cella porta il picco di quel campione a −1 dBFS regolando il suo **Level** (che puoi sempre ritoccare). Per farlo su tutto il kit: menu **Kit → Normalize all cells**.

> **Nota:** la normalizzazione allinea i *picchi*, non il volume percepito. Una hi-hat e una cassa con lo stesso picco possono suonare a volumi diversi: ritocca il Level a orecchio.

## Manopole e valori (valido per tutta l'app)

- **Trascina** una manopola per cambiarla; **doppio clic** la riporta al valore predefinito (dove previsto).
- **Tasto destro** su un controllo apre il menu del **MIDI learn** (capitolo 6).
