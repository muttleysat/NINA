# 6. Automazione e MIDI learn

## Automazione dalla DAW (plugin VST3)

Tutti i controlli di NINA sono **parametri automatizzabili** dalla DAW, in lettura e scrittura, con **nomi leggibili** (non numeri di CC).

### Come compaiono nella DAW

Ogni cella ha i suoi parametri, ordinati in gruppi. Per esempio:

- `A1 Level`, `A1 Pan`, `A1 Tune`, `A1 Pitch`, `A1 Start`, `A1 End`, `A1 Reverse`, `A1 Trigger Mode`
- Inviluppo: `A1 Attack`, `A1 Hold`, `A1 Decay`, `A1 Sustain`, `A1 Release`
- Effetti: `A1 Saturation Drive`, `A1 Filter Cutoff`, `A1 Filter Type`, `A1 Delay Time`, `A1 Delay Sync`, `A1 Delay Division`, `A1 Reverb Mix`… e gli interruttori `A1 Delay On`…
- Master: `Master Level` e gli stessi effetti con prefisso `Master`.

Sono **38–40 parametri per cella** per le 48 celle, più quelli del master: circa 1.950 in totale. I **valori hanno unità e testo**: "2400 Hz", "Gate", "On", "1/8".

I parametri sono raggruppati per cella ed effetto (per esempio *Cell A1 → A1 Filter → …*), così sono ordinati e cercabili nel menu dell'automazione.

### Registrare automazione

Muovendo un controllo di NINA mentre la DAW è in registrazione dell'automazione (Write, Touch, Latch), il movimento viene registrato. Il movimento di una manopola segnala alla DAW l'inizio e la fine del gesto. L'automazione registrata si **riproduce** muovendo le manopole.

### Cose da sapere

- **Gli ID dei parametri sono stabili**: non cambieranno tra le versioni, così le automazioni dei tuoi progetti non si rompono.
- L'automazione agisce anche nell'**esportazione offline** della DAW.
- Il **Bounce WAV** e **Export lanes** di NINA usano i valori **correnti** dei controlli, **non** l'automazione della DAW.
- I valori dei lock degli step (capitolo 4) non sono parametri della DAW: sono dentro il pattern.
- Il mapping delle note (Root, Key low, Key high) non è automatizzabile.

> **Stato della verifica:** il comportamento dei nomi, dei gruppi e della registrazione in **Cubase 16** non è ancora stato provato; è stato verificato con test automatici. Se qualcosa non appare come descritto, segnalalo.

## MIDI learn

Il MIDI learn collega una manopola o un fader del tuo controller MIDI a un controllo di NINA.

### Assegnare

1. **Tasto destro** sul controllo (manopola, interruttore, menu...).
2. Scegli **MIDI Learn**: il controllo si evidenzia con "MOVE CC...".
3. **Muovi la manopola** del controller: la prima volta che invia un valore, l'assegnazione è fatta. Sul controllo compare l'etichetta **CC74** (il numero del tuo controller).

### Rimuovere

Tasto destro sul controllo → **Clear MIDI mapping**. Per annullare un apprendimento in corso scegli di nuovo la voce nel menu.

### Come si comporta

- Il controllo segue il controller **su tutto l'intervallo** (0–127). Gli **interruttori** si accendono da 64 in su; i **menu a scelta** (per esempio il tipo di filtro) scelgono la voce in base al valore.
- Un parametro ha **un solo** CC; lo stesso CC può muovere **più parametri**.
- Le assegnazioni si **salvano con il kit** (`.slkit`) e nel progetto della DAW.
- Nel plugin i CC arrivano dalla DAW (per esempio da una traccia MIDI che li invia allo strumento); nello standalone arrivano dal dispositivo MIDI scelto in **Options**.
- Il MIDI learn vale per i controlli di NINA; le note che suonano le celle (capitolo 2) sono separate.

### Non ancora disponibile

Pitch bend e aftertouch, filtro per canale MIDI, modalità relativa degli encoder, "soft takeover" (il controllo salta al valore del controller) e un pannello con l'elenco di tutte le assegnazioni.
