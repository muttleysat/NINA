# 4. Sequencer

Il sequencer è nella scheda **Seq**. Ogni **riga** suona una cella; ogni riga ha i suoi **step** (fino a 64), la sua lunghezza e la sua risoluzione.

## Avviare e fermare

- **Play/Stop** nella barra in alto, oppure la **barra spaziatrice**.
- **Standalone:** il sequencer ha un trasporto interno e parte dall'inizio ogni volta che premi Play. Il tempo si imposta nel riquadro **BPM**.
- **Plugin:** se il trasporto della DAW sta suonando, il sequencer lo **segue** (tempo e posizione) e il pulsante Play è disattivato. Se la DAW è ferma, puoi provare il pattern con il Play del plugin.

## Aggiungere le celle

- Dalla griglia: tasto destro su un pad → **Add to sequencer**.
- Dalla scheda Seq: **+ Cell** apre un elenco con **ID, nome e forma d'onda**. Passando il mouse su una voce **il campione suona**, così scegli ascoltando. In fondo c'è **Add all loaded cells**.
- Per togliere una riga: il pulsante **x** della riga.

Un **clic sul nome** della riga fa suonare la cella e la seleziona (così la scheda FX e il pannello a destra mostrano quella cella).

## La riga

Da sinistra a destra:

| Elemento | Funzione |
|----------|----------|
| **M** | Mute della riga |
| **S** | Solo: se almeno una riga è in solo, suonano solo le righe in solo |
| **FX** | Acceso: la riga usa gli effetti della cella. Spento: suona senza effetti per nota (filtro, saturazione, LoFi) e senza mandate a delay/riverbero. Gli step possono riaccenderli con i lock |
| Nome | ID e nome della cella con la mini forma d'onda |
| Risoluzione | Durata di ogni step: da **1/1** a **1/64**, anche puntati e terzine. Ogni riga ha la sua |
| Lunghezza | Numero di step (1–64). Ogni riga ha la sua lunghezza: con lunghezze diverse nascono le **poliritmie** |
| **x** | Rimuove la riga |

Le righe sono **sincronizzate al tempo**: righe con risoluzioni diverse (per esempio una a 1/16 e una a 1/32) restano allineate.

### Lunghezza e colonne

La lunghezza di ogni riga si imposta da **8 a 64** con **+** e **−** (o trascinando il numero). Le colonne visibili sono quelle della riga più lunga: si vedono sempre **tutti gli step su una riga, senza pagine**, e si restringono con la lunghezza. Se la finestra è troppo stretta per 64 step compare lo scroll orizzontale e l'intestazione delle righe resta ferma. Un righello numera gli step ogni 4 e gli step sono raggruppati a 4 a 4. Gli step oltre la lunghezza di una riga sono spenti e non cliccabili. Un contorno bianco segue lo step che sta suonando.

La scheda Seq usa tutta la larghezza della finestra (browser e pannello della cella sono nella scheda Main). Scorri in verticale se hai molte righe.

## Gli step

