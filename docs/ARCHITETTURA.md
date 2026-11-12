# Sample Lab – Architettura tecnica

Versione: 0.1 – riferita a `REQUISITI.md` v1.4.

## 1. Stack

| Ambito | Scelta |
|---|---|
| Linguaggio | C++20 |
| Framework | JUCE 8 (VST3 + Standalone dallo stesso target) |
| Build | CMake (≥ 3.22), JUCE scaricato con FetchContent |
| Piattaforme | Windows x64 (MSVC 2022), macOS universale (arm64 + x86_64) |
| Time-stretch | Rubber Band (R3) – principale; Signalsmith Stretch – alternativa |
| Analisi (BPM/transienti) | Essentia (aubio come alternativa) |
| Decodifica | JUCE (WAV/AIFF/FLAC/OGG) + libsndfile/minimp3 |
| Resampling | libsamplerate / r8brain |
| JSON kit | nlohmann/json |

Le librerie si aggiungono per fase (vedi sez. 8), non tutte subito.

## 2. Struttura del repository

```
SampleLab/
├── CMakeLists.txt
├── docs/                  REQUISITI.md, ARCHITETTURA.md
├── Source/
│   ├── PluginProcessor.*  entry point audio/MIDI e stato (ora: scheletro)
│   ├── PluginEditor.*     finestra principale (ora: scheletro con layout adattivo)
│   ├── engine/            motore audio: voci, celle, mixer, uscite       (fase 1)
│   ├── dsp/               effetti, inviluppi, stretch, normalizzazione   (fasi 2-3)
│   ├── sequencer/         pattern, lane, step, clock, export MIDI/WAV    (fase 5)
│   ├── kit/               modello dati, (de)serializzazione .slkit       (fase 1)
│   ├── analysis/          BPM, transienti, slicing                        (fase 4)
│   └── ui/                componenti grafici                              (da fase 1)
└── third_party/           librerie non gestite via FetchContent
```

## 3. Moduli e responsabilità

```
        ┌──────────────┐   MIDI / trasporto    ┌─────────────────┐
 DAW ──▶│  Processor   │──────────────────────▶│   Sequencer     │
 /host  │ (entry point)│◀──────────────────────│ (lane per cella)│
        └──────┬───────┘   trigger celle       └─────────────────┘
               │ blocchi audio
        ┌──────▼───────┐
        │    Engine    │  VoicePool ◀─ Cell[0..N] (sample, mapping, envelope,
        │              │               pitch/stretch, fx chain, uscita)
        └──────┬───────┘
               │ per uscita
        ┌──────▼───────┐   ┌─────────────┐
        │ Mixer / Out  │──▶│ FX globale  │──▶ uscite DAW / dispositivo audio
        └──────────────┘   └─────────────┘
```

- **Processor**: riceve MIDI e blocchi, legge BPM e posizione di trasporto dall'host (o dal clock interno nello standalone), smista a Sequencer ed Engine. Persiste lo stato.
- **Engine**: possiede le **Cell** (fino a 128). Ogni cella ha: riferimento a un `SampleData` (buffer condiviso, immutabile), parametri (start/end, AHDSR, pitch, tune, keylock, volume, normalizzazione, modalità trigger, nota/range, uscita), catena effetti. Il **VoicePool** (64+ voci) alloca le voci in tempo reale.
- **Sequencer**: per ogni cella aggiunta, una **Lane** con step (fino a 64), risoluzione, lunghezza, swing. Produce eventi di trigger con micro-timing sample-accurate; applica i parameter lock come override temporanei sui parametri di cella.
- **Kit**: modello dati unico (`KitModel`) serializzato in `.slkit` (JSON) con cartella di sample omonima.
- **Analysis**: BPM, onset e slicing, eseguiti in un thread in background al caricamento.
- **UI**: componenti JUCE vettoriali con layout adattivo (sez. 6).

## 4. Thread e real-time

