# 5. Kit e file

Un **kit** contiene tutte le celle con i loro campioni e impostazioni, gli effetti master, i 16 pattern del sequencer e le assegnazioni del MIDI learn.

## Il formato del kit

Quando salvi un kit con il nome *MioKit*, vengono creati **due elementi affiancati**:

```
Kits/
├── MioKit.slkit      ← il file del kit (testo JSON leggibile)
└── MioKit/           ← cartella con lo stesso nome: i campioni usati, copiati
    ├── Kick 1.wav
    └── Snare 2.wav
```

- Il **nome del kit** è sempre quello del file: salvando con un altro nome (Save as) il nome cambia.
- I **campioni sono file audio normali**: se ti serve uno di loro lo prendi direttamente dalla cartella.
- Nel file `.slkit` i campioni sono scritti con percorsi **relativi**: puoi spostare o copiare insieme file e cartella.
- Le modifiche (Start/End, effetti, livelli…) sono salvate nel `.slkit` e **non alterano mai** i file audio.

## Tre tipi di file: kit, sequenze e song

Il **kit** è il cuore del programma, e il salvataggio principale (**Save**) scrive **solo il kit**. Sequenze e song hanno ciascuna il proprio file e il proprio pulsante di salvataggio:

| File | Cosa contiene | Come si salva | Come si apre |
|------|---------------|---------------|--------------|
| **`.slkit`** (kit) | Celle e campioni (copiati nella cartella), tutti i parametri, effetti per cella, master, MIDI learn. **Niente** sequenze né song | Menu **Kit → Save kit** / **Save kit as...** (o il pulsante **Save** in alto) | **Kit → Open...** | Apre un file `.slkit` (kit), `.slseq` (sequenze) o `.slsong` (song): riconosce il tipo dall'estensione |
| **`.slseq`** (sequenze) | I **16 pattern**: righe, step, lock degli effetti, swing, nomi, pattern attivo | Pulsante **Save seq** nella scheda **Seq** (seconda riga di comandi) | **Kit → Open...** | Apre un file `.slkit` (kit), `.slseq` (sequenze) o `.slsong` (song): riconosce il tipo dall'estensione |
| **`.slsong`** (song) | La catena di blocchi, il Loop e il nome | Pulsante **Save song** nella scheda **Song** | **Kit → Open...** | Apre un file `.slkit` (kit), `.slseq` (sequenze) o `.slsong` (song): riconosce il tipo dall'estensione |

**Open** è uno solo e riconosce il tipo dall'estensione:
- scegliendo un **`.slkit`** si cambiano i suoni (se hai modifiche non salvate al kit, chiede conferma) e le **sequenze e la song che stai usando restano** quelle di prima;
- scegliendo un **`.slseq`** i 16 pattern vengono sostituiti da quelli del file (chiede conferma, si annulla con Ctrl+Z);
- scegliendo un **`.slsong`** la catena viene sostituita da quella del file.

Cose da sapere:
- **L'asterisco** vicino al nome del kit riguarda **solo il kit**. Le modifiche a sequenze e song non salvate nei loro file non lo accendono, ma se stai per perderle (per esempio con **New kit**) l'app **chiede conferma** e te lo dice.
- **New kit** crea un progetto vuoto: azzera **anche** le sequenze e la song (dopo la conferma, se non erano salvate).
- I **vecchi kit** salvati con le versioni precedenti, che contengono anche i pattern e la song, si aprono ancora **per intero** (con tutto il loro contenuto); risalvandoli con **Save kit** i pattern e la song non vengono più scritti nel file: salvali prima con **Save seq** e **Save song** se ti servono.
- Le righe dei pattern e i blocchi della song richiamano le celle e i pattern **per numero** (A1 è la cella 0...): funzionano con qualunque kit, ma suonano ciò che c'è in quelle celle.
- Il **progetto della DAW** (il plugin dentro Cubase) salva invece **tutto insieme** (kit, sequenze e song) nel progetto, come prima: i tre file servono per riusare le parti tra progetti diversi.

## Il menu Kit e i pulsanti in alto

| Comando | Cosa fa |
|---------|---------|
| Nome del kit | Mostra il nome. Un asterisco (`*`) significa modifiche non salvate. "Untitled" = non ancora salvato |
| **< >** | Passa al kit precedente/successivo **nella stessa cartella** del kit corrente (attive solo se ce n'è un altro) |
| **Save** | Salva il **kit** (solo il kit: la prima volta chiede il nome) |
| **Kit → New kit** | Crea un kit vuoto |
| **Kit → Open...** | Apre un file `.slkit` (kit), `.slseq` (sequenze) o `.slsong` (song): riconosce il tipo dall'estensione |
| **Kit → Save kit** / **Save kit as...** | Salva / salva con un altro nome (solo il kit) |
| **Kit → Normalize all cells** | Normalizza il Level di tutte le celle (picco a −1 dBFS) |
| **Kit → Recent kits** | Gli **ultimi 5 kit** aperti o salvati, con **Clear recent** per svuotare l'elenco |

Se hai modifiche non salvate e apri un altro kit o ne crei uno nuovo, l'app **chiede conferma**.

## Sample mancanti

Se apri un kit e un campione non si trova (per esempio hai spostato la cartella), la cella mostra **MISSING** in rosso. Rimetti la cartella accanto al file `.slkit` e riapri il kit.

## Il progetto della DAW

Quando usi il plugin, lo stato completo (kit, parametri, assegnazioni MIDI) viene salvato **dentro il progetto della DAW** e ricaricato con lui. Per condividere o riutilizzare un kit in altri progetti, salvalo anche come `.slkit`.

Nello standalone lo stato dell'ultima sessione viene ripristinato all'avvio.

## Dove vanno i file esportati

Le esportazioni del sequencer (MIDI, audio) propongono una cartella **Bounces** accanto al file `.slkit`; se il kit non è ancora salvato propongono la cartella Documenti. I file trascinati direttamente nella DAW passano da una cartella temporanea del sistema.
