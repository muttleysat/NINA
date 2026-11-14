[English](ARCHITECTURE.md) · [Italiano](ARCHITETTURA.md)

# NINA – Technical architecture

Version: 0.1 – refers to `REQUIREMENTS.md` v1.4 (Italian original: `REQUISITI.md`).

## 1. Stack

| Area | Choice |
|---|---|
| Language | C++20 |
| Framework | JUCE 8 (VST3 + Standalone from the same target) |
| Build | CMake (≥ 3.22), JUCE downloaded with FetchContent |
| Platforms | Windows x64 (MSVC 2022), macOS universal (arm64 + x86_64) |
| Time-stretch | Rubber Band (R3) – main; Signalsmith Stretch – alternative |
| Analysis (BPM/transients) | Essentia (aubio as an alternative) |
| Decoding | JUCE (WAV/AIFF/FLAC/OGG) + libsndfile/minimp3 |
| Resampling | libsamplerate / r8brain |
| Kit JSON | nlohmann/json |

Libraries are added per phase (see section 8), not all at once.

## 2. Repository structure

```
NINA/
├── CMakeLists.txt
├── docs/                  REQUIREMENTS.md, ARCHITECTURE.md (and the Italian originals), manual/
├── Source/
│   ├── PluginProcessor.*  audio/MIDI entry point and state
│   ├── PluginEditor.*     main window (adaptive layout)
│   ├── engine/            audio engine: voices, cells, mixer, outputs     (phase 1)
│   ├── dsp/               effects, envelopes, stretch, normalisation      (phases 2-3)
│   ├── sequencer/         patterns, lanes, steps, clock, MIDI/WAV export  (phase 5)
│   ├── kit/               data model, (de)serialisation .slkit            (phase 1)
│   ├── analysis/          BPM, transients, slicing                         (phase 4)
│   └── ui/                graphical components                             (from phase 1)
└── third_party/           libraries not managed via FetchContent
```

## 3. Modules and responsibilities

```
        ┌──────────────┐   MIDI / transport    ┌─────────────────┐
 DAW ──▶│  Processor   │──────────────────────▶│   Sequencer     │
 /host  │ (entry point)│◀──────────────────────│ (lane per cell) │
        └──────┬───────┘   cell triggers       └─────────────────┘
               │ audio blocks
        ┌──────▼───────┐
        │    Engine    │  VoicePool ◀─ Cell[0..N] (sample, mapping, envelope,
        │              │               pitch/stretch, fx chain, output)
        └──────┬───────┘
               │ per output
        ┌──────▼───────┐   ┌─────────────┐
        │ Mixer / Out  │──▶│ Global FX   │──▶ DAW outputs / audio device
        └──────────────┘   └─────────────┘
```

- **Processor**: receives MIDI and blocks, reads BPM and transport position from the host (or from the internal clock in the standalone), dispatches to the Sequencer and the Engine. Persists the state.
- **Engine**: owns the **Cells** (up to 128). Each cell has: a reference to a `SampleData` (shared, immutable buffer), parameters (start/end, AHDSR, pitch, tune, keylock, volume, normalisation, trigger mode, note/range, output), effects chain. The **VoicePool** (64+ voices) allocates voices in real time.
- **Sequencer**: for each added cell, a **Lane** with steps (up to 64), resolution, length, swing. It produces trigger events with sample-accurate micro-timing; it applies the parameter locks as temporary overrides on the cell parameters.
- **Kit**: a single data model (`KitModel`) serialised to `.slkit` (JSON) with a sample folder of the same name.
- **Analysis**: BPM, onset and slicing, run in a background thread at load time.
- **UI**: vector JUCE components with adaptive layout (sec. 6).

## 4. Threads and real time

| Thread | Tasks | Rules |
|---|---|---|
| **Audio** | `processBlock`: sequencer, voices, effects, mix | No allocation, locks or I/O. Only lock-free structures and pointers to immutable buffers |
| **Message/UI** | Interface, menus, drag & drop | Does not touch the audio state directly: writes atomic parameters or puts commands in a queue |
| **Loading** | File decoding, resampling, analysis, normalisation, stretch pre-render | Publishes the result to the audio thread with an atomic pointer swap (`shared_ptr` / RCU) |
| **Export** | Offline bounce WAV/AIFF/FLAC, stems | Uses a copy of the engine, not the audio thread |

UI → audio communication: **atomic parameters** (`AudioProcessorValueTreeState` for the automatable ones) and a **lock-free FIFO** for commands (sample change, pattern update). Audio → UI communication: meters, play position and cell activity through atomics read by a UI timer.

