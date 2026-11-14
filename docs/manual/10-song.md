# 10. Song (Pattern Sequencer)

The **Song** tab lines up the patterns you created in the **Seq** tab to form a whole track: intro, verse, chorus... Each piece of the chain is a **block** (a pattern played a certain number of times). The blocks play one after the other.

The **patterns stay in the kit** and are edited in the Seq tab: if you change a pattern, it also changes everywhere it is used in the song.

## Building the song

1. In the **Seq** tab prepare the patterns (up to 16).
2. Open the **Song** tab and click **+ Block**: choose the pattern to add at the end of the chain. The pattern active in the Seq tab is marked.
3. Each block is a card with:
   - the **pattern** (drop-down menu: you can change it whenever you want);
   - the **repeats** (**−/+** or drag the number): one "round" is one cycle of the pattern, as long as its longest row (usually one bar of 16 steps at 1/16). At the top right of the card you see the duration in bars;
   - **<** and **>** to move the block earlier or later;
   - **Dup** to duplicate it (the duplicate goes right after);
   - **x** to remove it.
   The blocks **wrap** onto several rows when they do not fit in one: how many fit per row depends on the width of the window. If the rows become too many for the available height, the cards **shrink by themselves** (down to a size that is still readable); with an even larger number a **vertical scroll bar** appears. When you add a block the view scrolls to the last one.
4. Under the blocks, the **song map** shows the whole chain to scale: each block is coloured like its pattern, with pattern number and repeats, and there is the **total length** (bars, minutes:seconds at the current BPM).
5. The **⋯** button next to **+ Block** offers **Add every pattern that has notes** (one block for each pattern with rows, in order) and **Clear the song**.

The **name** of the song (field next to Save song) is used for the names of the exported files.

## Playing the song

- **Play** (top bar or space bar) is tied to the **open tab**: in the **Song** tab it plays the **chain from the start**; in all the others (Main, Seq, FX...) it plays as always the **pattern** selected in the Seq tab. In the Song tab the text «Play here plays the song» reminds you. If you change tab while it is playing, playback continues as it was; the new mode applies from the next Play.
- **Starting from a block**: click a block card (on the title or on an empty space, not on the menus and buttons): its title lights up with the text **Play from here** and **Play** starts from that block instead of from the beginning. A second click on the same block removes the selection (Play from the start). With Loop on, at the end the song starts again from the first block.
- While listening the block that is playing lights up with a progress bar and the cursor runs on the map.
- **Loop** on: at the end the song starts again from the first block. Off: at the end Play stops (and the next Play starts again from the beginning).
- In the **plug-in**, when the song is the one playing, the position follows the DAW: the song starts from the zero point of the project. With Loop on it repeats when the DAW passes the end.
- Each pattern always starts from the beginning when its block begins. Notes still held (Gate or Loop mode) are closed at the end of each block.
- While the song plays you can edit the patterns in the Seq tab: the change is heard immediately.

## Saving and opening the song

- **Save song** writes a **.slsong** file (readable text) with only the chain of blocks, the name and the Loop. The file proposes the `Songs` folder next to the kit.
- To **open** a song use **Kit → Open...** and choose the `.slsong` file: the chain replaces the current one.
- The song is **not in the kit file** (the kit contains only the sounds): it is its own `.slsong` file. It is useful to keep several versions or exchange chains between projects: the blocks refer to the patterns by number (1–16), so they work with the patterns you have loaded (`.slseq` file, see chapter 5).
- The song can be undone with **Ctrl+Z** like the rest (adding, moving, removing blocks, repeats, Loop).

## Exporting the song

The same commands as in the Seq tab, but on the whole chain. The length is that of the song (there is no need to choose the bars).

| Command | What it does |
|---------|--------------|
| **Export MIDI** | The whole song in a single MIDI clip. Ratchet, nudge, swing and probability are already resolved into notes; reverse and effect locks do not exist in MIDI |
| **Bounce WAV** | The whole song in a single 24-bit audio file, with effects, locks, synced delay and master effects |
| **Export Lanes** | One WAV file for each cell played in the song (all of the same length), with the effects of the cell but not the master ones |
| **Drag MIDI / Drag audio / Drag lanes** | Like the three previous commands, but dragging directly onto the tracks of the DAW |
| **Tail** (cut / loop / keep) | How to treat the tails of delay and reverb: cut at the end of the song, wrapped to the beginning (for a loop) or kept (longer file). It is the same setting as in the Seq tab |

The tempo used is the current one (DAW or BPM box). As for the sequencer, the exports use the **current** values of the controls, not the automation of the DAW.

## Things to know

- The song has at most **128 blocks**, each with **1–99** repeats.
- A pattern without rows plays as silence for the duration of one bar.
- The duration of a round is that of the **longest** row of the pattern: a shorter row repeats inside the round.
- There is not yet a loop of a single part of the song (only the whole chain, with **Loop**).
- A song made of different patterns does not change the BPM: the tempo stays single for the whole track.