| Azione | Risultato |
|--------|-----------|
| Clic su uno step **vuoto** | Lo accende e lo seleziona |
| Clic su uno step acceso **non selezionato** | Lo seleziona (senza spegnerlo) |
| Clic su uno step acceso **già selezionato** | Lo spegne |
| **Tasto destro** o **Maiusc+clic** | Solo selezione, non cambia l'accensione |
| **Trascinare su/giù** uno step acceso | Cambia la **velocity** (l'altezza della barra dello step) |
| **Trascinare in orizzontale** partendo da uno step vuoto | "Pennello": accende tutti gli step toccati (anche su altre righe) |
| **Trascinare in orizzontale** partendo da uno step acceso | "Gomma": spegne tutti gli step toccati |

Sullo step acceso, piccoli segni indicano: un **pallino** = ha dei lock sugli effetti; **x2/x3/x4** = ratchet; **%** = probabilità sotto il 100%.

### Copiare e incollare step e righe

- **Step singolo:** seleziona uno step (tasto destro o Maiusc+clic lo seleziona senza cambiarlo) e premi **Ctrl+C**; seleziona un altro step e premi **Ctrl+V**. Si copia tutto lo step: acceso/spento, velocity, probabilità, ratchet, nudge, gate, pitch, reverse, modalità e **lock** degli effetti. Si può incollare anche su un'altra riga o in un altro pattern.
- **Riga intera:** tasto destro sulla riga (sul nome della cella o su uno spazio vuoto dell'intestazione) → **Copy lane**; poi sulla riga di destinazione → **Paste into this lane**. Si copiano gli step, la lunghezza e la risoluzione; la cella, Mute, Solo e FX della riga di destinazione restano. **Clear all steps** svuota la riga.
- Tutto si annulla con **Ctrl+Z**.

## L'editor dello step (STEP EDIT)

In basso, per lo step selezionato. In alto il titolo dice quale step stai modificando.

### Comportamento

| Controllo | Effetto |
|-----------|---------|
| **Mode** | **As cell**: usa il Trigger mode della cella. **One-shot**: suona tutto il campione. **Gate**: suona per la durata dello step e poi si spegne con il Release. **Loop**: ripete il campione per la durata dello step |
| **Reverse** | Suona questo step al contrario (si combina con il Reverse della cella: due inversioni si annullano) |
| **Velocity** | Volume dello step (1–127) |
| **Probability** | Probabilità che lo step suoni (0–100%) |
| **Nudge** | Anticipa o ritarda lo step (−50…+50% della sua durata) |
| **Gate** | Durata della nota in percentuale dello step: conta per le modalità Gate e Loop |
| **Pitch** | Trasposizione dello step (±24 semitoni) |
| **Ratchet** | Ripete il colpo 1–4 volte dentro lo step |

La **probabilità è ripetibile**: lo stesso pattern dà sempre lo stesso risultato (utile per l'esportazione). **Reset seed** cambia il "seme" casuale: le scelte di probabilità cambiano ma restano ripetibili.

### Effetti per step (lock)

Sotto le manopole ci sono le carte **Saturation, LoFi, Filter, Delay, Reverb**. Servono a cambiare un effetto **solo su quello step**.

- **Manopola grigia**: segue il valore della cella.
- **Manopola arancione**: valore **bloccato** su questo step. Si blocca muovendo la manopola; **doppio clic** toglie il blocco.
- Bloccare un valore **accende l'effetto per quel colpo**, anche se sulla cella è spento. Con l'interruttore della carta puoi forzare l'effetto acceso o spento per lo step.
- **Clear locks** toglie tutti i lock dello step.

I parametri bloccabili sono quelli che hanno senso per un singolo colpo:

| Carta | Si blocca | Resta della cella (non si blocca per step) |
|-------|-----------|--------------------------------------------|
| Saturation | Drive, Mix | — |
| LoFi | Bits, Rate, Mix | — |
| Filter | Cutoff, Reso | Tipo di filtro |
| Delay | **Send** (quanto viene mandato) | Time, Feedback, Sync, valore musicale (ma si regolano dalla stessa carta) |
| Reverb | **Send** | Size, Damp (ma si regolano dalla stessa carta) |

Nella carta Delay trovi anche **Sync** e il valore musicale: sono le impostazioni della cella (le stesse della scheda FX).

**Esempio:** un'eco solo sul terzo colpo.

1. Sulla cella lascia il **Delay spento**.
2. Seleziona lo step e nella carta Delay imposta **Time** (o Sync + valore) e **Feedback**.
3. Sposta **Send** verso l'alto: solo quel colpo manda l'eco, e l'eco continua a risuonare dopo lo step.

## Pattern, swing e battute

La barra in alto della scheda Seq ha due righe:

- **Riga 1:** `< Pattern 1 >` e il menu **⋯** (azioni sul pattern) · **+ Cell** e il pulsante a quadrati (**aggiungi tutte le celle con campione**) · **Export N bars** e **Tail** · **Export MIDI**, **Bounce WAV**, **Export Lanes** · la manopola **Swing** · **Reset Seed**.
- **Riga 2:** le maniglie **DRAG TO DAW** (capitolo 7).

### I 16 pattern

`< Pattern 1 >` sceglie uno dei **16 pattern** del kit, ognuno indipendente (righe, step, swing, seed).

Il menu **⋯** accanto ai pattern permette di lavorare su di essi:

| Voce | Cosa fa |
|------|---------|
| **Duplicate to the next empty pattern** | Copia il pattern attivo nel primo pattern **vuoto** che segue (se arrivi in fondo riparte dall'inizio) e lo apre: lo modifichi o lo allunghi senza toccare l'originale. Se non c'è nessun pattern vuoto compare un avviso |
| **Copy pattern** | Mette il pattern attivo negli appunti |
| **Paste into this pattern** | Sostituisce il contenuto del pattern attivo con quello copiato (chiede conferma se non è vuoto). Il **nome** del pattern non cambia |
| **Clear pattern** | Svuota il pattern attivo (chiede conferma) |
| **Rename...** | Cambia il nome mostrato nel menu |

**Cambiare pattern mentre suona.** Se il sequencer sta suonando e scegli un altro pattern, il pattern in corso **finisce il suo ciclo** e il nuovo parte **al giro successivo**, senza tagli. Il ciclo è la durata della riga più lunga del pattern in corso (per esempio 16 step a 1/16 = una battuta; 64 step a 1/16 = quattro battute). Nel frattempo il pattern scelto è già visibile e modificabile; il contorno bianco che segue lo step sta sul pattern che sta davvero suonando, quindi sul nuovo compare solo quando parte. Le note tenute (modalità Gate o Loop) del vecchio pattern vengono rilasciate al cambio. Da fermo il cambio è immediato.

Per copiare in un pattern preciso: **Copy** sul pattern sorgente, passa a quello di destinazione con **< >**, poi **Paste**. Gli appunti valgono finché l'app è aperta e non vengono salvati nel kit.

### Swing e seed

- **Swing**: ritarda gli step dispari di una frazione dello step (0–50%).
- **Reset Seed**: cambia il "seme" casuale (probabilità e ratchet cambiano ma restano ripetibili).
- **Export 1/2/4/8 bars** e **Tail**: lunghezza e coda delle esportazioni (capitolo 7); non cambiano la riproduzione.

## Registrare da tastiera e quantizzare

Nella seconda riga della scheda, accanto alle maniglie per la DAW, ci sono **Click**, **Rec** e **Rec options**.

**Click** (metronomo) e conteggio iniziale. Il pulsante **Click** accende un **metronomo**: un click a ogni quarto mentre il sequencer gira, più alto sul primo quarto di ogni battuta (4/4). Va al Main e non passa dagli effetti. Per registrare in tempo c'è anche il **conteggio iniziale** (precount): con **Rec** acceso e il trasporto interno (standalone, o plugin con la DAW ferma), il **Play** non parte subito dal pattern ma conta prima **1 battuta** (4 click, anche a metronomo spento) e il pulsante Rec mostra il numero che manca (4, 3, 2, 1). Poi parte il pattern e si registra. La durata si sceglie in **Rec options → Count-in before Play** (**Off**, **1 bar**, **2 bars**). Nel plugin, con la DAW in marcia, il conteggio è quello della DAW. Le note suonate durante il conteggio non si registrano (tranne un anticipo di mezzo quarto sul primo quarto).

**Rec** (registrazione).
1. Accendi **Rec** e fai partire il sequencer (**Play**): dopo il conteggio iniziale, se è attivo.
2. Suona la tastiera MIDI: ogni nota **accende lo step** più vicino della riga della cella che quella nota suona, con la **velocity** con cui l'hai premuta. Se la cella non ha ancora una riga, viene aggiunta.
3. Se la cella copre più tasti (campione melodico), si registra anche l'**intonazione** (Pitch dello step = nota suonata − Root note).
4. La registrazione **sovrascrive** velocity, intonazione e posizione dello step su cui cade; gli altri parametri dello step (probabilità, ratchet, lock...) restano.
5. Le note che suoni continuano a suonare normalmente mentre registri. Rec non registra mentre suona la scheda **Song**. Con la DAW in registrazione (plugin) e la modalità "silent while recording" il sequencer sta zitto e Rec non scrive nulla.

Una nota che cade tra due step va a quello più vicino. Quanto del tuo "feeling" resta lo decide **Rec options → Recorded notes: snap to the grid**: **100%** (predefinito) mette la nota esattamente sulla griglia, **75 / 50 / 25%** ne conservano una parte come **Nudge**, **Free** conserva tutto il micro-timing. La registrazione non compensa la latenza della scheda audio.

**Rec options** (menu, con le impostazioni della registrazione e la quantizzazione degli step già scritti).
- **Quantize existing steps → All lanes** o **Selected lane** con intensità 100 / 75 / 50 / 25%: riporta verso la griglia gli step che hanno un **Nudge** (per esempio quelli registrati con snap parziale). 100% azzera il Nudge.
- Lo **swing** del pattern non viene toccato: resta com'è.
- Lo spostamento tra risoluzioni diverse non è una quantizzazione: per cambiare la divisione di una riga si usa la sua risoluzione (vedi sopra).

## Cose da sapere

- Copia e incolla valgono per singolo step, riga intera o pattern intero, non per gruppi di step. **Ctrl+Z / Ctrl+Y** annullano e ripetono anche le modifiche al sequencer (capitolo 8).
- Un lock vale per il colpo di quello step; la sua eco (delay/riverbero) continua anche dopo.
- Modificare una cella (nome, effetti, livelli) cambia anche come suona nel sequencer, perché la cella è la stessa.
