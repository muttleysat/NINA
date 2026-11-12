# Sample Lab – Requisiti di prodotto

Versione: 0.5 (bozza) – 2026-11-12
Stato: documento vivo, da estendere iterativamente.

## 1. Visione

**Sample Lab** (nome temporaneo) è un plugin **VST3** di tipo drum sampler/sample player, ispirato a Native Instruments Battery. Permette di caricare singoli campioni o loop su una griglia di celle, mapparli su una tastiera MIDI, modificarli in modo non distruttivo (forma d'onda, dinamica, tune, time-stretch) e processarli con effetti per cella e globali. I kit si salvano come file `.slkit` collegato a una cartella con i sample, che restano file audio normali recuperabili direttamente.

## 2. Ambito della v1

**Dentro**: formato VST3, **Windows e macOS** (macOS Intel + Apple Silicon, build universale), uso personale, tutto quanto descritto ai punti 4–10.
**Fuori (per ora)**: AU/AAX, Linux, distribuzione commerciale, libreria di kit/sample inclusa, sintesi, registrazione audio, sequencer interno, scambio di kit con formati di terzi (es. Battery `.kt`).

## 3. Glossario

- **Cella**: slot della griglia che contiene un campione e i suoi parametri.
- **Kit**: insieme di celle + parametri + effetto globale + sample associati.
- **Loop**: campione con durata musicale (battute/beat), sincronizzabile al tempo del progetto.
- **One-shot**: campione singolo non ritmico.

## 4. Interfaccia grafica

Ispirata all'organizzazione di Battery, ma con grafica e asset originali (nessuna copia di risorse o marchi NI).

| ID | Requisito | Priorità |
|----|-----------|----------|
| UI-1 | Finestra unica ridimensionabile con tre aree: **browser** a sinistra, **griglia celle** in alto/centro, **pannello di editing** in basso. | Must |
| UI-2 | Il browser dà accesso all'intero file system (dischi, cartelle, root del PC), con albero cartelle, lista file, filtro per estensione audio, ricerca per nome. | Must |
| UI-3 | Preascolto (audition) del file selezionato nel browser, con volume regolabile e, per i loop, opzione di preascolto sincronizzato al tempo DAW. | Must |
| UI-4 | Caricamento in cella tramite drag & drop dal browser (e da Esplora risorse) o doppio clic sulla cella selezionata. | Must |
| UI-5 | Griglia di celle di default **4 righe (A–D) × 12 colonne = 48 celle**, con possibilità di **aggiungere/rimuovere righe e celle** (righe etichettate A, B, C…; colonne numerate). Ogni cella mostra nome, colore assegnabile, indicatore di attività e mute/solo. | Must |
| UI-9 | Layout di riferimento (screenshot Battery): barra superiore con nome kit, indicatore/campo BPM e Sync; colonna sinistra con tab *Library / Files* (e tag di categoria per i campioni); sotto la griglia, la **waveform** con marker S/E e controlli rapidi (Tune, Reverse, Pan, Level, Key Range, selezione layer); in basso la **catena effetti** a moduli affiancati; schede inferiori (Main, Effects, Modulation, Setup, Editor, Master) per i diversi pannelli. Da ripensare con grafica originale. | Should |
| UI-10 | Colori per cella assegnabili manualmente e per categoria (kick, snare, hat, ecc.). | Could |
| UI-6 | Selezionando una cella, il pannello di editing mostra i parametri di **quella** cella nella stessa finestra (nessuna finestra separata). | Must |
| UI-7 | Selezione "Kit/Globale" per lavorare sugli effetti e parametri dell'intero kit. | Must |
| UI-8 | Scalabilità UI (HiDPI / zoom) e tema scuro. | Should |

## 5. Gestione campioni e mapping MIDI

| ID | Requisito | Priorità |
|----|-----------|----------|
| MAP-0 | **Mapping automatico di default**: la prima cella (A1) è assegnata a **C1** (MIDI 36, convenzione C3 = 60 usata da Cubase) e ogni cella successiva alla nota seguente, in ordine orizzontale per riga (A1…A12, B1…B12, …). Con la griglia 4×12 si coprono C1–B4 (note 36–83). Aggiungendo celle il mapping prosegue; il valore di partenza è modificabile globalmente. | Must |
| MAP-1 | Ogni cella ha un **tasto root** e un **range di tasti** (nota bassa–alta). Di default il range coincide con la sola nota assegnata automaticamente; l'utente può cambiare **manualmente** la nota di una cella (o il range) in qualsiasi momento. Con range di più note il campione viene trasposto rispetto al root (uso melodico). Le celle manuali non vengono toccate se si riapplica il mapping automatico. | Must |
| MAP-2 | Assegnazione rapida con **MIDI learn**: premere un tasto sulla tastiera per assegnare la cella. | Must |
| MAP-3 | Range di **velocity** per cella e sensibilità alla velocity (volume, eventuale filtro). | Should |
| MAP-4 | Più celle sulla stessa nota (layer) e **gruppi di esclusione** (es. hi-hat chiuso/aperto, choke). | Should |
| MAP-5 | Modalità polifonica / monofonica per cella. | Must |
| MAP-6 | Formati supportati in import: WAV, AIFF, FLAC, MP3, OGG (WAV/AIFF 16/24/32 float, fino a 192 kHz). Resampling automatico alla frequenza del progetto. | Must |

## 6. Editing del campione (non distruttivo)

L'originale su disco non viene mai modificato; le modifiche sono parametri salvati nel kit.

| ID | Requisito | Priorità |
|----|-----------|----------|
| ED-1 | Visualizzazione forma d'onda con zoom e scroll, stereo/mono. | Must |
| ED-2 | **Start e End** regolabili con marker trascinabili sulla waveform; snap opzionale agli zero crossing. | Must |
| ED-3 | Loop point (sustain loop) opzionale, con crossfade. | Could |
| ED-4 | **Dinamica**: inviluppo di ampiezza **AHDSR** (attack, hold, decay, sustain, release), gain, pan, fade-in/fade-out, normalizzazione, inversione di fase. | Must |
| ED-5 | **Reverse** (riproduzione al contrario). | Should |
| ED-6 | Compressore/transient shaper per cella (rientra nella catena effetti, sez. 7). | Should |
| ED-7 | Undo/redo sulle modifiche dei parametri. | Should |

## 7. Effetti

Gli effetti sono disponibili **per cella** e a livello **globale (kit)**, con stessa interfaccia, bypass e dry/wet.

| ID | Requisito | Priorità |
|----|-----------|----------|
| FX-1 | Architettura: catena di effetti per cella → mixer di kit → catena globale → uscita. | Must |
| FX-2 | **Standard**: EQ parametrico, filtro (LP/HP/BP), compressore, limiter, saturazione/drive, delay, riverbero. | Must |
| FX-3 | **Creativi**: bit crusher/sample-rate reducer, distorsione/waveshaper, chorus, flanger, phaser, tape stop, stutter/gater, ring modulator. (Elenco iniziale, da raffinare.) | Should |
| FX-4 | Ordine degli effetti nella catena modificabile; numero di slot per cella (proposta: 4) e globale (proposta: 6). | Should |
| FX-5 | Send/aux per riverbero e delay condivisi tra celle. | Could |
| FX-6 | **Uscite multiple**: ogni cella è assegnabile a una uscita della DAW (stereo principale + uscite aggiuntive, proposta 16 stereo). L'assegnazione è salvata nel kit. Gli effetti per cella sono applicati prima dell'uscita assegnata; la catena globale agisce solo sulle celle instradate sul master. | Must |

## 8. Loop, sync e time-stretch

| ID | Requisito | Priorità |
|----|-----------|----------|
| LP-1 | Per ogni cella, modalità **Loop Sync**: il campione viene stirato per adattarsi al **BPM del progetto** letto dalla DAW (tempo variabile incluso). | Must |
| LP-2 | Definizione della lunghezza musicale del loop (n. di beat/battute) con rilevamento automatico del BPM e correzione manuale. | Must |
| LP-3 | Partenza del loop quantizzata (nessuna / beat / battuta) e allineamento alla posizione di trasporto DAW. | Should |
| LP-4 | Modalità di time-stretch con qualità selezionabile (es. ritmico/percussivo vs. tonale). | Must |
| LP-5 | Sync disattivabile: il loop può suonare a velocità originale (o a velocità variabile col pitch) senza sync. | Must |
| LP-6 | Slicing dei loop: vedi sezione 8.1. | Must |

### 8.1 Slicing dei loop

| ID | Requisito | Priorità |
|----|-----------|----------|
| SL-1 | **Slice automatici** su un loop: per rilevamento dei transienti (con sensibilità regolabile) o per griglia (divisione in N parti uguali / valore ritmico: 1/4, 1/8, 1/16…). | Must |
| SL-2 | **Slice manuali**: aggiungere, spostare ed eliminare marker sulla waveform, con snap a zero crossing o alla griglia ritmica. Gli slice automatici sono poi modificabili a mano. | Must |
| SL-3 | **Distribuzione sulle celle**: funzione "Distribuisci slice" che crea una cella per ogni slice, **in ordine orizzontale**, a partire da una cella di destinazione scelta, procedendo lungo la riga (A1→A12) e passando alla successiva (B1…), così da occupare note MIDI consecutive. Ogni cella riceve start/end, nome (es. `Loop_01`), e riferisce lo stesso file sorgente (nessuna copia del sample). | Must |
| SL-4 | Opzioni di distribuzione: cella di partenza, sovrascrivere o saltare le celle occupate, avviso/aggiunta di righe se le celle non bastano, assegnazione automatica di note MIDI consecutive, e modalità trigger (one-shot/gate) e uscita per le celle create. | Should |
| SL-5 | Le celle create dagli slice mantengono un collegamento al loop originale: se il loop viene risincronizzato, gli slice seguono il tempo del progetto. | Should |
| SL-6 | Modalità alternativa "slice su singola cella": il loop resta in una cella e gli slice sono suonati da note MIDI consecutive (stile MPC Chop). | Could |
| SL-7 | Possibilità di annullare l'intera distribuzione con un solo undo. | Should |

## 9. Tune, pitch e modalità di riproduzione

| ID | Requisito | Priorità |
|----|-----------|----------|
| PT-1 | **Tune** (intonazione fine, ±100 cent) e **Pitch** (semitoni, ±24) per cella. | Must |
| PT-2 | **Relazione pitch / time-stretch** (comportamento tipo *Key Lock / Master Tempo* di VirtualDJ): quando un campione è sincronizzato al tempo del progetto, il time-stretch adatta la durata **preservando l'intonazione originale**, senza stravolgere il timbro. Il pitch percepito resta quello impostato da Tune/Pitch dell'utente. | Must |
| PT-2b | Selettore per cella **Key Lock** on/off: con Key Lock off la velocità di riproduzione è accoppiata al pitch (varispeed, come un giradischi); con on (default per i loop sincronizzati) l'intonazione è indipendente dal tempo. | Should |
| PT-2c | Limiti di stretch consigliati per mantenere qualità (es. ±50% / ±8 BPM tipici); oltre, avviso o scelta del motore di stretch più adatto. | Could |
| PT-3 | Pitch bend MIDI e keytracking (la nota suonata trasposta rispetto al root) attivabili per cella. | Should |
| PT-4 | **Modalità di trigger stile Akai MPC**, selezionabile per cella e valida sia per one-shot sia per loop: | Must |
| | • **Note On (one-shot)**: il campione suona fino in fondo, il Note Off è ignorato. | |
| | • **Note Off / gate**: il campione suona solo finché il tasto è premuto; al rilascio parte la fase di release dell'inviluppo. | |
| | • **Loop**: il campione si ripete finché il tasto è premuto. | |
| PT-5 | Round-robin/alternanza e randomizzazione leggera (pitch, velocity) per cella. | Could |

## 10. Kit: salvataggio e caricamento

Formato: un **file kit `.slkit`** (testo JSON leggibile) collegato a una **cartella di sample** normale, dalla quale i campioni si possono recuperare direttamente con Esplora risorse/Finder. Il file e la cartella stanno affiancati:

```
Kits/
├── MioKit.slkit            # JSON: versione schema, nome, celle, mapping, parametri, effetti, tempi, riferimento alla cartella
└── MioKit/                 # cartella con lo stesso nome del kit: sample copiati (file audio originali, apribili ovunque)
    ├── Kick Funk 7.wav
    ├── Snare Funk 1.wav
    └── ...
```

| ID | Requisito | Priorità |
|----|-----------|----------|
| KIT-1 | Al salvataggio, l'utente dà un **nome al kit** nell'applicazione; sia il file sia la cartella assumono **esattamente quel nome** (`<NomeKit>.slkit` e cartella `<NomeKit>/`, affiancati nella stessa posizione). I sample usati vengono **copiati nella cartella**, come file audio originali non rinominati né convertiti. Caratteri non validi per il file system vengono sostituiti; se nome o cartella esistono già, richiesta di conferma per sovrascrivere o scelta di un altro nome. | Must |
| KIT-1b | Rinominando il kit con "Salva con nome", file e cartella vengono creati con il nuovo nome; il nome del kit mostrato nella barra superiore coincide sempre con quello del file. | Must |
| KIT-2 | `kit.json` versionato (`schemaVersion`) con migrazione dalle versioni precedenti. | Must |
| KIT-3 | Il file `.slkit` memorizza i sample con **percorsi relativi** alla cartella del kit (compatibili Windows/macOS). Il kit viene salvato anche nello stato del plugin della DAW (recall del progetto) con lo stesso riferimento. | Must |
| KIT-3b | Possibilità di scegliere una cartella di sample diversa (es. condivisa da più kit) e di **salvare il kit come copia** in una nuova posizione, con copia o collegamento dei sample. | Should |
| KIT-4 | Sample duplicati (stesso contenuto) copiati una sola volta (hash). Nomi file in conflitto risolti con suffisso numerico. | Should |
| KIT-5 | Gestione sample mancanti o cartella spostata: avviso, ricerca automatica accanto al file `.slkit` e possibilità di rilocare la cartella. | Must |
| KIT-8 | **Apri / Salva / Salva con nome / Nuovo kit** dal menu del plugin (barra superiore, accanto al nome del kit), con finestra di selezione file `.slkit`. Avviso se ci sono modifiche non salvate prima di aprire un altro kit. | Must |
| KIT-9 | **Kit recenti**: un menu a tendina (nel nome del kit in alto) elenca gli **ultimi kit aperti o salvati** (ultimi 5, più recente in alto), con il nome del kit e il percorso al passaggio del mouse. Selezionando una voce il kit si carica. I kit non più presenti sono segnalati e rimovibili dall'elenco; è disponibile "Svuota recenti". L'elenco è memorizzato nelle impostazioni utente e condiviso tra tutte le istanze del plugin. | Must |
| KIT-7 | Poiché i sample sono file indipendenti, le modifiche (start/end, effetti, ecc.) restano **non distruttive** nel `.slkit` e non alterano mai i file audio. | Must |
| KIT-6 | Salvataggio/caricamento di **singola cella** (preset di cella). | Could |

## 11. Requisiti non funzionali

| ID | Requisito |
|----|-----------|
| NF-1 | Latenza trascurabile: nessuna allocazione/lock/IO nel thread audio; sample caricati in RAM (streaming da disco per sample lunghi come estensione futura). |
| NF-2 | Polifonia minima 64 voci totali; CPU contenuta con 16 celle attive e effetti. |
| NF-3 | Elaborazione audio a 32-bit float interna; supporto 44.1–192 kHz e buffer variabili. |
| NF-4 | Stabilità: nessun crash su file corrotti o non supportati; caricamento in thread separato. |
| NF-5 | Compatibilità con le DAW principali su Windows e macOS (DAW di riferimento: **Cubase 16** (Windows e macOS); verifiche secondarie su Reaper, Ableton Live, FL Studio; Logic Pro non carica VST3 e per ora è escluso). Validazione con il validator VST3 di Steinberg. |
| NF-8 | Kit `.slkit` identici e interscambiabili tra Windows e macOS (percorsi relativi, endianness e codifica nomi file gestite). |
| NF-6 | Tutti i parametri automatizzabili dalla DAW (con ID stabili). |
| NF-7 | Lingua UI: inglese (italiano come possibile traduzione futura). |

## 12. Proposta tecnica (da confermare)

- **Linguaggio e framework**: C++17/20 con **JUCE** (VST3, GUI, gestione audio/MIDI/file). Build con CMake.
- **Time-stretch/pitch**: libreria di terze parti. Candidate: **Signalsmith Stretch** (MIT, buona qualità, semplice da integrare), **Rubber Band** (qualità eccellente ma GPL/licenza commerciale), **SoundTouch** (LGPL). Da valutare in base alla licenza di distribuzione.
- **Licenze**: essendo ad uso personale e non distribuito, JUCE (GPL/tier gratuito) e Rubber Band (GPL) sono utilizzabili senza costi; da rivedere se in futuro il plugin venisse distribuito.
- **Build multi-piattaforma**: CMake + CI (es. GitHub Actions) con runner Windows e macOS; la build per Mac richiede macOS (o CI) e, per uso locale, firma ad-hoc del bundle VST3.
- **Slicing**: rilevamento transienti con onset detection (spectral flux) e beat tracking per il BPM; candidate librerie: aubio, o implementazione propria.
- **Uscite multiple**: bus audio multipli in JUCE (AudioProcessor con più output bus); attenzione al supporto variabile tra DAW.
- **Marchio**: "Battery" e il suo aspetto sono di Native Instruments; ci si ispira a concetto e layout, ma nome, grafica e asset saranno originali.

## 13. Domande aperte

**Risolte (v0.3)**: pitch con Key Lock stile VirtualDJ (PT-2); griglia 4×12 espandibile fino a **128 celle** (UI-5); uso personale; Windows + macOS; uscite per cella (FX-6); slicing in v1 con distribuzione orizzontale per riga (sez. 8.1); mapping automatico da C1 una nota per cella, con override manuale (MAP-0/MAP-1); AU/Logic escluso; DAW di riferimento Cubase 16.

Formato kit deciso: file `.slkit` (JSON) + cartella di sample collegata (sez. 10).

**Ancora aperte**: nessuna per ora.

## 14. Roadmap indicativa

1. **MVP**: plugin VST3 (Win + Mac) con griglia 4×12, browser file, mapping MIDI, trigger one-shot/gate, start/end, AHDSR, tune/pitch, uscite per cella, salvataggio kit.
2. **Effetti**: catena per cella e globale (standard poi creativi).
3. **Loop**: sync al BPM, time-stretch con Key Lock.
4. **Slicing**: slice automatici/manuali e distribuzione sulle celle.
5. **Rifinitura**: layer/gruppi, undo, ottimizzazioni, test multi-DAW.
