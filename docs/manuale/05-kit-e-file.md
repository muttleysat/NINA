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

## Il menu Kit e i pulsanti in alto

| Comando | Cosa fa |
|---------|---------|
| Nome del kit | Mostra il nome. Un asterisco (`*`) significa modifiche non salvate. "Untitled" = non ancora salvato |
| **< >** | Passa al kit precedente/successivo **nella stessa cartella** del kit corrente (attive solo se ce n'è un altro) |
| **Save** | Salva il kit (la prima volta chiede il nome) |
| **Kit → New kit** | Crea un kit vuoto |
| **Kit → Open...** | Apre un file `.slkit` |
| **Kit → Save** / **Save as...** | Salva / salva con un altro nome |
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
