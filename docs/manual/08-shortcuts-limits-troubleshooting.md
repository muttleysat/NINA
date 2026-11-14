# 8. Shortcuts, limits and troubleshooting

## Shortcuts and gestures

| Gesture | Function |
|---------|----------|
| **Space bar** | Play/Stop of the sequencer (in the Slice tab: plays/stops the zone) |
| **Ctrl+C / Ctrl+V** | Copy / paste the selected step (Seq tab) |
| **Ctrl+Z** | Undo (Main, Seq, FX and Slice tabs) |
| **Ctrl+Y** or **Ctrl+Shift+Z** | Redo |
| Click on a pad | Selects and plays the cell |
| Right click on a pad | Cell menu (add to sequencer, load, rename, clear) |
| Double click on a browser file | Loads the sample into the selected pad |
| Drag a file onto a pad | Loads the sample |
| Double click on a knob | Default value; on the locked knobs of the steps: removes the lock |
| Right click on a control | MIDI learn menu |
| Drag up/down a lit step | Velocity of the step |
| Right click or Shift+click on a step | Selects without turning on/off |
| Click on a row name | Plays the cell and selects it |
| Hover over an entry of **+ Cell** | Plays the cell so you choose by listening |

## Undo and redo

**Ctrl+Z** undoes, **Ctrl+Y** (or Ctrl+Shift+Z) redoes; the same items are in the **Kit** menu (at the top). They work in the **Main**, **Seq**, **Song**, **FX** and **Slice** tabs.

- These can be undone: loading and clearing cells (also in bulk), distributed slices, the name, the keys and the MIDI range, **all knobs and switches** (envelope, pitch, effects, Sync, Key lock, Loop BPM, output...), the slice markers of a cell and, in the sequencer, **steps, rows added or removed, swing, name and content of the patterns**, and in the Song tab **blocks, repeats and Loop** (also the steps recorded with **Rec**, in blocks of about a tenth of a second). If you undo a change made on another pattern, the app shows it.
- A knob drag or a slice distribution is **a single step**. The last 100 are kept.
- In the Slice tab with the **File** source the markers, the zone (the brackets) and the Source BPM can also be undone. Undo always removes the last thing done, whether in the File source or in the cells.
- These **cannot** be undone: MIDI learn, saving kits, changing the active pattern (it is navigation). Opening or creating a kit clears the history.
- Changes made by the automation of the DAW do not enter the history: only what you do in the interface counts.

## What is not there yet

- Loop tempo detection is an **estimate from the length** (it does not analyse the hits): for loops not cut at exact bars the BPM must be typed by hand.
- Loop synchronisation works on the **whole** sample (Start/End always refer to the original): there is not yet a way to align the grid of the loop (for example to shift the start of the first hit).
- For slicing: no alignment of the cuts to the tempo grid (see chapter 9). The tempo of tracks without a fixed tempo has to be estimated over short zones (no beat map).
- Further creative effects beyond those in chapter 3.
- Adding **rows and cells** beyond the 48 of the grid.
- The **macOS** version (the code provides for it but it has never been compiled or tried) and the AU format.
- Samples from an audio input (sampling from the audio interface): excluded for now.

## Things to verify (not tried yet)

- Operation of the VST3 in **Cubase 16**: opening, resizing, parameter names, automation recording, MIDI input.
- **Dragging** files to Cubase.
- **Sync to tempo** and **Key lock** with real loops in the app (verified only with automated tests on test signals).
- Use with a real MIDI keyboard and a controller for MIDI learn.

## Troubleshooting

**I hear nothing (standalone).** Open **Options** and choose the output device. Check that the *Volume* in the MASTER module is not at −60 dB and that the cell has a sample (not "MISSING").

**The sound is too quiet.** Try **Normalize** on the cell or **Kit → Normalize all cells**, then raise **Level** or the master **Volume** (up to +12 dB).

**The plug-in does not appear in Cubase.** Check that `NINA.vst3` is in `C:\Program Files\Common Files\VST3` (or, for a per-user installation, in `%LOCALAPPDATA%\Programs\Common\VST3`) and rescan the plug-ins.

**An echo is heard on all the steps.** The Delay is on for the cell: all notes send the echo. Turn off the Delay switch on the cell (FX tab) and use the per-step **Send**.

**An effect of a step is not heard.** Move the knob of the effect: locking a value turns it on for that step. Check that the row has **FX** on (otherwise the row plays without per-note effects) and that the switch of the card is not forced off.

**A cell shows MISSING.** The sample cannot be found: put the folder back next to the `.slkit` file and reopen the kit.

**The sequencer does not start in the plug-in.** If the transport of the DAW is playing, the sequencer follows it. If the DAW is stopped, press Play in the plug-in. If it still does not start, check that at least one row has steps turned on.

**Some controls disappear when I resize.** The sizes follow the narrower side of the window, but there is a minimum (about 1000×640): it does not go below that. If you see cut controls, report it with the dimensions.

**The kit shows an asterisk (*).** There are unsaved changes: press **Save**.

## Where to find the working files

- Saved kits: where you chose, as a `.slkit` + folder pair.
- Exports: **Bounces** folder next to the kit (or Documents).
- State of the standalone: in the settings of the application in Windows (the `NINA` folder in `AppData\Roaming`).
