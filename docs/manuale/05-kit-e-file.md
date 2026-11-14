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

## Cosa contiene un kit, e salvare per sezione

Il salvataggio normale (**Save**, **Save as...**) scrive un **progetto completo** in un solo file `.slkit`: celle e campioni, effetti, master, MIDI learn, i **16 pattern** del sequencer e la **song**. Per tenere le parti separate ci sono tre salvataggi, uno per sezione:

| Sezione | Cosa salva | Dove | File |
|---------|------------|------|------|
| **Kit** (suoni) | Celle, campioni (copiati nella cartella), effetti, master, MIDI learn. **Senza** sequenze e song | **Kit → Save kit sounds only...** | `.slkit` |
| **Sequenze** | I **16 pattern** (righe, step, lock, swing, nomi) e il pattern attivo | Scheda **Seq**, menu **⋯** accanto al nome del pattern → **Save all sequences...** | `.slseq` |
| **Song** | La catena di blocchi, il Loop e il nome | Scheda **Song** → **Save song** | `.slsong` |

**Per aprirli c'è un solo comando: Kit → Open...** Sceglie da solo cosa fare in base al tipo di file:

- **`.slkit`** (kit): apre il kit. Se il file contiene **sequenze o una song** (salvataggio normale) sostituisce tutto come prima; se invece è un kit **dei soli suoni** (senza pattern né song, per esempio salvato con *Save kit sounds only*) cambia i suoni e **mantiene le sequenze e la song che hai ora**. Se hai modifiche non salvate, chiede conferma.
- **`.slseq`** (sequenze): sostituisce **tutti i 16 pattern** con quelli del file (chiede conferma, si annulla con Ctrl+Z).
- **`.slsong`** (song): carica la catena di blocchi al posto di quella attuale.

Cose da sapere:
- **Save kit sounds only** salva una **copia**: il kit su cui stai lavorando non cambia (nome, percorso e asterisco restano come sono). I campioni vengono copiati nella cartella accanto al file, come per il salvataggio normale.
- Aprendo un kit dei soli suoni, il kit risulta **modificato** (asterisco): contiene le sequenze di prima, che non sono nel file aperto. Un **Save** normale lo sovrascriverebbe con tutto. Un kit senza nessuna riga nei pattern e senza song è considerato «dei soli suoni»: per ripartire da zero usa **Kit → New kit**.
- Le righe dei pattern e i blocchi della song richiamano le celle e i pattern **per numero** (A1 è la cella 0...): funzionano con qualunque kit, ma suonano ciò che c'è in quelle celle.

## Il menu Kit e i pulsanti in alto

| Comando | Cosa fa |
|---------|---------|
| Nome del kit | Mostra il nome. Un asterisco (`*`) significa modifiche non salvate. "Untitled" = non ancora salvato |
| **< >** | Passa al kit precedente/successivo **nella stessa cartella** del kit corrente (attive solo se ce n'è un altro) |
| **Save** | Salva il kit (la prima volta chiede il nome) |
| **Kit → New kit** | Crea un kit vuoto |
| **Kit → Open...** | Apre un file `.slkit` (kit), `.slseq` (sequenze) o `.slsong` (song): riconosce il tipo dall'estensione |
| **Kit → Save** / **Save as...** | Salva / salva con un altro nome (progetto completo) |
| **Kit → Save kit sounds only...** | Salva i soli suoni (senza sequenze e song) in un `.slkit` |
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
