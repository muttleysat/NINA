# 7. Exporting to the DAW

From the **Seq** tab you can bring the pattern into Cubase in three ways. All of them use **the current tempo and length**: the BPM shown at the top and the **1/2/4/8 bars** menu.

## The tempo of the audio export (plug-in)

Audio (Bounce WAV, Export lanes, Drag audio, Drag lanes) is created **at the tempo of the project**. In the plug-in NINA knows the tempo of the DAW only **after the DAW has sent it**, that is usually once the transport has started at least once (then it remembers it, even with the DAW stopped: the BPM box at the top becomes non-editable and shows "last received"). To avoid files at the wrong speed (for example at the default 120 BPM when your project is at 90), **if NINA has not received the tempo yet the audio export stops with a warning**.

- Solution 1: press **Play** in the DAW (even for a second) and try again.
- Solution 2: type the tempo of the project in the **BPM** box (double click) and try again.
- The confirmation window after **Bounce WAV** tells you **at how many BPM and how many Hz** the file was created: check that they match the project.
- If the tempo NINA received **is not that of your track**, you can choose the **export tempo** by hand: in the plug-in double-click the **BPM** box at the top and type the tempo (for example 126). The value appears in orange with an asterisk (`126.0 BPM *`) and applies to **all exports** (audio and MIDI) until you set it back to **0**, which returns to the automatic tempo of the DAW. The playback tempo stays that of the DAW.
- **"Follow tempo" / Musical Mode:** every exported audio WAV file contains **ACID metadata** with the **tempo** (that of the export), the **number of quarter notes** and the adaptable loop flag. Cubase reads them on import: with the **Follow tempo** option (Musical Mode) on, the file is recognised at the right tempo, and if you later **change the tempo of the project** the audio adapts correctly. Without this data Cubase **estimates** the tempo from the length and can be very wrong. If a file already exported with an earlier version is recognised wrongly, export it again, or in Cubase set the tempo by hand in the **Pool** (*Tempo* field of the file).
- **Sample rate of the files:** the menu next to **Tail** (at first it shows **Auto** followed by the rate the DAW communicated to the plug-in, for example *Auto 48k*) (in the Seq and Song tabs) chooses the rate of the exported audio: **auto** (that of the project/DAW, as the DAW communicates it to the plug-in), **44.1**, **48**, **88.2** or **96 kHz**. The audio is **rendered directly** at that rate, without later conversions, and the duration stays identical. If a file imported into Cubase plays at the wrong speed, choose the sample rate of your project (you find it in Cubase under *Project > Project Setup*) and export again. The window after **Bounce WAV** and the log tell you the rate used.
- Every audio export writes a line in the `ui.log` log (the `NINA` folder inside `AppData\Roaming`) starting with `EXPORT`: it contains the **tempo used**, the current tempo, the one received from the DAW, the one chosen by hand and the **sample rate**. It helps to understand where a file at the wrong speed comes from.
- If the project changes tempo, NINA uses the one received last (a single tempo for the whole file). The **MIDI** export does not have this problem: the notes follow the tempo of the project in any case.

## What each option exports

| Option | Content | Effects included? |
|--------|---------|-------------------|
| **MIDI** | The notes of the pattern as a MIDI clip. Each cell plays its note (**Key low** of the cell), with velocity, ratchet, swing and probability already "resolved" into fixed notes | **No** (MIDI cannot contain them) |
| **Audio** (Bounce WAV) | The whole sequence in **a single** 24-bit WAV file | **Yes**: cell effects, step locks, synced delay, tails (see Tail), master effects |
| **Rows** (Export lanes) | **One WAV file per row** in a folder, all **of the same length** | Yes: the effects of each cell (with synced delay and tails). **Without** the master effects |

The row files summed give exactly the whole audio (without the master effects). Use them to **remix** each sound on its own track.

Reverse and effect locks do not exist in MIDI: to have them in the DAW use audio.

## Playing and recording by hand with the sequencer (plug-in)

When the transport of the DAW plays, the sequencer starts with it. If you record a drum line by hand with the record button of Cubase, the sequencer would get in the middle of what you play. For this, in the row under Swing/Reset Seed of the Seq tab, there is the **DAW** menu (visible only in the plug-in):

