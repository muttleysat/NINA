# 3. Effetti

Apri la scheda **FX** in basso. In alto a sinistra scegli **a cosa** applicare gli effetti:

- **Cell A1 …** (il nome della cella selezionata): gli effetti di quella cella.
- **Master**: gli effetti sull'intero kit, dopo tutte le celle.

Ogni effetto è una **scheda** con un interruttore in alto a destra (acceso/spento) e le sue manopole. Le manopole si possono regolare anche quando l'effetto è spento: i valori restano memorizzati.

## Gli effetti

| Effetto | Controlli | Cosa fa |
|---------|-----------|---------|
| **Saturation** | Drive (0–36 dB), Mix | Distorsione dolce (saturazione) |
| **LoFi** | Bits (1–16), Rate (1–40×), Mix | Riduce i bit e la frequenza di campionamento: suono "sporco" digitale |
| **Filter** | Tipo (Lowpass/Highpass/Bandpass), Cutoff (20 Hz–20 kHz), Reso | Filtro risonante |
| **Compressor** | Thresh, Ratio, Attack, Release, Makeup | Compressore |
| **Delay** | Sync, valore musicale, Time, Feedback, Mix | Eco |
| **Reverb** | Size, Damp, Mix | Riverbero |

## Come lavorano gli effetti di una cella

Capire questo evita sorprese, soprattutto nel sequencer.

1. **Saturation, LoFi e Filter** lavorano **su ogni singola nota**: ogni colpo ha il suo filtro. Due note ravvicinate non si influenzano.
2. **Compressor** lavora sulla **somma** di tutte le note della cella.
3. **Delay e Reverb** sono **mandate** (send): ogni nota manda una quantità del suo suono all'effetto, e l'effetto aggiunge l'eco o il riverbero. Il manopola **Mix** è la quantità mandata di default. L'eco di una nota **continua a risuonare** dopo la fine della nota (fino a circa 4 secondi dall'ultima mandata).

Se un effetto è spento sulla cella, quella cella non lo usa, **tranne** gli step del sequencer che lo accendono con un *lock* (capitolo 4).

## Delay sincronizzato al tempo

Nella scheda Delay:

- **Sync** acceso: il tempo dell'eco segue il **BPM** (della DAW, oppure quello interno dello standalone) e lo scegli come valore musicale dal menu: da **1/1** a **1/64**, anche puntati (**1/8.**) e terzine (**1/8T**).
- **Sync** spento: il tempo si imposta in millisecondi con la manopola **Time** (fino a 2000 ms).

Cambiando il tempo del progetto, l'eco si adegua con una transizione morbida (effetto "nastro").

> Il BPM dello standalone si scrive nel riquadro in alto (doppio clic). Nel plugin viene dalla DAW e non è modificabile.

## Effetti master

Con **Master** selezionato gli stessi effetti agiscono sul mix finale: Saturation, LoFi e Filter sul segnale complessivo, poi Compressor; **Delay e Reverb** mandano il mix (con **Mix** come quantità) e aggiungono eco e riverbero.

## Consigli

- Per un filtro che lavora solo su alcuni colpi (per esempio un cutoff che si chiude su certi step), spegni il Filter sulla cella e usa i *lock* degli step nel sequencer.
- Per un'eco su alcuni colpi soltanto: lascia il Delay **spento** sulla cella, imposta Time/Feedback una volta (anche dal pannello dello step) e nello step sposta **Send**.
- Se un suono diventa troppo forte con la saturazione, riduci il **Level** della cella o usa **Makeup** del compressore con cautela.
