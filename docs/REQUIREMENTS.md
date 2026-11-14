[English](REQUIREMENTS.md) · [Italiano](REQUISITI.md)

# NINA – Product requirements

Version: 1.6 (draft) – 2026-11-12
Status: living document, to be extended iteratively.

> **Notes for version 1.0 (English edition).** This is a translation of the Italian original (`REQUISITI.md`), written while the project was still a personal one. Since then: NINA is released as free software under the AGPL-3.0 (see section 12); the standalone always starts empty instead of restoring the last kit (SA-5); the kit file contains only the kit, with sequences (`.slseq`) and song (`.slsong`) in their own files (SQ-9, KIT chapter); the user interface is in English (NF-7). Everything else below is the original text.

## 1. Vision

**NINA** (Drumbox Sequencer Powerhouse) is a **VST3** plug-in of the drum sampler/sample player kind, inspired by Native Instruments Battery. It lets you load single samples or loops onto a grid of cells, map them to a MIDI keyboard, edit them non-destructively (waveform, dynamics, tune, time-stretch) and process them with per-cell and global effects. Kits are saved as a `.slkit` file linked to a folder with the samples, which remain normal audio files that can be recovered directly.

## 2. Scope of v1

**In**: VST3 format **and standalone application**, **Windows and macOS** (macOS Intel + Apple Silicon, universal build), personal use, everything described in points 4–10.
**Out (for now)**: AU/AAX, Linux, commercial distribution, bundled kit/sample library, synthesis, audio recording, exchange of kits with third-party formats (e.g. Battery `.kt`).

## 3. Glossary

- **Cell**: slot of the grid that contains a sample and its parameters.
- **Kit**: set of cells + parameters + global effect + associated samples.
- **Loop**: sample with a musical length (bars/beats), synchronisable to the project tempo.
- **One-shot**: single non-rhythmic sample.

## 4. Graphical interface

Inspired by the organisation of Battery, but with original graphics and assets (no copy of NI resources or trademarks).

| ID | Requirement | Priority |
|----|-------------|----------|
| UI-1 | Single resizable window with three areas: **browser** on the left, **cell grid** at the top/centre, **editing panel** at the bottom. | Must |
| UI-2 | The browser gives access to the whole file system (drives, folders, PC root), with folder tree, file list, audio extension filter, search by name. | Must |
| UI-3 | Audition (preview) of the file selected in the browser, with adjustable volume and, for loops, an option for preview synchronised to the DAW tempo. | Must |
| UI-4 | Loading into a cell by drag & drop from the browser (and from Windows Explorer) or double click on the selected cell. | Must |
| UI-5 | Default cell grid of **4 rows (A–D) × 12 columns = 48 cells**, with the possibility to **add/remove rows and cells** (rows labelled A, B, C…; columns numbered). Each cell shows name, assignable colour, activity indicator and mute/solo. | Must |
| UI-9 | Reference layout (Battery screenshot): top bar with kit name, BPM indicator/field and Sync; left column with *Library / Files* tabs (and category tags for the samples); under the grid, the **waveform** with S/E markers and quick controls (Tune, Reverse, Pan, Level, Key Range, layer selection); at the bottom the **effects chain** as side-by-side modules; lower tabs (Main, Effects, Modulation, Setup, Editor, Master) for the different panels. To be rethought with original graphics. | Should |
| UI-11 | **Context menu (right click) on the cell**: Add/Remove from sequencer, Rename, Colour, Assign MIDI note (MIDI learn), Assign output, Duplicate, Clear cell, Show file in browser. | Must |
| UI-12 | **Resizable window** both in the VST3 plug-in and in the standalone, by dragging corners/edges (and, in the standalone, also with maximise/full screen). Minimum size (proposed 1000×640 px) and maximum (up to full screen), free proportions with minimum constraints. In the VST3 resizing goes through the native support of the DAW (verified in Cubase 16). Size and position are remembered between sessions and saved with the DAW project. | Must |
| UI-12b | **Percentage resizing fallback**: if the DAW (Cubase or others) does not allow free resizing of the plug-in window, or handles it badly, the plug-in offers a **Window size** control (menu in the top bar and shortcut) with predefined choices as a percentage of the base size: e.g. **75%, 100%, 125%, 150%, 200%** (plus any custom values). Changing the percentage resizes the window and scales the whole interface as a block, with the same layout adaptations as UI-13 (no clipping, proportional texts/controls). The chosen size is remembered and saved with the project. Percentages are limited to those that fit on the screen, with warning/automatic reduction. Also available in the standalone. | Must |
| UI-13 | **Adaptive layout**: when the window changes size all elements adapt accordingly, with no clipping or overlap: the cell grid is redrawn (larger or smaller cells, keeping the 12 columns), the waveform and the sequencer take the available space, the effects panels reorganise or compact, the browser on the left and the other columns have proportional width with min/max limits. Texts, knobs and icons scale together. At reduced sizes the less used panels become tabs or compress. | Must |
| UI-14 | **Scalability and sharpness**: vector graphics (no fixed bitmaps) to stay sharp at every size and pixel density (HiDPI/Retina, 100–300% scale); interface zoom factor adjustable (e.g. 75%–200%) independent of the window size. | Must |
| UI-15 | **Efficient structure**: clear organisation in fixed areas (browser, grid, editor/sequencer/effects as tabs, top bar), the most used functions one click away, no deeply nested menus; current state always visible (selected cell, kit, BPM, playing pattern). Ability to hide/show the browser and to enlarge the active panel (editor, sequencer) to full space. | Must |
| UI-16 | **Accessibility and fast use**: keyboard shortcuts for the main actions (play/stop, tab change, waveform zoom, undo/redo), tooltips on all controls, double click to reset values, editing values with the wheel and direct numeric entry, multiple selection of cells and steps with drag and Shift/Ctrl. | Should |
| UI-17 | **Attractive graphics**: consistent dark theme with a controlled palette and accent colours per cell/category, polished knobs and faders, waveform with gradient and light shading, short smooth animations (cell triggers, meters), consistent icons, typography readable at every size. Option for a light/alternative theme as a future extension. | Should |
| UI-18 | **UI performance**: smooth redraw (60 fps reference) even with 48+ cells and the sequencer active, even during resizing; rendering with GPU acceleration when available (Direct2D on Windows, Metal on macOS), without affecting the audio thread. | Should |
| UI-10 | Colours per cell assignable manually and per category (kick, snare, hat, etc.). | Could |
| UI-6 | Selecting a cell, the editing panel shows the parameters of **that** cell in the same window (no separate window). | Must |
| UI-7 | "Kit/Global" selection to work on the effects and parameters of the whole kit. | Must |
| UI-8 | UI scalability (HiDPI / zoom) and dark theme. | Should |

