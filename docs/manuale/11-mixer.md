# 11. Mixer

La scheda **Mixer** è una console come quella di Cubase: ogni cella con un campione ha il suo **canale**, e a destra c'è il **master**. Serve per bilanciare il kit a colpo d'occhio, ascoltare una cella da sola (Solo), zittirne un'altra (Mute) e vedere chi suona più forte con i **meter** digitali.

Tutti i controlli sono gli stessi parametri delle altre schede (Level, Pan, Mix del delay e del riverbero...): muovendo un fader qui si muove anche la manopola Level della cella nella scheda Main, e viceversa. Sono tutti automatizzabili dalla DAW e hanno il **MIDI learn** (tasto destro sul controllo).

## Il canale di una cella

Dall'alto in basso:

| Elemento | Cosa fa |
|----------|---------|
| **Uscita** (menu, di solito *Main*) | L'uscita del plugin su cui suona la cella: Main (il mix) oppure Out 2...Out 16 per mandarla su una traccia separata della DAW (come nel pannello della cella) |
| **FX: S L F C D R** | Gli effetti della cella: **S**aturation, **L**oFi, **F**ilter, **C**ompressor, **D**elay, **R**everb. Un clic li accende e li spegne (acceso = arancione). I parametri di ogni effetto restano nella scheda FX |
| **Dly / Rev** | Le due mandate: quanto segnale della cella va al delay e al riverbero. Alzandole con il mouse, l'effetto si accende da solo. Se l'effetto è spento, la manopola è attenuata (la mandata non si sente) |
| **PAN** | La posizione stereo, da L100 a R100. Doppio clic: al centro |
| **M / S** | **Mute** (la cella non suona) e **Solo** (suonano solo le celle in Solo). Si possono avere più celle in Solo. Si accendono in arancione |
| **Fader** + scala in dB | Il volume della cella (Level), da −60 a +12 dB. Doppio clic: 0 dB. La rotella del mouse lo muove di poco; Maiusc rallenta il trascinamento |
| **Meter** | Due barre (sinistra e destra) con segmenti verdi, gialli e rossi, scala −60…+6 dB. La linea bianca è il **picco**, che resta per circa un secondo. In cima a ogni barra c'è la spia di **clip**: si accende in rosso oltre 0 dB e resta accesa finché non clicchi sul meter |
| **Valore in dB** | Il livello del fader. Doppio clic per scrivere un valore a mano |
| **Picco** | Il picco più alto dall'ultimo reset (rosso se ha superato 0 dB). Un clic sul meter lo azzera |
| **Nome** | Barra nel colore della cella con la sigla (A1...) e il nome. Un clic seleziona la cella (e la mostra nella scheda Main) |

La cella selezionata ha il bordo arancione. I meter mostrano il segnale **dopo** gli effetti della cella e il suo fader; una cella in Mute (o esclusa da un Solo) mostra zero.

## Il master

A destra, in arancione: il **volume master** del kit (fader), il suo **meter** con picco e clip. È lo stesso controllo "Volume" della scheda Main. Dopo il master c'è la catena degli effetti master (scheda FX).

## Barra in alto

- **All cells**: mostra anche le celle vuote (di solito si vedono solo quelle con un campione).
- **Reset mutes** / **Reset solos**: spengono tutti i Mute o tutti i Solo.
- Con molte celle la fila di canali scorre in orizzontale; il master resta fermo a destra.

## Mute e Solo: cosa cambiano

- Valgono per **tutto l'audio della cella**: sequencer, tasti MIDI, clic sul pad, anteprime.
- **Non sono** il Mute/Solo delle righe del sequencer (quelli riguardano solo le note del pattern): sono indipendenti e si possono usare insieme.
- Sono **salvati nel kit** e annullabili con Ctrl+Z. Sono automatizzabili dalla DAW (parametri "Mute" e "Solo" di ogni cella).
- **Bounce WAV** (sequencer e song) rispetta Mute e Solo del mixer, come una traccia silenziata nella DAW. **Export lanes** e le tracce separate no: ogni cella viene sempre scritta, altrimenti avresti file vuoti.
- Il Mute non ferma i campioni: la cella continua a "girare" in silenzio, quindi riaccendendola nel mezzo di un loop non perde il tempo.

## Cose da sapere

- I meter sono di **picco** (non RMS/loudness): servono a evitare il clipping e a vedere i rapporti tra le celle.
- Le uscite extra (Out 2...) esistono solo nel plugin, se la DAW le ha attivate; nello standalone tutto va al Main.
- Non c'è ancora un pannello per **gruppi di canali** (bus), né i **pre/post-fader** delle mandate: le mandate sono sul segnale della cella prima del suo fader.
