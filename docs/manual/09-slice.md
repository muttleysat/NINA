# 9. Slice: cutting a loop onto the cells

The **Slice** tab (the last at the bottom, with the scissors icon) cuts a sample into **slices** and distributes them onto the cells: a drum loop becomes one cell per hit. The source can be the sample of a **cell** or an **external audio file** (even a whole track, of which you choose the zone).

The tab has three rows at the top, always in the same order:

| Row | What it contains |
|-----|------------------|
| **SOURCE** | **Cell / File** (the source), **Load file...** (only with File), the name of the source with length and format, **Audition** and its volume |
| **CUT** | How to find the cut points: **Detect** with **Sensitivity**, **Even**, **Clear**, **Chop** |
| **TEMPO** | **Source BPM** with its tools: **Detect**, **/2**, **x2**, **Tap**, **Click test** |

Below, the waveform with the markers, the **CELLS** strip and the row with **Distribute to cells**.

## How it works

1. In **Main** select the cell with the loop (for example A10) and open the **Slice** tab. The sample is that of the selected cell; the part that is cut is the one between **Start** and **End** of the cell.
2. Find the cut points:
   - **Detect** puts a marker on every **hit** found. The **Sensitivity** knob sets how sensitive it is: higher finds more hits, even weak and close ones. If you change it, the detection is redone by itself.
   - **Even: 1/2 … 1/32** cuts the region into **equal** parts.
   - **Clear** removes all the markers (the one at the start stays).
3. Refine by hand on the large waveform:
   - **Click on a slice**: you hear it (if **Audition** is on; next to it there is the preview volume).
   - **Drag a marker** to move it (the numbered flag at the top).
   - **Double click** on the waveform to **add** a marker.
   - **Right click** on a marker to **remove** it.
   - The **− [ ] +** buttons (bottom right) zoom; with the **wheel** you zoom around the mouse, with **Shift+wheel** you scroll. The orange brackets are Start and End of the cell, which are changed in the Main tab.
   - Hovering over a slice shows its **duration in ms**.
4. The **CELLS** strip shows the cells that will be filled: in orange with the **number of the slice**, the original cell with a white outline (**original**), the cells already occupied in red with a triangle and the text **occupied** (they would be replaced). **Click a cell** to start the distribution from there.
5. **Keep original in A10** (on): the slices go into the cells **after** the original, which stays as it is. Off: they start from the cell of the loop itself.
6. **Distribute to cells** fills the cells. If some are occupied it asks for confirmation.

The text at the bottom left summarises everything: number of slices, from which cell to which, how many will be overwritten.

## Listening, stop and zoom

- **Play zone** (SOURCE row) or the **space bar** plays the zone from start to end with a white cursor; press again (or **Esc**) to **stop**. In the Slice tab the space bar therefore controls listening to the zone and not the sequencer. Esc also stops the Chop and the Click test.
- A click on a slice plays only that slice (with Audition on).
- With **zoom** the waveform **follows the cursor** while it plays (in pages). If you move the view by hand (wheel) the following stops for a few seconds. Zooming with the **− [ ] +** buttons centres on the cursor if it is playing; the wheel zooms **around the mouse** (Shift+wheel scrolls).
- **Scrub**: **drag** the mouse on the waveform to hear the point under the cursor (short 140 ms listens while you move). A click on the **time ruler** (the strip of numbers at the top) plays from the indicated point to the end of the zone; dragging on the ruler scrubs. If you are listening to the **whole zone** (Play zone), a click on the waveform **jumps to that point** and continues to the end; the scrub also resumes from where you release the mouse. While the music plays (or during the scrub) the slice under the mouse is not highlighted.
- **Position cursor**: where you release the mouse after a scrub, where you click, and where playback is when you press **Stop** (or Esc, or the space bar), a **dashed cursor** with the **time** (minutes:seconds.milliseconds) stays. It does not disappear: it is used to locate a point in a long track and then move the brackets. With the **File** source: **[** brings the **start** of the zone to the cursor and **]** brings its **end**; alternatively right click on the waveform → **Zone start here** / **Zone end here**. Right click also offers **Place the cursor here**, **Play from here** and **Add a slice marker here** (right click on a marker still removes it). The cursor is reset when you change file or cell.

## Source: a cell or an external file

- **Cell** (default): the source is the sample of the cell selected in the Main tab. The zone is the one between Start and End of the cell (changed in Main). The slices stay in the **same file**, each with its own Start/End.
- **File**: the source is an external audio file. To load it:
  - **drag** the file onto the Slice tab (from Windows Explorer or from the browser on the left), or
  - press **Load file...**, or
  - **double click** a browser file while you are in the Slice tab.

  The waveform shows **the whole file**. The two **orange brackets** delimit the **zone** to cut: drag them (also zooming with **− [ ] +** and Ctrl+wheel). The part outside the zone is darkened. Detect, Even, Chop, Click test and the slices work **only inside the zone**. The first marker always follows the start of the zone.
