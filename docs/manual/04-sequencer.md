# 4. Sequencer

The sequencer is in the **Seq** tab. Each **row** plays one cell; each row has its own **steps** (up to 64), its own length and its own resolution.

## Starting and stopping

- **Play/Stop** in the top bar, or the **space bar**.
- **Standalone:** the sequencer has an internal transport and starts from the beginning every time you press Play. The tempo is set in the **BPM** box.
- **Plug-in:** if the transport of the DAW is running, the sequencer **follows** it (tempo and position) and the Play button is disabled. If the DAW is stopped, you can try the pattern with the Play of the plug-in.

## Adding cells

- From the grid: right click on a pad → **Add to sequencer**.
- From the Seq tab: **+ Cell** opens a list with **ID, name and waveform**. Hovering over an entry **plays the sample**, so you choose by listening. At the bottom there is **Add all loaded cells**.
- To remove a row: the **x** button of the row.

A **click on the name** of the row plays the cell and selects it (so the FX tab and the panel on the right show that cell).

## The row

From left to right:

| Element | Function |
|---------|----------|
| **M** | Mute of the row |
| **S** | Solo: if at least one row is in solo, only the rows in solo play |
| **FX** | On: the row uses the effects of the cell. Off: it plays without per-note effects (filter, saturation, LoFi) and without sends to delay/reverb. Steps can turn them back on with locks |
| Name | ID and name of the cell with the mini waveform |
| Resolution | Duration of each step: from **1/1** to **1/64**, also dotted and triplets. Each row has its own |
| Length | Number of steps (1–64). Each row has its own length: different lengths create **polyrhythms** |
| **x** | Removes the row |

The rows are **synchronised to the tempo**: rows with different resolutions (for example one at 1/16 and one at 1/32) stay aligned.

### Length and columns

The length of each row is set from **8 to 64** with **+** and **−** (or by dragging the number). The visible columns are those of the longest row: you always see **all the steps on one line, without pages**, and they narrow as the length grows. If the window is too narrow for 64 steps, horizontal scrolling appears and the row header stays fixed. A ruler numbers the steps every 4 and the steps are grouped in fours. Steps beyond the length of a row are off and not clickable. A white outline follows the step that is playing.

The Seq tab uses the whole width of the window (the browser and the cell panel are in the Main tab). Scroll vertically if you have many rows.

## The steps

| Action | Result |
|--------|--------|
| Click on an **empty** step | Turns it on and selects it |
| Click on a lit step that is **not selected** | Selects it (without turning it off) |
| Click on a lit step that is **already selected** | Turns it off |
| **Right click** or **Shift+click** | Selection only, does not change the on/off state |
| **Drag up/down** a lit step | Changes the **velocity** (the height of the bar of the step) |
| **Drag horizontally** starting from an empty step | "Brush": turns on all the steps touched (also on other rows) |
| **Drag horizontally** starting from a lit step | "Eraser": turns off all the steps touched |

On a lit step, small marks indicate: a **dot** = it has locks on the effects; **x2/x3/x4** = ratchet; **%** = probability below 100%.

### Saving the sequences

The kit file does not contain the patterns. To save them (all 16 together) use the **Save seq** button, in the second row of controls of the Seq tab: it writes a `.slseq` file (it proposes the `Sequences` folder next to the kit). To reopen them use **Kit → Open...** and choose the `.slseq` file (it replaces all the patterns, with confirmation). Details in chapter 5.

### Copying and pasting steps and rows

- **Single step:** select a step (right click or Shift+click selects it without changing it) and press **Ctrl+C**; select another step and press **Ctrl+V**. The whole step is copied: on/off, velocity, probability, ratchet, nudge, gate, pitch, reverse, mode and the **locks** of the effects. You can also paste onto another row or into another pattern.
- **Whole row:** right click on the row (on the name of the cell or on an empty space of the header) → **Copy lane**; then on the destination row → **Paste into this lane**. The steps, the length and the resolution are copied; the cell, Mute, Solo and FX of the destination row stay. **Clear all steps** empties the row.
- Everything can be undone with **Ctrl+Z**.

