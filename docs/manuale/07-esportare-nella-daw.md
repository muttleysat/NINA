# 7. Esportare nella DAW

Dalla scheda **Seq** puoi portare il pattern in Cubase in tre modi. Tutti usano **il tempo e la lunghezza correnti**: il BPM mostrato in alto e il menu **1/2/4/8 bars**.

## Il tempo dell'esportazione audio (plugin)

L'audio (Bounce WAV, Export lanes, Drag audio, Drag lanes) viene creato **al tempo del progetto**. Nel plugin NINA conosce il tempo della DAW solo **dopo che la DAW glielo ha mandato**, cioè di solito quando il trasporto è partito almeno una volta (poi se lo ricorda, anche a DAW ferma: il riquadro BPM in alto diventa non modificabile e indica "last received"). Per evitare file a velocità sbagliata (per esempio a 120 BPM predefiniti quando il tuo progetto è a 90), **se NINA non ha ancora ricevuto il tempo l'esportazione audio si ferma con un avviso**.

- Soluzione 1: premi **Play** nella DAW (anche per un secondo) e riprova.
- Soluzione 2: scrivi il tempo del progetto nel riquadro **BPM** (doppio clic) e riprova.
- La finestra di conferma dopo **Bounce WAV** indica **a quanti BPM e a quanti Hz** è stato creato il file: controlla che coincidano con il progetto.
- Se il tempo che NINA ha ricevuto **non è quello del tuo brano**, puoi scegliere a mano il **tempo di esportazione**: nel plugin fai doppio clic sul riquadro **BPM** in alto e scrivi il tempo (per esempio 126). Il valore compare in arancione con un asterisco (`126.0 BPM *`) e vale per **tutte le esportazioni** (audio e MIDI) finché non lo rimetti a **0**, che torna al tempo automatico della DAW. Il tempo della riproduzione resta quello della DAW.
- **«Segui tempo» / Musical Mode:** ogni file WAV audio esportato contiene i **metadati ACID** con il **tempo** (quello dell'esportazione) e il **numero di quarti**, e il flag di loop adattabile. Cubase li legge all'importazione: con l'opzione **Segui tempo** (Musical Mode) accesa il file è riconosciuto al tempo giusto, e se poi **cambi il tempo del progetto** l'audio si adatta correttamente. Senza questi dati Cubase **stima** il tempo dalla durata e può sbagliare di molto. Se un file già esportato con una versione precedente è riconosciuto male, riesportalo, oppure in Cubase imposta il tempo a mano nel **Pool** (campo *Tempo* del file).
- **Frequenza di campionamento dei file:** il menu accanto a **Tail** (all'inizio mostra **Auto** seguito dalla frequenza che la DAW ha comunicato al plugin, per esempio *Auto 48k*) (nelle schede Seq e Song) sceglie la frequenza dell'audio esportato: **auto** (quella del progetto/della DAW, come la comunica la DAW al plugin), **44.1**, **48**, **88.2** o **96 kHz**. L'audio viene **renderizzato direttamente** a quella frequenza, senza conversioni successive, e la durata resta identica. Se un file importato in Cubase suona a velocità sbagliata, scegli la frequenza del tuo progetto (la trovi in Cubase in *Progetto > Impostazioni progetto*) e riesporta. La finestra dopo **Bounce WAV** e il diario indicano la frequenza usata.
- Ogni esportazione audio scrive una riga nel diario `ui.log` (cartella `NINA` dentro `AppData\Roaming`) che comincia con `EXPORT`: contiene il **tempo usato**, il tempo corrente, quello ricevuto dalla DAW, il tempo scelto a mano e la **frequenza di campionamento**. Serve per capire da dove viene un file a velocità sbagliata.
- Se il progetto cambia tempo, NINA usa quello ricevuto ultimamente (un solo tempo per tutto il file). L'esportazione **MIDI** non ha questo problema: le note seguono il tempo del progetto in qualunque caso.

## Cosa esporta ogni opzione

| Opzione | Contenuto | Effetti inclusi? |
|---------|-----------|------------------|
| **MIDI** | Le note del pattern come clip MIDI. Ogni cella suona la sua nota (**Key low** della cella), con velocity, ratchet, swing e probabilità già "risolti" in note fisse | **No** (il MIDI non può contenerli) |
| **Audio** (Bounce WAV) | L'intera sequenza in **un solo file** WAV a 24 bit | **Sì**: effetti delle celle, lock degli step, delay sincronizzato, code (vedi Tail), effetti master |
| **Righe** (Export lanes) | **Un file WAV per ogni riga** in una cartella, tutti **della stessa lunghezza** | Sì: gli effetti di ogni cella (con delay sincronizzato e code). **Senza** gli effetti master |

I file delle righe sommati danno esattamente l'audio intero (senza gli effetti master). Usali per **rimixare** ogni suono sulla sua traccia.

Reverse e lock degli effetti non esistono nel MIDI: per averli nella DAW usa l'audio.

## Suonare e registrare a mano con il sequencer (plugin)

Quando il trasporto della DAW suona, il sequencer parte con lui. Se registri a mano una linea di batteria con il pulsante di registrazione di Cubase, il sequencer andrebbe in mezzo a ciò che suoni. Per questo, nella riga sotto Swing/Reset Seed della scheda Seq, c'è il menu **DAW** (si vede solo nel plugin):

| Voce | Cosa fa |
|------|---------|
| **follow transport** | Il sequencer suona sempre quando la DAW suona (anche in registrazione) |
| **silent while recording** (predefinito) | Suona con la DAW, ma **sta zitto mentre la DAW registra**: senti e registri solo ciò che suoni tu |
| **ignore transport** | Ignora il trasporto della DAW: parte solo con il pulsante **Play** del plugin (il tempo resta quello della DAW) |

La scelta si salva nel progetto. Il sequencer non registra nulla da solo: le note che suoni tu arrivano a Cubase come normale MIDI. Con **follow** puoi invece sentire il sequencer come base mentre registri.

## Le code di delay e riverbero: Tail

Accanto al menu delle battute c'è il menu **Tail**, che decide cosa succede alle code degli effetti nei file **audio** (Bounce WAV, Export lanes, Drag audio, Drag lanes):

| Tail | Lunghezza del file | Cosa fa |
|------|--------------------|---------|
| **cut** (predefinito) | **Esattamente** le battute scelte | La coda oltre la fine viene tagliata: il file è quantizzato e si allinea alla griglia della DAW |
| **loop** | Esattamente le battute scelte | La coda della fine **rientra all'inizio**: la clip in loop suona senza stacchi (come se il pattern girasse) |
| **keep** | Battute + tutta la coda | Il file è più lungo del pattern; si ferma quando la coda finisce |

Per un loop in Cubase usa **cut** o **loop** (meglio loop se l'eco deve continuare tra un giro e l'altro). Con **loop** il pattern viene suonato due volte e si tiene la seconda: eco molto lunghe (oltre una battuta) possono essere incomplete all'inizio.

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

1. Imposta il tempo del progetto e prepara il pattern in NINA (stesso BPM).
2. Se vuoi poter **modificare le note**: *Drag MIDI* su una traccia che suona NINA (o un altro strumento).
3. Se vuoi **il suono finito**: *Drag audio* su una traccia audio.
4. Se vuoi **rimixare** ogni suono: *Drag lanes*, poi bilancia le tracce.
5. Se cambi il pattern o il tempo, **riesporta**: l'export è un'istantanea, non un collegamento.

## Uscite separate (plugin)

Il plugin ha **16 uscite stereo**: **Main** (il mix) e **Out 2 … Out 16**. In **Cubase** le uscite aggiuntive sono spente di default: aprili dal rack dei VSTi (le uscite dello strumento) e attiva quelle che ti servono, una per traccia.

1. Seleziona una cella e nel pannello a destra scegli **Output** (Main, Out 2…).
2. In alternativa usa il menu **Kit → Outputs: one cell per output** per mandare le prime 15 celle con campione su Out 2…Out 16 (le altre restano su Main), oppure **all cells to Main** per annullare.
3. Se un'uscita è spenta nella DAW, la cella **suona nel Main** (non sparisce).

Le uscite singole sono **prima** degli effetti master e del volume master (restano invece gli effetti della cella). Anche il sequencer le usa: le righe suonano sulla uscita della loro cella. Le esportazioni (MIDI, audio, righe) non cambiano: l'audio esportato contiene tutto nel mix, come prima.

> **Stato della verifica:** il routing è provato da test automatici sul motore; non ancora verificato in Cubase. Nello standalone c'è solo il Main (le uscite multiple valgono nel plugin).

## Limiti

- L'export non include l'**automazione** della DAW, ma i valori correnti dei controlli.
- Il delay sincronizzato usa il BPM **del momento dell'export**.
- Un'esportazione **non fa il tempo reale**: è più veloce, e non dipende dalla DAW.
- Con **Tail: keep** il file dell'intera sequenza è tagliato dopo la fine della coda per non avere silenzio inutile; i file delle righe restano della stessa lunghezza tra loro, in ogni modalità.
