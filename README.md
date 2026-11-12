# Sample Lab

Sampler/drum machine VST3 + standalone (Windows e macOS), ispirato a Native Instruments Battery, con sequencer a step. Uso personale.

- Requisiti: [docs/REQUISITI.md](docs/REQUISITI.md)
- Architettura: [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md)

## Build (Windows)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output in `build/SampleLab_artefacts/Release/` (cartelle `VST3` e `Standalone`).

## ASIO (solo standalone su Windows)

I driver ASIO richiedono l'SDK Steinberg, che non è nel repository (licenza). Per abilitarli:

1. Scarica l'ASIO SDK da <https://www.steinberg.net/asiosdk>.
2. Estrai il contenuto in `third_party/asiosdk/` (deve esistere `third_party/asiosdk/common/iasiodrv.h`).
3. Riesegui `cmake -S . -B build ...`: la configurazione stampa "ASIO SDK trovato".

Senza SDK lo standalone usa solo WASAPI/DirectSound. Nel VST3 la scheda audio è gestita dalla DAW, quindi ASIO non serve.

## Test

```powershell
cmake --build build --config Release --target SampleLabTests
build\SampleLabTests_artefacts\Release\SampleLabTests.exe
```

Coprono motore di riproduzione (mapping, velocity, level, start/end, pitch, reverse, one-shot/gate/loop, layer, voice stealing) e formato kit (salvataggio con cartella omonima, ricarica, sample mancanti, versione schema).

## Stato

Sequencer (primo passo): scheda **Seq** con una riga per ogni cella aggiunta (tasto destro su una cella > "Add to sequencer"), step con velocity, probabilità, ratchet, nudge, gate, pitch e parameter lock sugli effetti; esportazione MIDI e bounce WAV. MIDI learn sui parametri (tasto destro su un controllo) e delay sincronizzato al BPM.

Fase 2 (effetti) avviata: scheda **FX** con sei effetti per cella e per il kit (Saturation, LoFi, Filter, Compressor, Delay, Reverb), ciascuno con interruttore; delay e riverbero continuano a suonare dopo la fine della nota.

Fase 1 (MVP sampler) in corso: griglia 4×12 con mapping da C1, browser dei file, drag & drop sui pad, editor di cella (forma d'onda con Start/End, AHDSR, Level, Pan, Tune, Pitch, modalità one-shot/gate/loop, reverse, range note), kit `.slkit` con cartella dei sample omonima, apri/salva/kit recenti, menu di ridimensionamento a percentuali. Mancano: uscite multiple per cella, verifica in Cubase 16, build su macOS.
