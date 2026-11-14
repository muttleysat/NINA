# 9. Slice: tagliare un loop sulle celle

La scheda **Slice** (ultima in basso, con l'icona delle forbici) taglia un campione in **slice** e le distribuisce sulle celle: un loop di batteria diventa una cella per colpo. La sorgente può essere il campione di una **cella** oppure un **file audio esterno** (anche un brano intero, di cui scegli la zona).

La scheda ha tre righe in alto, sempre nello stesso ordine:

| Riga | Cosa contiene |
|------|---------------|
| **SOURCE** | **Cell / File** (la sorgente), **Load file...** (solo con File), il nome della sorgente con durata e formato, **Audition** e il suo volume |
| **CUT** | Come trovare i punti di taglio: **Detect** con **Sensitivity**, **Even**, **Clear**, **Chop** |
| **TEMPO** | **Source BPM** con i suoi strumenti: **Detect**, **/2**, **x2**, **Tap**, **Click test** |

Sotto, la forma d'onda con i marcatori, la striscia delle celle **CELLS** e la riga con **Distribute to cells**.

## Come funziona

1. In **Main** seleziona la cella con il loop (per esempio A10) e apri la scheda **Slice**. Il campione è quello della cella selezionata; la parte tagliata è quella tra **Start** ed **End** della cella.
2. Trova i punti di taglio:
   - **Detect** mette un marcatore su ogni **colpo** trovato. La manopola **Sensitivity** regola quanto è sensibile: più alta trova più colpi, anche deboli e ravvicinati. Se la cambi, il rilevamento si rifà da solo.
   - **Even: 1/2 … 1/32** taglia la regione in parti **uguali**.
   - **Clear** toglie tutti i marcatori (resta quello dell'inizio).
3. Rifinisci a mano sulla forma d'onda grande:
   - **Clic su una slice**: la senti (se **Audition** è acceso; accanto c'è il volume dell'anteprima).
   - **Trascina un marcatore** per spostarlo (la bandierina numerata in alto).
   - **Doppio clic** sulla forma d'onda per **aggiungere** un marcatore.
   - **Tasto destro** su un marcatore per **toglierlo**.
   - I pulsanti **− [ ] +** (in basso a destra) zoomano; con la **rotella** zoomi attorno al mouse, con **Maiusc+rotella** scorri. Le parentesi arancioni sono Start ed End della cella, che si cambiano nella scheda Main.
   - Passando il mouse su una slice si vede la sua **durata in ms**.
4. La striscia **CELLS** mostra le celle che verranno riempite: in arancione con il **numero della slice**, la cella originale con il contorno bianco (**original**), le celle già occupate in rosso con un triangolo e la scritta **occupied** (verrebbero sostituite). **Clicca una cella** per far partire la distribuzione da lì.
5. **Keep original in A10** (acceso): le slice vanno nelle celle **dopo** l'originale, che resta com'è. Spento: partono dalla cella stessa del loop.
6. **Distribute to cells** riempie le celle. Se alcune sono occupate chiede conferma.

La scritta in basso a sinistra riassume tutto: numero di slice, da quale a quale cella, quante verranno sovrascritte.

## Ascolto, stop e zoom

- **Play zone** (riga SOURCE) o la **barra spaziatrice** suona la zona dall'inizio alla fine con un cursore bianco; premi di nuovo (o **Esc**) per **fermare**. Nella scheda Slice la barra spaziatrice comanda quindi l'ascolto della zona e non il sequencer. Esc ferma anche il Chop e il Click test.
- Un clic su una slice suona solo quella slice (con Audition acceso).
- Con lo **zoom** la forma d'onda **segue il cursore** mentre suona (a pagine). Se sposti la vista a mano (rotella) il seguimento si ferma per qualche secondo. Lo zoom con i pulsanti **− [ ] +** si centra sul cursore se sta suonando; la rotella zooma **attorno al mouse** (Maiusc+rotella scorre).
- **Scrub**: **trascina** il mouse sulla forma d'onda per sentire il punto sotto il cursore (brevi ascolti da 140 ms mentre ti muovi). Un clic sul **righello del tempo** (la fascia dei numeri in alto) suona dal punto indicato fino alla fine della zona; trascinando sul righello fai lo scrub. Se stai ascoltando **tutta la zona** (Play zone), un clic sulla forma d'onda **salta a quel punto** e prosegue fino alla fine; anche lo scrub riprende da dove rilasci il mouse. Mentre la musica suona (o durante lo scrub) la slice sotto il mouse non si evidenzia.

## Sorgente: una cella o un file esterno

- **Cell** (predefinito): la sorgente è il campione della cella selezionata nella scheda Main. La zona è quella tra Start ed End della cella (si cambia in Main). Le slice restano nello **stesso file**, ognuna con il proprio Start/End.
- **File**: la sorgente è un file audio esterno. Per caricarlo:
  - **trascina** il file sulla scheda Slice (da Esplora risorse o dal browser a sinistra), oppure
  - premi **Load file...**, oppure
  - **doppio clic** su un file del browser mentre sei nella scheda Slice.

  La forma d'onda mostra **tutto il file**. Le due **parentesi arancioni** delimitano la **zona** da tagliare: trascinale (anche zoomando con **− [ ] +** e Ctrl+rotella). La parte fuori dalla zona è scurita. Detect, Even, Chop, Click test e le slice lavorano **solo dentro la zona**. Il primo marcatore segue sempre l'inizio della zona.
- Con **File**, **Distribute to cells** parte dalla **cella selezionata in Main** (cambia la partenza cliccando una cella nella striscia CELLS). Ogni slice viene **ritagliata in un file WAV a 24 bit** nella cartella `Slices` accanto al kit (oppure in `Documenti\NINA Slices` se il kit non è ancora salvato), in una sottocartella con il nome del file; se esiste già ne crea una nuova (`... 2`), così non sovrascrive mai una distribuzione precedente. Le celle sono quindi campioni normali e il kit non dipende dal brano originale.
- **Sync slices to tempo** (solo con File): accende **Sync to tempo** sulle nuove celle. Il Source BPM viene salvato in ognuna.
- Cambiando tra Cell e File, i marcatori e la zona di ciascuna sorgente restano come li avevi lasciati. Il file esterno e la sua zona non vengono salvati nel kit: si perdono chiudendo l'app (le celle create restano).

## Tempo della sorgente

Il Sync delle slice stira ogni cella dal **tempo originale** al BPM del progetto, quindi serve il BPM giusto della sorgente. Si imposta una volta sola per la zona scelta: tutte le slice lo ereditano (non si analizza mai ogni slice da sola).

- **Source BPM**: il valore (trascina, scrivi o usa − e +). Con una cella è lo stesso **Loop BPM** del pannello della cella.
- **Detect**: stima il tempo dalla **regolarità dei colpi** nella zona (non dalla durata), cercando tra 70 e 170 BPM, con preferenza per i tempi vicini a 120. Serve almeno circa 2 secondi di ritmo; su brani senza colpi netti può sbagliare.
- **/2** e **x2**: correggono un tempo trovato doppio o dimezzato.
- **Tap**: batti il tempo ascoltando (Audition acceso, clic su una slice o Chop); dopo 2 colpi calcola la media. Se aspetti più di 2 secondi ricomincia.
- **Click test**: suona la zona con un **click a ogni battito** al Source BPM, a partire dal **primo marcatore**, con un accento ogni 4 battiti. Se il click resta sul battito dall'inizio alla fine il tempo è giusto; se scivola, ritocca il BPM o scegli una zona più corta (nei brani dal vivo il tempo non è fisso: conviene lavorare per zone brevi). Un cursore bianco scorre sulla forma d'onda; si ferma da solo o con **Stop test**.

Consiglio per un brano: scegli la zona **a battute intere** partendo da un colpo forte, premi **Detect**, controlla con **Click test**, poi taglia (Detect, Even, Chop o a mano) e **Distribute**.

## Chop: tagliare ascoltando (stile MPC)

Il pulsante **Chop** (riga CUT) fa suonare la regione dall'inizio alla fine e ti lascia mettere i tagli **a tempo**, mentre ascolti:

1. Premi **Chop**: i marcatori esistenti vengono tolti (resta solo l'inizio), il loop parte e un **cursore bianco** scorre sulla forma d'onda.
2. Ogni volta che premi **M** sulla tastiera del computer, o **una nota qualsiasi** della tastiera MIDI, viene messo un marcatore nel punto in cui si trova il cursore. Durante il Chop le note MIDI non suonano le celle: servono solo per tagliare.
3. Il Chop finisce da solo alla fine della regione, oppure premendo di nuovo il pulsante (**Stop chop**). Cambiando scheda o cella si ferma.
4. Rifinisci i marcatori a mano (trascina, doppio clic, tasto destro) e premi **Distribute to cells**.

Il suono esce con il ritardo della scheda audio: se tagli sempre un po' in ritardo, anticipa leggermente il tasto o sposta i marcatori dopo.

## Cosa succede alle celle

- Ogni cella riceve lo **stesso file** del loop (nessuna copia dei dati: non occupa memoria in più) con il proprio **Start/End** dell'inizio e della fine della slice. Il nome è quello del loop più un numero (`amen 01`, `amen 02`…).
- Con una **cella** come sorgente le slice **ereditano il tempo del loop**: Loop BPM (stimato se la cella originale non lo aveva), Sync e Key lock. Se accendi **Sync to tempo** sulle slice (meglio selezionandole tutte: le opzioni si applicano a tutte le celle selezionate) ognuna segue il BPM del progetto restando al suo posto nel loop; l'elaborazione è una sola, condivisa. **Non premere Detect** su una singola slice: stimerebbe il tempo dalla sua durata (un colpo solo) e sbaglierebbe; lascia il BPM del loop.
- Le celle sono celle normali: ognuna ha il suo tasto MIDI (dal **C1** in poi, come sempre), il suo inviluppo, gli effetti, e si possono aggiungere al sequencer. Per rifinire una slice usa Start/End nella scheda Main.
- I marcatori restano salvati con il kit: riaprendo la scheda Slice sulla stessa cella ritrovi i tagli.
- Se le slice sono più delle celle libere in fondo alla griglia (48 in tutto), vengono riempite solo quelle disponibili e la scritta in basso lo avvisa.

## Limiti

- Il rilevamento dei colpi funziona bene su percussioni con attacchi netti (batterie, loop ritmici). Su materiale tenuto o molto denso (pad, musica completa) trova molti punti o pochi: abbassa o alza **Sensitivity** e correggi a mano.
- I tagli cadono sul punto in cui sale il suono; non c'è ancora l'allineamento alla griglia del tempo.
- La scheda lavora su una cella alla volta. Se sbagli una distribuzione, annullala con **Ctrl+Z**.
