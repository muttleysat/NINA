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

## Test

```powershell
cmake --build build --config Release --target SampleLabTests
build\SampleLabTests_artefacts\Release\SampleLabTests.exe
```

Coprono motore di riproduzione (mapping, velocity, level, start/end, pitch, reverse, one-shot/gate/loop, layer, voice stealing) e formato kit (salvataggio con cartella omonima, ricarica, sample mancanti, versione schema).

## Stato

Fase 1 (MVP sampler) in corso: griglia 4×12 con mapping da C1, browser dei file, drag & drop sui pad, editor di cella (forma d'onda con Start/End, AHDSR, Level, Pan, Tune, Pitch, modalità one-shot/gate/loop, reverse, range note), kit `.slkit` con cartella dei sample omonima, apri/salva/kit recenti, menu di ridimensionamento a percentuali. Mancano: uscite multiple per cella, verifica in Cubase 16, build su macOS.