| Item | What it does |
|------|--------------|
| **follow transport** | The sequencer always plays when the DAW plays (also while recording) |
| **silent while recording** (default) | Plays with the DAW, but **stays quiet while the DAW records**: you hear and record only what you play |
| **ignore transport** | Ignores the transport of the DAW: it starts only with the plug-in's **Play** button (the tempo stays that of the DAW) |

The choice is saved in the project. The sequencer does not record anything by itself: the notes you play reach Cubase as normal MIDI. With **follow** you can instead hear the sequencer as a backing while you record.

## The tails of delay and reverb: Tail

Next to the bars menu there is the **Tail** menu, which decides what happens to the tails of the effects in the **audio** files (Bounce WAV, Export lanes, Drag audio, Drag lanes):

| Tail | Length of the file | What it does |
|------|--------------------|--------------|
| **cut** (default) | **Exactly** the chosen bars | The tail beyond the end is cut: the file is quantised and aligns to the grid of the DAW |
| **loop** | Exactly the chosen bars | The tail of the end **wraps around to the beginning**: the looped clip plays without gaps (as if the pattern were running) |
| **keep** | Bars + the whole tail | The file is longer than the pattern; it stops when the tail ends |

For a loop in Cubase use **cut** or **loop** (loop is better if the echo must continue between one round and the next). With **loop** the pattern is played twice and the second pass is kept: very long echoes (over one bar) can be incomplete at the beginning.

## The fastest way: drag into the DAW

Above the rows, in **DRAG TO DAW**, there are three handles:

- **Drag MIDI** – drag it onto a MIDI or instrument track of Cubase: it creates the clip.
- **Drag audio** – drag it onto an audio track: the whole sequence with the effects.
- **Drag lanes** – drag it into the project: all the files arrive, one per row.

The files are generated **at the moment you start dragging**, with the tempo and bars of the moment (audio can take a moment). They go through a temporary folder of the system.

> **Verification status:** dragging to Cubase has not been tried yet. In the standalone it uses the file-drag function of the operating system; in the plug-in inside Cubase it depends on how the DAW accepts dragging from plug-ins. If it does not start, use the buttons to save the files.

## Saving the files (buttons)

- **Export MIDI** – opens the save window for the `.mid` file.
- **Bounce WAV** – saves the `.wav` of the whole sequence.
- **Export lanes** – asks for a folder and creates in it a sub-folder with one file per row.

The proposed name contains kit, pattern, BPM and bars (for example `MyKit_Pattern 1_120bpm_2bar.wav`). When importing them into Cubase remember to set the project to the same tempo.

## A recommended flow in Cubase

1. Set the tempo of the project and prepare the pattern in NINA (same BPM).
2. If you want to be able to **edit the notes**: *Drag MIDI* onto a track that plays NINA (or another instrument).
3. If you want **the finished sound**: *Drag audio* onto an audio track.
4. If you want to **remix** each sound: *Drag lanes*, then balance the tracks.
5. If you change the pattern or the tempo, **export again**: the export is a snapshot, not a link.

## Separate outputs (plug-in)

The plug-in has **16 stereo outputs**: **Main** (the mix) and **Out 2 … Out 16**. In **Cubase** the additional outputs are off by default: open them from the VSTi rack (the outputs of the instrument) and activate those you need, one per track.

1. Select a cell and in the panel on the right choose **Output** (Main, Out 2…).
2. Alternatively use the **Kit → Outputs: one cell per output** menu to send the first 15 cells with a sample to Out 2…Out 16 (the others stay on Main), or **all cells to Main** to cancel.
3. If an output is off in the DAW, the cell **plays on the Main** (it does not disappear).

The single outputs are **before** the master effects and the master volume (the effects of the cell stay). The sequencer also uses them: the rows play on the output of their cell. The exports (MIDI, audio, rows) do not change: the exported audio contains everything in the mix, as before.

> **Verification status:** the routing is proven by automated tests on the engine; not yet verified in Cubase. In the standalone there is only the Main (multiple outputs apply in the plug-in).

## Limits

- The export does not include the **automation** of the DAW, but the current values of the controls.
- The synced delay uses the BPM **of the moment of the export**.
- An export **does not run in real time**: it is faster, and does not depend on the DAW.
- With **Tail: keep** the file of the whole sequence is cut after the end of the tail to avoid useless silence; the files of the rows stay of the same length as each other, in every mode.