- With **File**, **Distribute to cells** starts from the **cell selected in Main** (change the start by clicking a cell in the CELLS strip). Each slice is **cut out into a 24-bit WAV file** in the `Slices` folder next to the kit (or in `Documents\NINA Slices` if the kit is not saved yet), in a sub-folder with the name of the file; if it already exists a new one is created (`... 2`), so it never overwrites a previous distribution. The cells are therefore normal samples and the kit does not depend on the original track.
- **Sync slices to tempo** (only with File): turns on **Sync to tempo** on the new cells. The Source BPM is saved in each.
- Switching between Cell and File, the markers and the zone of each source stay as you left them. The external file and its zone are not saved in the kit: they are lost when you close the app (the cells created stay).

## Tempo of the source

The Sync of the slices stretches each cell from the **original tempo** to the BPM of the project, so the right BPM of the source is needed. It is set once for the chosen zone: all the slices inherit it (each slice is never analysed on its own).

- **Source BPM**: the value (drag, type or use − and +). With a cell it is the same **Loop BPM** of the cell panel.
- **Detect**: estimates the tempo from the **regularity of the hits** in the zone (not from the length), searching between 70 and 170 BPM, with a preference for tempos near 120. About 2 seconds of rhythm at least are needed; on tracks without clear hits it can be wrong.
- **/2** and **x2**: correct a tempo found at double or half.
- **Tap**: tap the tempo while listening (Audition on, click on a slice or Chop); after 2 taps it calculates the average. If you wait more than 2 seconds it starts again.
- **Click test**: plays the zone with a **click on every beat** at the Source BPM, starting from the **first marker**, with an accent every 4 beats. If the click stays on the beat from beginning to end the tempo is right; if it drifts, adjust the BPM or choose a shorter zone (in live tracks the tempo is not fixed: it is better to work on short zones). A white cursor runs on the waveform; it stops by itself or with **Stop test**.

Advice for a track: choose the zone **in whole bars** starting from a strong hit, press **Detect**, check with **Click test**, then cut (Detect, Even, Chop or by hand) and **Distribute**.

## Chop: cutting while listening (MPC style)

The **Chop** button (CUT row) plays the region from start to end and lets you place the cuts **in time**, while you listen:

1. Press **Chop**: the existing markers are removed (only the start stays), the loop starts and a **white cursor** runs on the waveform.
2. Every time you press **M** on the computer keyboard, or **any note** of the MIDI keyboard, a marker is placed where the cursor is. During Chop the MIDI notes do not play the cells: they are only used for cutting.
3. The Chop ends by itself at the end of the region, or by pressing the button again (**Stop chop**). Changing tab or cell stops it.
4. Refine the markers by hand (drag, double click, right click) and press **Distribute to cells**.

The sound comes out with the delay of the audio device: if you always cut a little late, press the key slightly earlier or move the markers afterwards.

## What happens to the cells

- Each cell receives the **same file** of the loop (no copy of the data: it does not take extra memory) with its own **Start/End** of the beginning and the end of the slice. The name is that of the loop plus a number (`amen 01`, `amen 02`…).
- With a **cell** as the source the slices **inherit the tempo of the loop**: Loop BPM (estimated if the original cell did not have one), Sync and Key lock. If you turn on **Sync to tempo** on the slices (better selecting them all: the options apply to all the selected cells) each follows the BPM of the project while staying in its place in the loop; the processing is a single one, shared. **Do not press Detect** on a single slice: it would estimate the tempo from its length (a single hit) and be wrong; leave the BPM of the loop.
- The cells are normal cells: each has its own MIDI key (from **C1** upwards, as always), its envelope, the effects, and can be added to the sequencer. To refine a slice use Start/End in the Main tab.
- The markers stay saved with the kit: reopening the Slice tab on the same cell you find the cuts again.
- If the slices are more than the free cells at the end of the grid (48 in total), only the available ones are filled and the text at the bottom warns you.

## Limits

- Hit detection works well on percussion with clear attacks (drums, rhythmic loops). On sustained or very dense material (pads, full music) it finds many points or few: lower or raise **Sensitivity** and correct by hand.
- The cuts fall at the point where the sound rises; there is not yet alignment to the tempo grid.
- The tab works on one cell at a time. If you make a mistake in a distribution, undo it with **Ctrl+Z**.