## The step editor (STEP EDIT)

At the bottom, for the selected step. At the top the title says which step you are editing.

### Behaviour

| Control | Effect |
|---------|--------|
| **Mode** | **As cell**: uses the Trigger mode of the cell. **One-shot**: plays the whole sample. **Gate**: plays for the duration of the step and then fades out with the Release. **Loop**: repeats the sample for the duration of the step |
| **Reverse** | Plays this step backwards (combined with the Reverse of the cell: two reversals cancel each other) |
| **Velocity** | Volume of the step (1–127) |
| **Probability** | Probability that the step plays (0–100%) |
| **Nudge** | Advances or delays the step (−50…+50% of its duration) |
| **Gate** | Duration of the note as a percentage of the step: matters for the Gate and Loop modes |
| **Pitch** | Transposition of the step (±24 semitones) |
| **Ratchet** | Repeats the hit 1–4 times inside the step |

**Probability is repeatable**: the same pattern always gives the same result (useful for exporting). **Reset seed** changes the random "seed": the probability choices change but stay repeatable.

### Effects per step (locks)

Under the knobs there are the **Saturation, LoFi, Filter, Delay, Reverb** cards. They are used to change an effect **only on that step**.

- **Grey knob**: follows the value of the cell.
- **Orange knob**: value **locked** on this step. It is locked by moving the knob; **double click** removes the lock.
- Locking a value **turns the effect on for that hit**, even if it is off on the cell. With the switch of the card you can force the effect on or off for the step.
- **Clear locks** removes all the locks of the step.

The lockable parameters are those that make sense for a single hit:

| Card | Can be locked | Stays with the cell (not locked per step) |
|------|---------------|-------------------------------------------|
| Saturation | Drive, Mix | — |
| LoFi | Bits, Rate, Mix | — |
| Filter | Cutoff, Reso | Filter type |
| Delay | **Send** (how much is sent) | Time, Feedback, Sync, musical value (but they are adjusted from the same card) |
| Reverb | **Send** | Size, Damp (but they are adjusted from the same card) |

In the Delay card you also find **Sync** and the musical value: they are settings of the cell (the same as in the FX tab).

**Example:** an echo only on the third hit.

1. On the cell leave the **Delay off**.
2. Select the step and in the Delay card set **Time** (or Sync + value) and **Feedback**.
3. Move **Send** up: only that hit sends the echo, and the echo keeps sounding after the step.

## Patterns, swing and bars

The top bar of the Seq tab has two rows:

- **Row 1:** `< Pattern 1 >` and the **⋯** menu (actions on the pattern) · **+ Cell** and the squares button (**add all cells with a sample**) · **Export N bars** and **Tail** · **Export MIDI**, **Bounce WAV**, **Export Lanes** · the **Swing** knob · **Reset Seed**.
- **Row 2:** the **DRAG TO DAW** handles (chapter 7).

### The 16 patterns

`< Pattern 1 >` chooses one of the **16 patterns** of the kit, each independent (rows, steps, swing, seed).

The **⋯** menu next to the patterns lets you work on them:

| Item | What it does |
|------|--------------|
| **Duplicate to the next empty pattern** | Copies the active pattern into the first **empty** pattern that follows (if you reach the end it starts again from the beginning) and opens it: you edit or lengthen it without touching the original. If there is no empty pattern a warning appears |
| **Copy pattern** | Puts the active pattern in the clipboard |
| **Paste into this pattern** | Replaces the content of the active pattern with the copied one (asks for confirmation if it is not empty). The **name** of the pattern does not change |
| **Clear pattern** | Empties the active pattern (asks for confirmation) |
| **Rename...** | Changes the name shown in the menu |

**Changing pattern while playing.** If the sequencer is playing and you choose another pattern, the current pattern **finishes its cycle** and the new one starts **on the next round**, without cuts. The cycle is the duration of the longest row of the current pattern (for example 16 steps at 1/16 = one bar; 64 steps at 1/16 = four bars). Meanwhile the chosen pattern is already visible and editable; the white outline that follows the step is on the pattern that is really playing, so it appears on the new one only when it starts. The held notes (Gate or Loop mode) of the old pattern are released at the change. When stopped the change is immediate.

