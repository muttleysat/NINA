# 3. Effetti

Apri la scheda **FX** in basso. In alto a sinistra scegli **a cosa** applicare gli effetti:

- **Cell A1 …** (il nome della cella selezionata): gli effetti di quella cella.
- **Master**: gli effetti sull'intero kit, dopo tutte le celle.

Ogni effetto è una **scheda** con un interruttore in alto a destra (acceso/spento) e le sue manopole. Le schede sono disposte su **due righe**, nell'ordine della catena (la scheda FX usa tutta la larghezza: in questa scheda il browser, il pannello della cella e la forma d'onda sono nascosti, e la griglia resta in alto per scegliere la cella). Le manopole si possono regolare anche quando l'effetto è spento: i valori restano memorizzati.

## Gli effetti

| Effetto | Controlli | Cosa fa |
|---------|-----------|---------|
| **Saturation** | Drive (0–36 dB), Mix | Distorsione dolce (saturazione) |
| **LoFi** | Bits (1–16), Rate (1–40×), Mix | Riduce i bit e la frequenza di campionamento: suono "sporco" digitale |
| **Filter** | Tipo (Lowpass/Highpass/Bandpass), Cutoff (20 Hz–20 kHz), Reso | Filtro risonante |
| **EQ** | Low (±15 dB), Mid Hz (100 Hz–8 kHz), Mid dB (±15 dB), Mid Q (0.3–8), High (±15 dB) | Equalizzatore a tre bande, con la **curva di risposta** disegnata sopra le manopole (20 Hz–20 kHz, ±18 dB; si aggiorna mentre giri le manopole, anche con l'automazione): **Low** è uno *shelf* (alza o abbassa tutto sotto i 120 Hz), **Mid** è una *campana* (scegli la frequenza, di quanto alzarla o abbassarla e quanto è stretta con **Q**), **High** è uno *shelf* sopra i 6 kHz |
| **Compressor** | Thresh, Ratio, Attack, Release, Makeup | Compressore |
| **Stutter** | Divisione (1/1…1/64), Decay, Mix | *Beat repeat* **sincronizzato al tempo**. Acceso, funziona così: **a ogni nuovo colpo della cella** registra la porzione di suono che parte da lì, lunga quanto la divisione scelta (1/16 a 120 BPM = 125 ms); intanto il colpo si sente normalmente, poi la porzione si **ripete a ciclo** finché non arriva un altro colpo o lo spegni. Se lo accendi mentre un **loop o un break** sta già suonando, ripete subito l'ultima porzione di ciò che si sentiva. **Decay** abbassa ogni ripetizione rispetto alla precedente (0 = tutte uguali), **Mix** dosa ripetizione e suono normale (1 = solo la ripetizione). Sul **Master** vale per tutto il kit (i nuovi colpi di qualunque cella lo riarmano) |
| **Chorus** | Rate (0,05–5 Hz), Depth, Mix | Raddoppia il suono con una copia leggermente ritardata (circa 15 ms) la cui intonazione ondeggia: effetto «largo», più corposo e stereo. **Rate** è la velocità dell'oscillazione, **Depth** quanto ondeggia, **Mix** la quantità della copia |
| **Flanger** | Rate, Depth, Fdbk (feedback, 0–0,9), Mix | Una copia con un ritardo brevissimo (0,2–2,8 ms) che si muove: crea il caratteristico «sibilo» a pettine che sale e scende. **Fdbk** rimanda l'uscita dentro l'effetto e lo rende più metallico e risonante |
| **Phaser** | Rate, Depth, Fdbk, Mix | Quattro filtri passa-tutto che si spostano tra 200 Hz e 4 kHz sommati al suono originale: un effetto «vorticoso» più morbido del flanger. **Fdbk** aumenta la profondità dei vuoti |
| **Delay** | Sync, valore musicale, Time, Feedback, Mix | Eco |
| **Reverb** | Size, Damp, Mix | Riverbero |

## Come lavorano gli effetti di una cella

Ordine della catena: Saturation > LoFi > Filter > **EQ** > Compressor > **Chorus > Flanger > Phaser** > Delay > Reverb > **Stutter** (lo Stutter è l'ultimo: ripete ciò che senti dalla cella, code incluse). Lo Stutter ripete al massimo 0,75 secondi: con tempi molto lenti le divisioni più lunghe vengono accorciate.

Capire questo evita sorprese, soprattutto nel sequencer.

1. **Saturation, LoFi e Filter** lavorano **su ogni singola nota**: ogni colpo ha il suo filtro. Due note ravvicinate non si influenzano.
2. **EQ**, **Compressor**, **Chorus**, **Flanger** e **Phaser** lavorano sulla **somma** di tutte le note della cella (nell'ordine della catena). Le mandate al delay e al riverbero partono prima dell'EQ. L'EQ non è bloccabile dagli step del sequencer.
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
