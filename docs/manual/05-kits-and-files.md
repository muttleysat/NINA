# 5. Kits and files

A **kit** contains all the cells with their samples and settings, the master effects, the 16 sequencer patterns and the MIDI learn assignments.

## The kit format

When you save a kit with the name *MyKit*, **two items side by side** are created:

```
Kits/
├── MyKit.slkit      ← the kit file (readable JSON text)
└── MyKit/           ← folder with the same name: the samples used, copied
    ├── Kick 1.wav
    └── Snare 2.wav
```

- The **name of the kit** is always that of the file: saving with another name (Save as) changes the name.
- The **samples are normal audio files**: if you need one of them you take it straight from the folder.
- In the `.slkit` file the samples are written with **relative** paths: you can move or copy file and folder together.
- The edits (Start/End, effects, levels…) are saved in the `.slkit` and **never alter** the audio files.

## Three kinds of file: kit, sequences and song

The **kit** is the heart of the program, and the main save (**Save**) writes **only the kit**. Sequences and song each have their own file and their own save button:

| File | What it contains | How to save it |
|------|------------------|----------------|
| **`.slkit`** (kit) | Cells and samples (copied into the folder), all the parameters, per-cell effects, master, MIDI learn. **No** sequences or song | **Kit → Save kit** / **Save kit as...** menu (or the **Save** button at the top) |
| **`.slseq`** (sequences) | The **16 patterns**: rows, steps, effect locks, swing, names, active pattern | **Save seq** button in the **Seq** tab (second row of controls) |
| **`.slsong`** (song) | The chain of blocks, the Loop and the name | **Save song** button in the **Song** tab |

All three are opened with **Kit → Open...**: it is a single command and it recognises the type from the extension:

- choosing a **`.slkit`** changes the sounds (if you have unsaved changes to the kit, it asks for confirmation) and the **sequences and the song you are using stay** as they were;
- choosing a **`.slseq`** replaces the 16 patterns with those of the file (it asks for confirmation, can be undone with Ctrl+Z);
- choosing a **`.slsong`** replaces the chain with that of the file.

Things to know:
- The **asterisk** next to the kit name concerns **only the kit**. Unsaved changes to sequences and song do not light it, but if you are about to lose them (for example with **New kit**) the app **asks for confirmation** and tells you.
- **New kit** creates an empty project: it also resets the sequences and the song (after confirmation, if they were not saved).
- **Old kits** saved with earlier versions, which also contain the patterns and the song, still open **in full** (with all their content); when you save them again with **Save kit** the patterns and the song are no longer written to the file: save them first with **Save seq** and **Save song** if you need them.
- The rows of the patterns and the blocks of the song refer to cells and patterns **by number** (A1 is cell 0...): they work with any kit, but they play whatever is in those cells.
- The **DAW project** (the plug-in inside Cubase) instead saves **everything together** (kit, sequences and song) in the project, as before: the three files are for reusing the parts between different projects.

## The Kit menu and the buttons at the top

| Command | What it does |
|---------|--------------|
| Kit name | Shows the name. An asterisk (`*`) means unsaved changes. "Untitled" = not saved yet |
| **< >** | Goes to the previous/next kit **in the same folder** as the current kit (active only if there is another one) |
| **Save** | Saves the **kit** (only the kit: the first time it asks for the name) |
| **Kit → New kit** | Creates an empty kit |
| **Kit → Open...** | Opens a `.slkit` (kit), `.slseq` (sequences) or `.slsong` (song) file: it recognises the type from the extension |
| **Kit → Save kit** / **Save kit as...** | Saves / saves with another name (only the kit) |
| **Kit → Normalize all cells** | Normalises the Level of all cells (peak at −1 dBFS) |
| **Kit → Recent kits** | The **last 5 kits** opened or saved, with **Clear recent** to empty the list |

If you have unsaved changes and open another kit or create a new one, the app **asks for confirmation**.

## Missing samples

If you open a kit and a sample cannot be found (for example you moved the folder), the cell shows **MISSING** in red. Put the folder back next to the `.slkit` file and reopen the kit.

## The DAW project

When you use the plug-in, the complete state (kit, parameters, MIDI assignments) is saved **inside the DAW project** and reloaded with it. To share or reuse a kit in other projects, also save it as a `.slkit`.

The standalone **always starts empty**: the state of the previous session is not restored. Reopen the kit, sequences and song you need with **Kit → Open...** (or from **Recent kits**).

## Where the exported files go

The exports of the sequencer (MIDI, audio) propose a **Bounces** folder next to the `.slkit` file; if the kit is not saved yet they propose the Documents folder. Files dragged directly into the DAW go through a temporary folder of the system.