| Thread | Compiti | Regole |
|---|---|---|
| **Audio** | `processBlock`: sequencer, voci, effetti, mix | Nessuna allocazione, lock o I/O. Solo strutture lock-free e puntatori a buffer immutabili |
| **Messaggi/UI** | Interfaccia, menu, drag & drop | Non tocca direttamente lo stato audio: scrive parametri atomici o inserisce comandi in una coda |
| **Caricamento** | Decodifica file, resampling, analisi, normalizzazione, calcolo stretch pre-render | Pubblica il risultato all'audio thread con uno scambio atomico di puntatore (`shared_ptr` / RCU) |
| **Export** | Bounce offline WAV/AIFF/FLAC, stem | Usa una copia del motore, non il thread audio |

Comunicazione UI → audio: **parametri atomici** (`AudioProcessorValueTreeState` per quelli automatizzabili) e **FIFO lock-free** per comandi (cambio sample, aggiornamento pattern). Comunicazione audio → UI: meter, posizione di play e attività celle tramite atomici letti da un timer UI.

## 5. Modello dati e kit

```
Kit
 ├─ name, schemaVersion, globalFx[], masterVolume
 ├─ cells[]  { id, row, col, name, color, samplePath(relativo), start, end,
 │            envelope{AHDSR}, level, normalize{mode,target,gain,enabled},
 │            pitch, tune, keylock, bpmInfo{bpm,beats,sync}, slices[],
 │            midi{rootNote, lowNote, highNote, vel range}, triggerMode,
 │            outputIndex, fxChain[] }
 ├─ sequencer { patterns[] { lanes[] { cellId, resolution, length, swing,
 │               steps[] { on, vel, prob, ratchet, nudge, mode, pitch, locks{} } }, seed } }
 └─ ui { width, height, zoom }
```

- File `<Kit>.slkit` (JSON) + cartella `<Kit>/` con i sample; percorsi relativi, nome uguale (KIT-1).
- Le modifiche sono non distruttive: i file audio non vengono mai alterati.
- `schemaVersion` con funzioni di migrazione.
- Lo stato nel progetto della DAW contiene il riferimento al `.slkit` più lo stato non ancora salvato.

## 5.1 Parametri e automazione (implementato)

Tutti i valori automatizzabili sono parametri JUCE/VST3 raccolti in `ParamSet` (`Source/params/`):

- **ID stabili**: `c00.level`, `c00.filter.cutoff`, `c47.reverb.mix`, `master.level`… (cella a due cifre + nome). Non devono cambiare tra le versioni (AU-4): un test verifica un elenco di ID di riferimento.
- **Nomi leggibili e gruppi**: "A1 Filter Cutoff", raggruppati come *Cell A1 > A1 Filter > ...* e *Master > ...*; il testo del valore include l'unità ("2400 Hz", "Gate", "On").
- **Numero**: 38 parametri per cella × 48 celle + 26 del master = 1.850.
- **Fonte di verità**: i parametri, non il `Kit`. Il thread audio li legge a ogni blocco (`ParamSet::fillLive` → `Engine::LiveParams`), quindi l'automazione funziona anche nell'export offline. Il `Kit` conserva i valori solo per il salvataggio (`writeToKit` prima di salvare, `readFromKit` dopo il caricamento).
- **Interfaccia**: le manopole usano `SliderParameterAttachment` (e i corrispondenti per bottoni e combo): seguono l'automazione e aprono/chiudono i gesti verso la DAW (Write/Touch/Latch). I marker Start/End della forma d'onda aprono un gesto al trascinamento.
- **Parti non automatizzabili**: campioni e mapping delle note (root/low/high) restano nello snapshot pubblicato dal thread UI.
- Il sequencer (lock per step) e il MIDI learn scriveranno sugli stessi parametri (AU-9, AU-10).

## 6. Interfaccia e ridimensionamento

