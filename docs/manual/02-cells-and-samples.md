# 2. Cells and samples

## The grid

The grid has **48 cells** (rows A–D, columns 1–12). Each cell contains a sample and all its settings. Each pad shows the ID (A1…D12), the name of the sample, a mini waveform and, if the cell is in the sequencer, an **orange dot** at the bottom right. A pad lights up when it plays.

### Loading a sample

| Way | How |
|-----|-----|
| From the browser (drag) | Drag the file from the browser on the left onto a pad |
| From Windows Explorer | Drag one or more files directly onto a pad. With several files, they fill the following pads in order |
| Double click | Double-click a file in the browser: it goes into the selected pad |
| Pad menu | Right click on the pad → **Load sample...** |

Formats read: WAV, AIFF, FLAC, OGG, MP3.

### Pad menu (right click)

- **Add to sequencer / Remove from sequencer** – adds or removes the cell from the sequencer.
- **Load sample...** – loads a file.
- **Rename...** – renames the cell.
- **Clear cell** – empties the cell.

### Multiple selection

To work on several cells together (for example to empty them in one go, after distributing slices):

| Gesture | Effect |
|---------|--------|
| **Ctrl+click** (Cmd on Mac) on a pad | Adds or removes that pad from the selection |
| **Shift+click** on a pad | Selects all pads from the last clicked one to this one (even across rows, in the order A1, A2…) |
| **Ctrl+A** | Selects all cells |
| **Esc** or a simple click | Goes back to the current cell only |
| **Del** (or Backspace) | **Empties** the selected cells that have a sample: it asks for confirmation and tells you how many there are |
| **Right click** on a selected cell | Menu with **Clear N selected cells**, **Add N selected to sequencer**, **Remove N selected from sequencer**, **Select only this cell** |

Selected pads have an orange border and veil; the **current** one (whose panel you see on the right) also has the glow. With Ctrl or Shift the pad does not play. Emptying a cell removes the sample and puts the cell back on its key (the file on disk is not touched), and it can be **undone** with Ctrl+Z (see chapter 8).

**Options on several cells together.** With several cells selected, the title of the panel on the right shows `(+N)` and these options apply to **all** the selected cells that have a sample: **Trigger mode**, **Reverse**, **Normalize**, **Sync to tempo**, **Key lock**, **Loop BPM**, **Detect** and **Output**. The values shown are those of the current cell. MIDI mapping (Root note, Key low/high) and the envelope and effect knobs stay on the current cell only.

### Selecting and listening

A click on a pad **selects** it and plays it (as long as you hold it, for the Gate and Loop modes). The selected pad has an orange border, and the panel on the right, the waveform and the modules at the bottom show **its** settings.

## The file browser

- The menu at the top chooses where to look: **Home**, **Desktop**, **Documents**, **Music** and all the drives of the computer.
- The **Search samples...** field filters files by name in the open folders.
- **Preview** (at the bottom) turns on the preview: selecting a file plays it. The slider next to it sets the preview volume.

## The cell panel (on the right)

| Element | What it does |
|---------|--------------|
| Title with **< >** | Shows the selected cell; the arrows go to the previous/next cell |
| **Sample** / **Load** | The assigned sample; Load opens the choice of a file |
| **Trigger mode** | **One-shot**: plays to the end and ignores the release of the key. **Gate**: plays while you hold the key, then fades out with the Release. **Loop**: repeats while you hold the key |
| **Reverse** | Plays the sample backwards |
| **Normalize** | Sets the Level of the cell so that the highest peak of the sample reaches −1 dBFS (the audio file does not change) |
| **Loop / tempo** | **Sync to tempo**, **Key lock**, **Loop BPM** and **Detect** (see below) |
| **MIDI mapping** | **Root note**: the note at which the sample plays at its original pitch. **Key low / Key high**: the range of keys that plays the cell |
| **Sample info** | Format and length |

### MIDI mapping

By default pad **A1 plays C1** (MIDI note 36) and each following pad the next note, row by row (A1…A12, B1…). The 48 cells cover the notes from C1 to B4.

- To change the note of a cell, change **Key low** and **Key high** (equal = a single note).
- If **Key low** and **Key high** are different, the cell plays on several keys and the sample is **transposed** relative to **Root note**: this is how you play a sample melodically.
- Several cells can answer to the same note: they play together (layers).

