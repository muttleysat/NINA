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

## Stato

Fase 0: scheletro con finestra ridimensionabile e layout adattivo (browser, griglia 4×12, pannello inferiore) e menu di ridimensionamento a percentuali. Nessun audio ancora.