- Tutto il disegno è **vettoriale** (juce::Graphics, Path, gradienti): nessuna bitmap fissa.
- Il layout si calcola in `resized()` in proporzione alla dimensione corrente, con limiti minimi/massimi per colonne e pannelli (UI-13). Il fattore di scala si ricava dalla larghezza rispetto a una dimensione di riferimento (1280×760).
- La finestra è ridimensionabile (`setResizable`, limiti 1000×640 → 4K). Nello scheletro è già presente il **fallback a percentuali** (menu 75/100/125/150/200%) con limite alla dimensione dello schermo (UI-12b).
- Dimensione e zoom sono salvati nello stato (`uiWidth`, `uiHeight`, `uiZoom`).
- Il browser file usa `juce::FileTreeComponent` per l'accesso a tutto il file system, con preascolto su un thread separato.

## 7. Motore di riproduzione per cella

1. **Trigger** (MIDI, sequencer o pad UI) → alloca una voce con velocity, nota, modalità (one-shot / gate / loop).
2. **Lettura**: interpolazione di alta qualità (Hermite/sinc) con start/end, reverse e loop; AHDSR e fade.
3. **Pitch/Tune**: ricampionamento (varispeed) oppure, con **Key Lock**, stretch a pitch costante.
4. **Loop Sync**: rapporto di stretch = BPM progetto / BPM del loop; con tempo variabile il rapporto si aggiorna per blocco. Rubber Band in modalità real-time per le voci di loop; per i one-shot si evita lo stretch.
5. **Catena effetti di cella** (con parameter lock dello step corrente) → volume (Level × normalizzazione × velocity) → pan → **uscita assegnata**.
6. **Mixer**: le uscite assegnate vanno ai bus della DAW; il resto passa dalla catena globale e dal master.

Normalizzazione: calcolata al caricamento/modifica di start/end sul thread di caricamento (picco, RMS o LUFS) e salvata come guadagno in dB.

## 8. Fasi di sviluppo

| Fase | Contenuto | Dipendenze nuove |
|---|---|---|
| **0 – Scheletro** (fatta) | CMake, VST3 + Standalone, finestra ridimensionabile con layout e menu percentuali | JUCE |
| **1 – MVP sampler** (in corso: fatto tutto tranne uscite multiple e verifica in Cubase) | Griglia 4×12, browser, caricamento sample, mapping C1, voci, start/end, AHDSR, Level, tune/pitch, salvataggio/apertura kit `.slkit`, kit recenti. Il JSON usa `juce::var` (nessuna dipendenza aggiuntiva). | – |
| **2 – Effetti** (primo passo fatto) | Catena per cella e globale con ordine fisso Saturation > LoFi > Filter > Compressor > Delay > Reverb, ciascuna con on/off. Coda di delay/riverbero dopo la fine della nota. Da fare: EQ, effetti creativi (chorus, flanger, phaser, stutter…), ordine modificabile, send, uscite multiple per cella | – (usa `juce_dsp`) |
| **3 – Loop e stretch** | BPM, sync, Key Lock, normalizzazione | Rubber Band, libsamplerate |
| **4 – Slicing** | Slice automatici/manuali, distribuzione su celle | Essentia / aubio |
| **5 – Sequencer** | Lane, step, parameter lock, quantizzazione, export MIDI/audio | – |
| **6 – Standalone completo** | Trasporto interno, impostazioni audio/MIDI, record | – |
| **7 – Rifinitura** | Layer, gruppi, undo, ottimizzazione, test Cubase 16 | – |

Il target standalone esiste dalla fase 0 e serve come banco di prova rapido.

## 9. Test e qualità

- **Cubase 16** come DAW di riferimento; controllo con il validator VST3 di Steinberg.
- Test automatici (JUCE `UnitTest` o Catch2) su: serializzazione kit, mapping note, calcolo normalizzazione, quantizzazione, rapporto di stretch.
- Verifica in tempo reale: nessuna allocazione nel thread audio (strumento di controllo in debug).

## 10. Build

```powershell
# Windows (Visual Studio 2022 Build Tools)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# Output: build/SampleLab_artefacts/Release/{VST3,Standalone}
```

```bash
# macOS
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

Il VST3 si copia in `C:\Program Files\Common Files\VST3` (Windows) o `~/Library/Audio/Plug-Ins/VST3` (macOS) per essere visto da Cubase.