## 5. Data model and kit

```
Kit
 ├─ name, schemaVersion, globalFx[], masterVolume
 ├─ cells[]  { id, row, col, name, color, samplePath(relative), start, end,
 │            envelope{AHDSR}, level, normalize{mode,target,gain,enabled},
 │            pitch, tune, keylock, bpmInfo{bpm,beats,sync}, slices[],
 │            midi{rootNote, lowNote, highNote, vel range}, triggerMode,
 │            outputIndex, fxChain[] }
 ├─ sequencer { patterns[] { lanes[] { cellId, resolution, length, swing,
 │               steps[] { on, vel, prob, ratchet, nudge, mode, pitch, locks{} } }, seed } }
 └─ ui { width, height, zoom }
```

- File `<Kit>.slkit` (JSON) + folder `<Kit>/` with the samples; relative paths, same name (KIT-1).
- Edits are non-destructive: the audio files are never altered.
- `schemaVersion` with migration functions.
- The state in the DAW project contains the reference to the `.slkit` plus the state not yet saved.
- Since version 1.0 the kit file contains only the kit (cells, parameters, master, MIDI learn); sequences (`.slseq`) and song (`.slsong`) have their own files. The DAW project still saves everything together.

## 5.1 Parameters and automation (implemented)

All automatable values are JUCE/VST3 parameters collected in `ParamSet` (`Source/params/`):

- **Stable IDs**: `c00.level`, `c00.filter.cutoff`, `c47.reverb.mix`, `master.level`… (two-digit cell + name). They must not change between versions (AU-4): a test verifies a list of reference IDs.
- **Readable names and groups**: "A1 Filter Cutoff", grouped as *Cell A1 > A1 Filter > ...* and *Master > ...*; the value text includes the unit ("2400 Hz", "Gate", "On").
- **Count**: 38 parameters per cell × 48 cells + 26 for the master = 1,850.
- **Source of truth**: the parameters, not the `Kit`. The audio thread reads them at every block (`ParamSet::fillLive` → `Engine::LiveParams`), so automation also works in offline export. The `Kit` keeps the values only for saving (`writeToKit` before saving, `readFromKit` after loading).
- **Interface**: the knobs use `SliderParameterAttachment` (and the equivalents for buttons and combos): they follow automation and open/close gestures towards the DAW (Write/Touch/Latch). The Start/End markers of the waveform open a gesture when dragged.
- **Non-automatable parts**: samples and note mapping (root/low/high) stay in the snapshot published by the UI thread.
- The sequencer (per-step locks) will write on the same parameters (AU-9).
- **MIDI learn** (`params/MidiLearn`): right click on a control > "MIDI Learn", then move a controller; the CC > parameter assignment is saved in the `.slkit` (`midiMap`) and in the DAW state. The audio thread reads a table of atomics (128 CC × 4 parameters) and moves the parameters, so it also works with the CCs of a MIDI clip in the offline export. Switches: on from 64 upwards. **Not yet**: pitch bend/aftertouch, MIDI channel filter, relative mode, soft takeover, panel listing the mappings.

## 5.2 Per-hit effects and sends (sequencer)

- **Per voice** (`InsertFx`): saturation, LoFi and filter. Each note has its own state; the parameter locks of a step change only that hit.
- **Per cell** (`FxChain`): compressor on the bus, delay and reverb as **sends**: each voice sends an amount (Mix) to the effect, which adds the wet output. The echo of a hit continues after the end of the step (4 s tail from the last send).
- **Lockable per step**: Drive, Sat Mix, Bits, Rate, LoFi Mix, Cutoff, Reso, Delay send, Reverb send. Locking a value turns the effect on for that hit; delay time/feedback, reverb size and compressor stay with the cell.
- **Master**: the same effects on the final mix (delay and reverb with Mix as the send).

## 6. Interface and resizing

- All drawing is **vector** (juce::Graphics, Path, gradients): no fixed bitmaps.
- The layout is computed in `resized()` in proportion to the current size, with minimum/maximum limits for columns and panels (UI-13). The scale factor is derived from the width relative to a reference size (1280×760).
- The window is resizable (`setResizable`, limits 1000×640 → 4K). The **percentage fallback** (75/100/125/150/200% menu) is already present, limited to the screen size (UI-12b).
- Size and zoom are saved in the state (`uiWidth`, `uiHeight`, `uiZoom`).
- The file browser uses `juce::FileTreeComponent` for access to the whole file system, with preview on a separate thread.

## 7. Playback engine per cell