## 5. Sample management and MIDI mapping

| ID | Requirement | Priority |
|----|-------------|----------|
| MAP-0 | **Default automatic mapping**: the first cell (A1) is assigned to **C1** (MIDI 36, C3 = 60 convention used by Cubase) and each following cell to the next note, in horizontal order by row (A1…A12, B1…B12, …). With the 4×12 grid C1–B4 (notes 36–83) are covered. Adding cells the mapping continues; the starting value can be changed globally. | Must |
| MAP-1 | Each cell has a **root key** and a **key range** (low–high note). By default the range coincides with the single automatically assigned note; the user can change the note (or the range) of a cell **manually** at any time. With a range of several notes the sample is transposed relative to the root (melodic use). Manual cells are not touched if the automatic mapping is applied again. | Must |
| MAP-2 | Quick assignment with **MIDI learn**: press a key on the keyboard to assign the cell. | Must |
| MAP-3 | **Velocity** range per cell and velocity sensitivity (volume, optional filter). | Should |
| MAP-4 | Several cells on the same note (layers) and **exclusion groups** (e.g. closed/open hi-hat, choke). | Should |
| MAP-5 | Polyphonic / monophonic mode per cell. | Must |
| MAP-6 | Supported import formats: WAV, AIFF, FLAC, MP3, OGG (WAV/AIFF 16/24/32 float, up to 192 kHz). Automatic resampling to the project sample rate. | Must |

### 5.1 MIDI learn of the parameters

Every controllable parameter (knobs, faders, switches) can be assigned to a control of a MIDI device (knob, fader, button of a controller), in addition to normal DAW automation.