**What you see on the pads:**
- At the bottom left of each pad you see the cell's **key** (for example `C1`) or its **range** (`C1 - C2`).
- An **empty** pad whose note falls within the range of another cell is drawn with **stripes** in the colour of that cell and the text `← A1`: it means that pressing that key plays cell A1, and that if you load a sample into that pad the two cells would play together.
- An orange triangle **!** at the top right signals that two cells **with a sample** share keys. Hovering over the pad, the tooltip lists which ones.

**If an empty pad has stripes:** it means that the range of another cell (the label next to the arrow, for example `← A10`) covers its note. It is not a badly deleted cell: to remove them, right click on the striped pad → **Reset key range of A10** (the cell that covers it), or on the pad of that cell → **Reset key range** (or set Key low and Key high equal again in the panel on the right). **Clear cell** also puts the cleared cell back on its starting key.

Note: the MIDI key notes also apply to the sequencer, but each row plays its cell only with the **Key low** note; the other keys of the range are for playing from the keyboard.

## Loop and tempo (Sync, Key lock)

To make a loop (for example a breakbeat or a drum pattern) follow the **BPM of the project**:

1. **Loop BPM** is the original tempo of the loop. When you load a sample of at least half a second it is **estimated from the length**, assuming that it lasts a whole number of bars (1, 2, 4, 8, 16 or 32 beats) and choosing the most plausible tempo (70–170 BPM, the closest to 120). The value can always be edited: if the estimate is wrong (for example a loop of 8 beats read as 16) correct it by hand or change **Start/End** and press **Detect**. With the value **--** (zero) the tempo is unknown and synchronisation does nothing.
2. Turn on **Sync to tempo**: the sample is **stretched or shortened** to the BPM of the project (that of the DAW in the plug-in, that of the **BPM** box at the top in the standalone). The original file does not change: the processed copy is prepared in the **background** with Rubber Band (high quality) and is heard after a moment. If the BPM changes, it is reprocessed by itself after a short pause.
3. **Key lock** on: the tempo changes **without changing the pitch**. Off: tempo and pitch change together, like on a turntable.

Pitch and Tune keep working on top of the synchronisation. Start and End stay at the same relative positions. Exporting (MIDI, audio, rows) uses the sample synchronised to the BPM of the moment.

> **Note:** the estimate from the length works well for loops cut at exact bars. A sample with silence at the end or not cut to measure gives the wrong tempo: in that case set Start/End on the exact loop and press **Detect**, or type the BPM.

## The waveform (under the grid)

It shows the sample of the selected cell.

- Drag the left marker to set **Start** and the right one to set **End**: the sample plays only between the two. The excluded areas are darkened.
- On the right you read **Start**, **End** and **Length** in milliseconds.
- The edit is **non-destructive**: the audio file is never changed.

## The modules of the Main tab

### AMP – envelope and level

| Control | Value | Effect |
|---------|-------|--------|
| Attack | 0–2 s | Entry time |
| Hold | 0–2 s | Time at full level |
| Decay | 0–5 s | Fall to the Sustain |
| Sustain | 0–1 | Held level |
| Release | 0–5 s | Exit time at release |
| Level | −60…+12 dB | Volume of the cell |
| Pan | −1…+1 | Stereo position |

Under the knobs the **envelope graph** shows the shape you are drawing.

### PITCH

- **Tune:** fine tuning, ±100 cents.
- **Pitch:** transposition, ±24 semitones.

The change is heard immediately, even on notes that are already playing.

### MASTER

- **Volume:** general volume of the kit, from −60 to +12 dB.
- The **meter** on the right shows the output level.

## Normalising to get the same volume

Samples often have different volumes. The **Normalize** button of the cell panel brings the peak of that sample to −1 dBFS by adjusting its **Level** (which you can always touch up). To do it on the whole kit: **Kit → Normalize all cells** menu.

> **Note:** normalisation aligns the *peaks*, not the perceived volume. A hi-hat and a kick with the same peak can sound at different volumes: adjust the Level by ear.

## Knobs and values (valid for the whole app)

- **Drag** a knob to change it; **double-click** brings it back to the default value (where provided).
- **Right click** on a control opens the **MIDI learn** menu (chapter 6).