1. **Trigger** (MIDI, sequencer or UI pad) → allocates a voice with velocity, note, mode (one-shot / gate / loop).
2. **Reading**: high-quality interpolation (Hermite/sinc) with start/end, reverse and loop; AHDSR and fade.
3. **Pitch/Tune**: resampling (varispeed) or, with **Key Lock**, stretch at constant pitch.
4. **Loop Sync**: stretch ratio = project BPM / loop BPM; with variable tempo the ratio is updated per block. Rubber Band in real-time mode for loop voices; stretch is avoided for one-shots.
5. **Cell effects chain** (with the parameter locks of the current step) → volume (Level × normalisation × velocity) → pan → **assigned output**.
6. **Mixer**: the assigned outputs go to the buses of the DAW; the rest goes through the global chain and the master.

Normalisation: computed at load/edit of start/end on the loading thread (peak, RMS or LUFS) and saved as a gain in dB.

## 8. Development phases

| Phase | Content | New dependencies |
|---|---|---|
| **0 – Skeleton** (done) | CMake, VST3 + Standalone, resizable window with layout and percentage menus | JUCE |
| **1 – Sampler MVP** (done, except verification in Cubase) | 4×12 grid, browser, sample loading, C1 mapping, voices, start/end, AHDSR, Level, tune/pitch, saving/opening `.slkit` kits, recent kits, multiple outputs. The JSON uses `juce::var` (no additional dependency). | – |
| **2 – Effects** (done, except the points below) | Per-cell and global chain with a fixed order Saturation > LoFi > Filter > EQ > Compressor > Chorus > Flanger > Phaser > Delay > Reverb > Stutter, each with on/off. Delay/reverb tail after the end of the note. BPM-synced delay (DAW or internal, 17 musical values, smoothed time). Multiple outputs (Main + 15 stereo, per cell, pre-master). To do: modifiable order | – (uses `juce_dsp`) |
| **3 – Loops and stretch** (first step done) | Done: BPM estimate from the length, sync to project BPM (DAW or internal), Key Lock on/off, offline background processing with Rubber Band R3 (single-file build, internal FFT and resampler, no external dependency). The engine reads `playData` (original or processed copy); snapshots are republished when processing ends and the old buffers stay alive for the active voices. To do: transient/grid detection, loop phase offset | Rubber Band |
| **4 – Slicing** (first step done) | Done: Slice tab, hit detection (energy flux of the difference, threshold and minimum distance depending on sensitivity, position refined at the zero crossing), equal divisions, editable markers saved in the kit, distribution onto the cells (same file, Start/End per cell) with an overwrite warning. Own implementation (`SliceDetect`), without external libraries. To do: grid alignment, loop tempo from the hits | – |
| **5 – Sequencer** (first step done) | Done: patterns (16 slots), lanes per cell added with the right click, up to 64 steps with length and resolution per lane (including triplets), swing, nudge, ratchet, probability with repeatable seed, gate, per-step pitch, parameter locks on the effects (values and on/off), per-step reverse and mode, FX switch per row ("dry"), mute/solo, transport (DAW or internal), step editor, MIDI export and 24-bit WAV audio bounce. Keyboard recording and quantisation (done). Mixer (console with one channel per cell: fader, peak meters per cell and master, Mute/Solo as automatable parameters, sends, FX on/off, output; done). Pattern Sequencer / Song: chain of patterns (`Song`, `Segment`, `Sequencer::generateSegments`), playback in the engine, MIDI/WAV/rows export of the song and `.slsong` file (done). To do: drag & drop of the clip to the DAW, AIFF/FLAC and stems, lock slides, multiple lane selection, per-step playback modes (reverse, slice) | – |
| **6 – Complete standalone** | Internal transport, audio/MIDI settings, record | – |
| **7 – Polish** | Layers, groups, undo, optimisation, Cubase 16 tests | – |

The standalone target has existed since phase 0 and serves as a quick test bench.

## 9. Testing and quality

- **Cubase 16** as the reference DAW; check with the Steinberg VST3 validator.
- Automated tests (JUCE `UnitTest` or Catch2) on: kit serialisation, note mapping, normalisation calculation, quantisation, stretch ratio.
- Real-time verification: no allocation in the audio thread (debug check tool).

## 10. Build

```powershell
# Windows (Visual Studio 2022 Build Tools)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# Output: build/Nina_artefacts/Release/{VST3,Standalone}
```

```bash
# macOS
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

The VST3 is copied to `C:\Program Files\Common Files\VST3` (Windows) or `~/Library/Audio/Plug-Ins/VST3` (macOS) to be seen by Cubase. On Windows the installer (`installer/`) does this.