To copy into a specific pattern: **Copy** on the source pattern, go to the destination one with **< >**, then **Paste**. The clipboard lasts as long as the app is open and is not saved in the kit.

### Swing and seed

- **Swing**: delays the odd steps by a fraction of the step (0–50%).
- **Reset Seed**: changes the random "seed" (probabilities and ratchets change but stay repeatable).
- **Export 1/2/4/8 bars** and **Tail**: length and tail of the exports (chapter 7); they do not change playback.

## Recording from the keyboard and quantising

In the second row of the tab, next to the DAW handles, there are **Click**, **Rec** and **Rec options**.

**Click** (metronome) and count-in. The **Click** button turns on a **metronome**: a click on every quarter note while the sequencer runs, higher on the first quarter of each bar (4/4). It goes to the Main output and does not pass through the effects. The **click volume** (metronome and count-in) is set with the slider next to the button (double click: default value) or in **Rec options → Click volume** (25, 50, 75, 100%); it is remembered in the project. To record in time there is also the **count-in** (precount): with **Rec** on and the internal transport (standalone, or plug-in with the DAW stopped), **Play** does not start the pattern at once but first counts **1 bar** (4 clicks, even with the metronome off) and the Rec button shows the number still missing (4, 3, 2, 1). Then the pattern starts and recording begins. The length is chosen in **Rec options → Count-in before Play** (**Off**, **1 bar**, **2 bars**). In the plug-in, with the DAW running, the count-in is the DAW's. Notes played during the count-in are not recorded (except for an anticipation of half a quarter on the first quarter).

**Rec** (recording).
1. Turn on **Rec** and start the sequencer (**Play**): after the count-in, if it is active.
2. Play the MIDI keyboard: each note **turns on the nearest step** of the row of the cell that note plays, with the **velocity** at which you pressed it. If the cell does not have a row yet, it is added.
3. If the cell covers several keys (melodic sample), the **pitch** is also recorded (Pitch of the step = note played − Root note).
4. Recording **overwrites** velocity, pitch and position of the step it falls on; the other parameters of the step (probability, ratchet, locks...) stay.
5. The notes you play keep sounding normally while you record. Rec does not record while the **Song** tab is playing. With the DAW recording (plug-in) and the "silent while recording" mode the sequencer stays quiet and Rec writes nothing.

A note that falls between two steps goes to the nearest one. How much of your "feel" stays is decided by **Rec options → Recorded notes: snap to the grid**: **100%** (default) puts the note exactly on the grid, **75 / 50 / 25%** keep a part of it as **Nudge**, **Free** keeps all the micro-timing. Recording does not compensate for the latency of the audio device.

**Rec options** (menu, with the recording settings and the quantisation of the steps already written).
- **Quantize existing steps → All lanes** or **Selected lane** with strength 100 / 75 / 50 / 25%: brings the steps that have a **Nudge** (for example those recorded with partial snap) back towards the grid. 100% resets the Nudge.
- The **swing** of the pattern is not touched: it stays as it is.
- Moving between different resolutions is not a quantisation: to change the division of a row use its resolution (see above).

### Length and resolution of all the rows

The **⋯** menu next to the pattern name has two submenus to change **all the rows together**: **Length of all lanes** (8, 12, 16, 24, 32, 48 or 64 steps) and **Resolution of all lanes** (1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/32T, 1/64). The steps already written stay where they are (changing the resolution changes when they play, as with the command of a single row); it can be undone with Ctrl+Z.

## Things to know

- Copy and paste work for a single step, a whole row or a whole pattern, not for groups of steps. **Ctrl+Z / Ctrl+Y** undo and redo also the changes to the sequencer (chapter 8).
- A lock applies to the hit of that step; its echo (delay/reverb) continues afterwards.
- Changing a cell (name, effects, levels) also changes how it sounds in the sequencer, because the cell is the same.