| ID | Requirement | Priority |
|----|-------------|----------|
| ML-1 | **Assignment with MIDI learn**: right click on a parameter, "MIDI Learn" item; the parameter enters listening mode (highlighted), a control on the controller is moved and the assignment is created on the first message received (CC, pitch bend, channel aftertouch). Can be cancelled with Esc or a new click. | Must |
| ML-2 | Items of the context menu of the parameter: **MIDI Learn**, **Clear MIDI mapping**, and the indication of the current assignment (e.g. "CC 74, ch. 1"). Assigned parameters have a visible indicator on the control. | Must |
| ML-3 | The value of the parameter follows the MIDI control over the whole range (0–127 mapped to min–max of the parameter, respecting the logarithmic scale/skew). Switches are activated with values ≥ 64 or as a toggle button with an option. | Must |
| ML-4 | **Control modes**: absolute (default) and relative (endless encoders: sign and two's complement, binary offset). Option per parameter. | Should |
| ML-5 | **Soft takeover (pickup)**: after a change of cell or kit, the parameter does not jump to the value of the controller until it crosses the current value, to avoid abrupt jumps. Optional. | Should |
| ML-6 | **Parameters of the selected cell**: "per selected cell" assignment (the controller knob acts on the parameters of the currently selected cell, e.g. Level, Tune, Cutoff) or "fixed on a cell" (always acts on the chosen cell). | Should |
| ML-7 | **"MIDI Mappings" panel**: list of all the assignments (parameter, control, channel, mode) with editing and deletion, "Clear all" and filter by channel/device. | Should |
| ML-8 | **Saving**: assignments are saved in the `.slkit` kit, and can be exported/imported as an independent **mapping preset** (to reuse the same controller with other kits). In addition, a global application mapping remembered between sessions. | Must |
| ML-9 | **Use in the plug-in**: MIDI messages come from the DAW; in Cubase 16 CCs reach the plug-in as MIDI events if the track receives from a MIDI device, to be verified with a prototype. Alternatively the assignment goes through the automation/MIDI mapping system of the DAW (parameters exposed with stable IDs, NF-6). **In the standalone** messages come directly from the selected MIDI devices (SA-2). | Must |
| ML-10 | The same MIDI message can control several parameters; note messages (used for cell mapping, MAP-0/MAP-2) stay separate from the MIDI learn of the parameters. Feedback to the controller (LEDs/motorised) is out of scope for v1. | Could |
| ML-11 | Using MIDI learn must not cause clicks: received values are smoothed and applied safely with respect to the audio thread. | Must |

## 6. Sample editing (non-destructive)

The original on disk is never modified; the edits are parameters saved in the kit.

| ID | Requirement | Priority |
|----|-------------|----------|
| ED-1 | Waveform display with zoom and scroll, stereo/mono. | Must |
| ED-2 | **Start and End** adjustable with draggable markers on the waveform; optional snap to zero crossings. | Must |
| ED-3 | Optional loop point (sustain loop), with crossfade. | Could |
| ED-4 | **Dynamics**: amplitude envelope **AHDSR** (attack, hold, decay, sustain, release), pan, fade-in/fade-out, phase inversion. Volume and normalisation: see ED-8…ED-12. | Must |
| ED-5 | **Reverse** (backwards playback). | Should |
| ED-6 | Compressor/transient shaper per cell (part of the effects chain, sec. 7). | Should |
| ED-7 | Undo/redo on parameter changes. | Should |
| ED-8 | **Volume per cell** (Level): fader/knob for each cell, from -∞ to +12 dB, with numeric value in dB, always visible in the cell panel (like the Battery Level) and adjustable with a quick gesture also from the cell in the grid (e.g. mouse wheel). Double click to return to 0 dB. Automatable by the DAW. | Must |
| ED-9 | **Metering**: level (peak) indicator for each cell during playback and meters on the master and on the outputs, with clip indication. | Should |
| ED-10 | **Non-destructive normalisation**: calculates a compensation gain for each sample, stored in the kit (`normalizeGain`) and applied in addition to the Level; the audio file is never modified. The gain is recalculated when start/end change. | Must |
| ED-11 | **Measurement modes** selectable per cell and for the kit: **Peak** (brings the maximum peak to a target value, e.g. -1 dBFS), **RMS** (average level of the audible part between start/end, suited to drum hits) and **Loudness LUFS** (for loops and long samples, ≥ 1 s). The target is adjustable (default: -1 dBFS peak, -18 dB RMS, -14 LUFS). With very short samples the LUFS mode is disabled or replaced by RMS. | Must |
| ED-12 | **Normalise the whole kit**: a command that applies normalisation to all the cells with a sample (or to the selected ones) to make them sound at the same perceived volume, with a preview of the gain calculated per cell and the possibility to undo with a single undo. Option to **automatically normalise every new sample** at load. A **Normalize** on/off switch per cell allows excluding a sample (e.g. a deliberately quieter hat). | Must |
| ED-13 | **Safety limit**: the normalisation gain is limited (e.g. max +24 dB) with a warning for almost silent samples, and the resulting peak cannot exceed 0 dBFS without a limiter on the cell. The volume per cell is added, in dB, to the normalisation, the velocity and the volume of the kit. | Should |

## 7. Effects

Effects are available **per cell** and at the **global (kit)** level, with the same interface, bypass and dry/wet.

| ID | Requirement | Priority |
|----|-------------|----------|
| FX-1 | Architecture: effects chain per cell → kit mixer → global chain → output. | Must |
| FX-2 | **Standard**: parametric EQ, filter (LP/HP/BP), compressor, limiter, saturation/drive, delay, reverb. | Must |
| FX-3 | **Creative**: bit crusher/sample-rate reducer, distortion/waveshaper, chorus, flanger, phaser, tape stop, stutter/gater, ring modulator. (Initial list, to be refined.) | Should |
| FX-4 | Order of the effects in the chain editable; number of slots per cell (proposal: 4) and global (proposal: 6). | Should |
| FX-5 | Send/aux for reverb and delay shared between cells. | Could |
| FX-8 | **Tempo sync of rhythmic effects**: effects with a time or a frequency (delay, stutter/gater, tremolo, auto-pan, chorus/flanger/phaser LFO, filter with LFO) have a **Free / Sync** switch. In Free the value is in ms or Hz; in Sync a musical value is chosen: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, with dotted (.) and triplet (T) variants. The tempo is the **BPM of the DAW** (also with variable tempo) or, in the standalone, the internal BPM (SA-3). | Must |
| FX-9 | **Phase synchronisation**: LFOs, stutter and gater lock to the transport position (bar/beat) and restart aligned at every Play or position jump; "Retrigger on note" option to restart at every note. A tempo change produces no clicks (smoothing). In the delay, **Tape** mode (a tempo change also changes the pitch) or **Clean** (no pitch variation). | Should |
| FX-6 | **Multiple outputs**: each cell is assignable to an output of the DAW (main stereo + additional outputs, proposal 16 stereo). The assignment is saved in the kit. Per-cell effects are applied before the assigned output; the global chain acts only on the cells routed to the master. | Must |

## 8. Loop, sync and time-stretch

| ID | Requirement | Priority |
|----|-------------|----------|
| LP-1 | For each cell, **Loop Sync** mode: the sample is stretched to fit the **project BPM** read from the DAW (variable tempo included). | Must |
| LP-2 | Definition of the musical length of the loop (number of beats/bars) with automatic BPM detection and manual correction. | Must |
| LP-3 | Quantised loop start (none / beat / bar) and alignment to the DAW transport position. | Should |
| LP-4 | Time-stretch modes with selectable quality (e.g. rhythmic/percussive vs. tonal). | Must |
| LP-5 | Sync can be disabled: the loop can play at the original speed (or at variable speed with the pitch) without sync. | Must |
| LP-6 | Slicing of loops: see section 8.1. | Must |

### 8.1 Loop slicing

| ID | Requirement | Priority |
|----|-------------|----------|
| SL-1 | **Automatic slices** on a loop: by transient detection (with adjustable sensitivity) or by grid (division into N equal parts / rhythmic value: 1/4, 1/8, 1/16…). | Must |
| SL-2 | **Manual slices**: add, move and delete markers on the waveform, with snap to zero crossing or to the rhythmic grid. Automatic slices can then be edited by hand. | Must |
| SL-3 | **Distribution onto the cells**: a "Distribute slices" function that creates one cell per slice, **in horizontal order**, starting from a chosen destination cell, proceeding along the row (A1→A12) and moving on to the next (B1…), so as to occupy consecutive MIDI notes. Each cell receives start/end, name (e.g. `Loop_01`), and refers to the same source file (no copy of the sample). | Must |
| SL-4 | Distribution options: starting cell, overwrite or skip occupied cells, warning/addition of rows if the cells are not enough, automatic assignment of consecutive MIDI notes, and trigger mode (one-shot/gate) and output for the cells created. | Should |
| SL-5 | Cells created from slices keep a link to the original loop: if the loop is resynchronised, the slices follow the project tempo. | Should |
| SL-6 | Alternative "slice on a single cell" mode: the loop stays in one cell and the slices are played by consecutive MIDI notes (MPC Chop style). | Could |
| SL-7 | Ability to undo the whole distribution with a single undo. | Should |

## 9. Tune, pitch and playback modes

| ID | Requirement | Priority |
|----|-------------|----------|
| PT-1 | **Tune** (fine tuning, ±100 cents) and **Pitch** (semitones, ±24) per cell. | Must |
| PT-2 | **Pitch / time-stretch relationship** (behaviour like VirtualDJ *Key Lock / Master Tempo*): when a sample is synchronised to the project tempo, the time-stretch adapts the duration **preserving the original pitch**, without distorting the timbre. The perceived pitch stays the one set by the user's Tune/Pitch. | Must |
| PT-2b | **Key Lock** on/off selector per cell: with Key Lock off the playback speed is coupled to the pitch (varispeed, like a turntable); with on (default for synchronised loops) the pitch is independent of the tempo. | Should |
| PT-2c | Recommended stretch limits to maintain quality (e.g. typical ±50% / ±8 BPM); beyond, a warning or choice of the most suitable stretch engine. | Could |
| PT-3 | MIDI pitch bend and keytracking (the played note transposed relative to the root) switchable per cell. | Should |
| PT-4 | **Akai MPC style trigger modes**, selectable per cell and valid for both one-shots and loops: | Must |
| | • **Note On (one-shot)**: the sample plays to the end, Note Off is ignored. | |
| | • **Note Off / gate**: the sample plays only while the key is pressed; on release the release phase of the envelope starts. | |
| | • **Loop**: the sample repeats while the key is pressed. | |
| PT-5 | Round-robin/alternation and light randomisation (pitch, velocity) per cell. | Could |

### 9.1 Step sequencer (up to 64 steps)

Internal sequencer that plays the cells of the kit and allows creating a **MIDI phrase** to bring into the DAW.

| ID | Requirement | Priority |
|----|-------------|----------|
| SQ-1 | Patterns of up to **64 steps**; adjustable length (1–64), rhythmic division per lane (SQ-14), swing. The sequencer has **one row (lane) per cell**, each with its own 64 steps and its own length (polyrhythms). Only the **cells added by the user** appear in the sequencer (SQ-12). | Must |
| SQ-2 | Synchronised to the **transport and tempo of the DAW** (play/stop/position, variable tempo), with the option to leave the pattern in free play. Multiple patterns (at least 16 slots per kit), also selectable from MIDI notes. | Must |
| SQ-3 | Parameters per step: **on/off, velocity, note/cell, duration (gate), probability, ratchet/repeats, micro-timing (nudge)**. | Must |
| SQ-4 | **Playback modes per step**: one-shot / gate, reverse, slice selection (for sliced loops), step pitch/tune, start offset. | Must |
| SQ-5 | **Effects per step** (*parameter lock*, Elektron style): each step can override the effects parameters of the cell (e.g. filter, drive, bit crusher, delay send) only for that step; the unlocked values stay those of the cell. | Must |
| SQ-5b | **Different levels of the same parameter between steps and between cells**: each cell has its own independent filter (and effects). A parameter such as the cutoff can have a different value at every step of the same lane, and different values in different lanes (e.g. kick with cutoff 400 Hz, snare with cutoff 3 kHz, hat with cutoff 9 kHz). The value between one step and the next is stepped or interpolated ("Slide" option, with glide time adjustable per step). | Must |
| SQ-5c | **Editing several lanes together**: selecting several cells/lanes, a parameter (e.g. cutoff) can be set or modulated for all with one gesture (absolute value, relative offset or ramp from one cell to another), and a lock can be copied onto other steps and other lanes. Bar view of the chosen parameter for all lanes. | Should |
| SQ-6 | Grid editor in the same window (tabbed panel), with quick editing of velocity/probability, copy/paste of steps and patterns, randomisation and clear. | Must |
| SQ-7 | **Export of a MIDI phrase to the DAW**: (a) **drag & drop** of the pattern from the plug-in to the Cubase track or project area as a MIDI clip (`.mid` file generated on the fly); (b) export to a `.mid` file; (c) *optional, not planned*: real-time MIDI output of the plug-in to be routed to another track, to be tried in Cubase 16 only if useful. The export keeps notes, velocity, duration and probability/ratchet "resolved" into fixed MIDI events. | Must |
| SQ-8 | The sequencer drives the cells directly (not only via internal MIDI); incoming MIDI notes from the DAW keep working in parallel. Possible "record from keyboard" mode (step recording) with quantisation. | Should |
| SQ-9 | Patterns saved in the `.slkit` kit together with the rest of the parameters (since v1.0: in their own `.slseq` file). | Must |
| SQ-10 | **Export audio (Bounce pattern)**: from the sequencer, an "Export WAV" command that generates an audio file of the pattern **complete with everything the sequencer does** (reverse, effect parameter locks, pitch, ratchet, probability, cell and global effects). The file is then imported in Cubase like any audio, next to the exported MIDI clip (SQ-7). | Must |
| SQ-10a | Export options: number of repetitions of the pattern (1, 2, 4… or length in bars), **final tail** for reverb/delay (automatic or in seconds), **WAV** format at 16/24 bit or 32 float (alternatively **AIFF** or **FLAC**), sample rate (that of the project or chosen), optional normalisation. | Must |
| SQ-10b | **Tempo**: the pattern is rendered at the current BPM of the DAW or at a manually chosen BPM. The BPM and the number of bars are written in the file name (e.g. `KitName_Pattern01_120bpm_4bar.wav`) and, if supported, in the WAV metadata. | Must |
| SQ-10c | **Stems**: besides the complete stereo mix, an option to export one file per cell or assigned output (kick, snare, hat…), all of the same length to align in Cubase. | Should |
| SQ-10d | Rendering happens **offline**, faster than real time, without having to play the DAW, with a progress bar and the possibility to cancel. | Should |
| SQ-10e | Save location chosen by the user; by default the `Bounces/` folder next to the `.slkit` file of the kit. Default name: kit name + pattern name. Drag & drop of the generated WAV directly onto a Cubase track, as for the MIDI clip. | Should |
| SQ-12 | **Context menu (right click) on the cell** in the grid: **"Add to sequencer"** item (becomes "Remove from sequencer" if the cell is already present). The sequencer thus contains only the cells chosen by the user, cell by cell, in the order in which they are added. Removing a row asks for confirmation if it contains active steps; the pattern of the row is kept until it is explicitly cleared. | Must |
| SQ-13 | Management of many rows: vertical scroll, adjustable row height, reordering by dragging (without changing the MIDI note of the cell), mute/solo per row, pages of 16 steps with horizontal scroll. Clicking a cell in the grid, the sequencer brings its row to the foreground. Single-row detail view with velocity and probability bars. Empty rows take no space in the saved kit. | Must |
| SQ-14 | **Resolution/quantisation per lane**: each row has its own rhythmic division, chosen among 1/4, 1/8, 1/16, 1/32, 1/64 and the respective triplets (1/8T, 1/16T, 1/32T) and quintuplets. The steps of the row last that division, so 64 steps at 1/16 cover 4 bars and at 1/32 cover 2. Rows with different resolutions can coexist in the same pattern (e.g. kick at 1/16, hat at 1/32) and stay aligned to the tempo of the DAW. Changing the resolution of a row with steps already entered, you can choose whether to **keep the musical position** (the steps move to stay at the same point in time) or **keep the indices** of the steps. | Must |
| SQ-15 | **Step quantisation**: a "Quantize" function per lane (or for the selected rows) that brings the steps and the micro-timing (nudge, keyboard recording) onto the chosen grid, with **strength** from 0 to 100% (partial quantisation) and "keep swing" option. Automatic real-time quantisation during step recording (SQ-8), with selectable grid. | Should |
| SQ-16 | **Swing per lane** in addition to the global one, with adjustable value per row. | Could |
| SQ-11 | **Random seed per pattern**: probability, randomisation and random ratchets use a seed stored in the pattern, so that playback and export are repeatable and identical. Option to change the seed or leave it random at every play. | Should |

*Feasibility note*: a VST3 plug-in cannot write directly onto the timeline of the DAW; for this reason the export goes through drag & drop of MIDI files or through MIDI output. MIDI cannot represent reverse and per-step effects, which are instead included in the WAV audio export (SQ-10).

### 9.2 Standalone version (Windows and macOS)

In addition to the VST3, NINA is distributed as a **stand-alone application** for Windows and macOS, with the same audio engine, the same interface and the same kits as the plug-in. Useful to use the sequencer and the sampler without a DAW and to prepare patterns and kits to bring later into Cubase.

| ID | Requirement | Priority |
|----|-------------|----------|
| SA-1 | A single code base produces **VST3 and standalone** (separate targets of the same project), with identical interface and functions. Native application for Windows (x64) and macOS (Intel + Apple Silicon). | Must |
| SA-2 | **Audio/MIDI settings**: choice of audio driver and device (WASAPI/ASIO on Windows, CoreAudio on Mac), sample rate, buffer size, assignment of the device outputs to the cell outputs (FX-6), selection of MIDI input devices (keyboard/pads) with MIDI learn. | Must |
| SA-3 | **Internal transport** (without a DAW): Play/Stop, **adjustable BPM**, time signature, tap tempo, optional metronome. The sequencer, the synchronised loops (LP-1) and the slices use this tempo instead of that of the DAW. | Must |
| SA-4 | Optional **external sync**: reception of **MIDI Clock** (start/stop/tempo) from a device or from another app; sending MIDI Clock as a possible extension. | Could |
| SA-5 | Opening, saving and recent kits (KIT-8, KIT-9) identical to the plug-in. Same `.slkit` format, so the kits are interchangeable between standalone and VST3. At startup, restore of the last kit and of the audio settings (since v1.0 the standalone starts empty and only the audio settings and recent kits are kept). | Must |
| SA-6 | All exports (MIDI clip, WAV, stems, SQ-7 and SQ-10) also available in standalone; drag & drop onto a DAW works as for the plug-in, where supported by the system. | Must |
| SA-7 | Recording of the output (live audio **Record**) to a WAV, in addition to the offline Bounce of the pattern. | Should |
| SA-8 | **Ableton Link** for tempo synchronisation with other apps and DAWs on the same network. | Could |
| SA-9 | Safe handling of the audio device: warning if it is missing or changes, no crash on disconnection, automatic restore of the settings. | Must |
| SA-10 | Application menu (File, Edit, Options) native on Windows and macOS, with keyboard shortcuts (open/save kit, play/stop with space bar, undo/redo). | Should |

## 10. Kit: saving and loading

Format: a **`.slkit` kit file** (readable JSON text) linked to a normal **sample folder**, from which the samples can be recovered directly with Windows Explorer/Finder. The file and the folder sit side by side:

```
Kits/
├── MyKit.slkit             # JSON: schema version, name, cells, mapping, parameters, effects, tempos, reference to the folder
└── MyKit/                  # folder with the same name as the kit: copied samples (original audio files, openable anywhere)
    ├── Kick Funk 7.wav
    ├── Snare Funk 1.wav
    └── ...
```

| ID | Requirement | Priority |
|----|-------------|----------|
| KIT-1 | On saving, the user gives a **name to the kit** in the application; both the file and the folder take **exactly that name** (`<KitName>.slkit` and folder `<KitName>/`, side by side in the same location). The samples used are **copied into the folder**, as original audio files, neither renamed nor converted. Characters not valid for the file system are replaced; if name or folder already exist, a request to confirm overwriting or to choose another name. | Must |
| KIT-1b | Renaming the kit with "Save as", file and folder are created with the new name; the kit name shown in the top bar always matches that of the file. | Must |
| KIT-2 | Versioned `kit.json` (`schemaVersion`) with migration from earlier versions. | Must |
| KIT-3 | The `.slkit` file stores the samples with **paths relative** to the kit folder (Windows/macOS compatible). The kit is also saved in the state of the plug-in in the DAW (project recall) with the same reference. | Must |
| KIT-3b | Ability to choose a different sample folder (e.g. shared by several kits) and to **save the kit as a copy** in a new location, with copy or link of the samples. | Should |
| KIT-4 | Duplicate samples (same content) copied only once (hash). File names in conflict resolved with a numeric suffix. | Should |
| KIT-5 | Handling of missing samples or a moved folder: warning, automatic search next to the `.slkit` file and ability to relocate the folder. | Must |
| KIT-8 | **Open / Save / Save as / New kit** from the plug-in menu (top bar, next to the kit name), with a `.slkit` file selection window. Warning if there are unsaved changes before opening another kit. | Must |
| KIT-9 | **Recent kits**: a drop-down menu (in the kit name at the top) lists the **last kits opened or saved** (last 5, most recent at the top), with the kit name and the path on mouse hover. Selecting an item loads the kit. Kits no longer present are flagged and removable from the list; "Clear recent" is available. The list is stored in the user settings and shared between all instances of the plug-in. | Must |
| KIT-7 | Since the samples are independent files, the edits (start/end, effects, etc.) stay **non-destructive** in the `.slkit` and never alter the audio files. | Must |
| KIT-6 | Saving/loading of a **single cell** (cell preset). | Could |

## 11. Non-functional requirements

| ID | Requirement |
|----|-------------|
| NF-1 | Negligible latency: no allocation/lock/IO in the audio thread; samples loaded in RAM (disk streaming for long samples as a future extension). |
| NF-2 | Minimum polyphony 64 voices in total; moderate CPU with 16 active cells and effects. |
| NF-3 | Internal 32-bit float audio processing; support for 44.1–192 kHz and variable buffers. |
| NF-4 | Stability: no crash on corrupted or unsupported files; loading in a separate thread. |
| NF-5 | Compatibility with the main DAWs on Windows and macOS (reference DAW: **Cubase 16** (Windows and macOS); secondary checks on Reaper, Ableton Live, FL Studio; Logic Pro does not load VST3 and is excluded for now). Validation with the Steinberg VST3 validator. |
| NF-8 | Identical `.slkit` kits, interchangeable between Windows and macOS (relative paths, endianness and file name encoding handled). |
| NF-6 | All parameters automatable by the DAW (with stable IDs). |
| NF-7 | UI language: English (Italian as a possible future translation). |

### 11.1 Automation and parameters exposed to the DAW

| ID | Requirement | Priority |
|----|-------------|----------|
| AU-1 | **All controls are parameters that the DAW can automate**, for reading and writing (Read, Write, Touch, Latch modes in Cubase): for each cell Start, End, Level, Pan, Tune, Pitch, Reverse, trigger mode, AHDSR envelope, all the effects parameters with their on/off switches; plus the parameters of the master and of the global effects. Values changed from the interface are recorded by the DAW, those automated are seen moving on the interface. | Must |
| AU-2 | **Recognisable names, not CC numbers**: each parameter has a clear name, a unit and a readable value (e.g. "Cutoff - 2.4 kHz", "Mode - Gate", "Level - -6 dB"). In the Cubase automation list they appear as "A1 Filter Cutoff" and not as "CC 45". | Must |
| AU-3 | **Group hierarchy**: the parameters are grouped by cell and effect (e.g. *Cell A1 > Filter > Cutoff*, *Master > Reverb > Mix*) through VST3 parameter units/groups, so as to be ordered and searchable in the menu of the DAW. | Must |
| AU-4 | **Stable and backward-compatible IDs**: the identifier of each parameter derives from the cell and the name of the parameter and does not change between versions. Parameters added in the future do not alter the existing ones, so as not to break projects already saved with automation. | Must |
| AU-5 | **Cell coverage**: the parameters are exposed for all the 48 cells of the grid (4×12), 38 per cell plus 26 for the master, about 1,850 in total, organised in groups so as not to weigh down the menu of the DAW. The cell limit is fixed at 48 until it is possible to add rows; for a larger number this choice will have to be reviewed. | Must |
| AU-6 | **Correct gestures**: every change from the interface signals the start and end of the gesture (touch/release) to the DAW, so that Write, Touch and Latch record correctly. Automated values are smoothed over time to avoid clicks. | Must |
| AU-7 | **Discrete parameters with text**: switches and selectors (filter type, trigger mode, sync on/off, sync value 1/16, etc.) have readable text values in the DAW. | Must |
| AU-8 | **"Selected cell" parameters** (optional): a small set of generic parameters (e.g. Level, Tune, Cutoff) that always act on the selected cell, to control it quickly from a controller or from the Cubase Quick Controls. The recorded automation on these depends on the selection and is flagged as such. | Could |
| AU-9 | **Interaction with the sequencer**: the value automated by the DAW is the base value of the parameter; the sequencer parameter locks (SQ-5) override it only in the step in which they are active. The priority order is documented and visible (indicator on the control with an active lock). | Must |
| AU-10 | **Link with MIDI learn** (sec. 5.1): MIDI learn assigns the controllers to the same parameters and goes through the same system, so it also works in the standalone (where DAW automation does not exist). | Must |
| AU-11 | **Verification in Cubase 16**: the parameters appear by name in the automation list and in the Quick Controls, are recorded and played back in all modes; also verification with a second DAW. | Must |

## 12. Technical proposal (to be confirmed)

- **Language and framework**: C++17/20 with **JUCE** (VST3, GUI, audio/MIDI/file handling). Build with CMake.
- **Principle on libraries**: *original text: personal use and never commercialised, so the best available libraries are chosen regardless of licence (GPL/AGPL included); paid/proprietary libraries that require purchase (e.g. zplane élastique) remain excluded.* **Update (v1.0): NINA is now itself released under the AGPL-3.0 and may be redistributed; the libraries used are GPL/AGPL-compatible (see `THIRD_PARTY_NOTICES.md`).**
- **Time-stretch/pitch**: **Rubber Band** (R3/"Finer" engine, the best open source quality, with real-time mode and rhythmic/tonal preset) as the main engine; **Signalsmith Stretch** as a light alternative for A/B comparison and for very short slices.
- **Audio analysis (BPM, beat, transients)**: **Essentia** (AGPL, state-of-the-art beat tracking and onset detection) for automatic detection of BPM and slices; **aubio** as a lighter alternative.
- **File decoding**: JUCE audio modules (WAV, AIFF, FLAC, OGG) plus **libsndfile**/**minimp3** for MP3 and additional formats, uniform on Windows and macOS.
- **Effects DSP**: `juce_dsp` module for filters/EQ/compressor, plus high-quality reference implementations for reverb (e.g. Dattorro/Zita), saturation with oversampling and ladder filters (e.g. Moog/TPT models); case-by-case choice in the effects development phase.
- **Resampling**: **libsamplerate** (Secret Rabbit Code) or r8brain-free-src for top quality sample-rate conversion.
- **Kit serialisation**: JSON (nlohmann/json or `juce::var`).
- **Multi-platform build**: CMake + CI (e.g. GitHub Actions) with Windows and macOS runners; the Mac build requires macOS (or CI) and, for local use, ad-hoc signing of the VST3 bundle.
- **Slicing**: transient detection with onset detection (spectral flux) and beat tracking for the BPM; candidate libraries: aubio, or own implementation.
- **Multiple outputs**: multiple audio buses in JUCE (AudioProcessor with several output buses); beware of the variable support between DAWs.
- **Trademark**: "Battery" and its look belong to Native Instruments; the concept and layout are an inspiration, but name, graphics and assets will be original.

## 13. Open questions

**Resolved (v0.3)**: pitch with VirtualDJ-style Key Lock (PT-2); 4×12 grid expandable up to **128 cells** (UI-5); personal use; Windows + macOS; outputs per cell (FX-6); slicing in v1 with horizontal distribution per row (sec. 8.1); automatic mapping from C1 one note per cell, with manual override (MAP-0/MAP-1); AU/Logic excluded; reference DAW Cubase 16.

Kit format decided: `.slkit` file (JSON) + linked sample folder (sec. 10).

**Still open**: none.
Sequencer structure decided: one row per cell, added with the right click on the cell (SQ-1, SQ-12).

Export towards Cubase decided: MIDI clip (drag & drop or `.mid` file) for the notes, WAV/AIFF/FLAC audio file for the sound with effects, to be imported into the DAW from outside. Real-time MIDI output (SQ-7c) remains an optional test.

## 14. Indicative roadmap

1. **MVP**: VST3 plug-in (Win + Mac) with 4×12 grid, file browser, MIDI mapping, one-shot/gate trigger, start/end, AHDSR, tune/pitch, outputs per cell, kit saving.
2. **Effects**: chain per cell and global (standard then creative).
3. **Loops**: sync to BPM, time-stretch with Key Lock.
4. **Slicing**: automatic/manual slices and distribution onto the cells.
5. **Sequencer**: 64-step patterns, per-step parameters, effect parameter locks, MIDI export and WAV audio export.
6. **Standalone**: stand-alone build for Windows/macOS with internal transport, audio/MIDI settings and record. The standalone target is set up from the skeleton of the project (JUCE) and also serves as a quick test bench during development, without going through the DAW.
7. **MIDI learn of the parameters** (sec. 5.1): assignment with right click, mappings panel, saving in the kit; to be started when the effects parameters are stable.
8. **Polish**: layers/groups, undo, optimisations, tests in Cubase 16 (and other DAWs).
