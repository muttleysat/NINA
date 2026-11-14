# 11. Mixer

The **Mixer** tab is a console like the one in Cubase: each cell with a sample has its own **channel**, and on the right there is the **master**. It is used to balance the kit at a glance, listen to one cell alone (Solo), silence another (Mute) and see who is loudest with the digital **meters**.

All the controls are the same parameters as in the other tabs (Level, Pan, delay and reverb Mix...): moving a fader here also moves the Level knob of the cell in the Main tab, and vice versa. They can all be automated by the DAW and have **MIDI learn** (right click on the control).

## The channel of a cell

From top to bottom:

| Element | What it does |
|---------|--------------|
| **Output** (menu, usually *Main*) | The output of the plug-in on which the cell plays: Main (the mix) or Out 2...Out 16 to send it to a separate track of the DAW (as in the cell panel) |
| **FX: S L F C D R E** | The effects of the cell: **S**aturation, **L**oFi, **F**ilter, **C**ompressor, **D**elay, **R**everb, **E**Q. One click turns them on and off (on = orange). The parameters of each effect stay in the FX tab |
| **Dly / Rev** | The two sends: how much of the signal of the cell goes to the delay and the reverb. Raising them with the mouse, the effect turns on by itself. If the effect is off, the knob is dimmed (the send is not heard) |
| **PAN** | The stereo position, from L100 to R100. Double click: centre |
| **M / S** | **Mute** (the cell does not play) and **Solo** (only the cells in Solo play). You can have several cells in Solo. They light up in orange |
| **Fader** + dB scale | The volume of the cell (Level), from −60 to +12 dB. Double click: 0 dB. The mouse wheel moves it slightly; Shift slows the drag |
| **Meter** | Two bars (left and right) with green, yellow and red segments, scale −60…+6 dB. The white line is the **peak**, which stays for about a second. At the top of each bar there is the **clip** light: it turns red above 0 dB and stays on until you click the meter |
| **dB value** | The level of the fader. Double click to type a value by hand |
| **Peak** | The highest peak since the last reset (red if it exceeded 0 dB). A click on the meter resets it |
| **Name** | Bar in the colour of the cell with the label (A1...) and the name. A click selects the cell (and shows it in the Main tab) |

The selected cell has an orange border. The meters show the signal **after** the effects of the cell and its fader; a cell in Mute (or excluded by a Solo) shows zero.

## The master

On the right, in orange: the **master volume** of the kit (fader), its **meter** with peak and clip. It is the same "Volume" control of the Main tab. After the master there is the chain of master effects (FX tab).

## Top bar

- **All cells**: also shows the empty cells (normally only those with a sample are shown).
- **Reset mutes** / **Reset solos**: turn off all the Mutes or all the Solos.
- With many cells the row of channels scrolls horizontally; the master stays fixed on the right.

## Mute and Solo: what they change

- They apply to **all the audio of the cell**: sequencer, MIDI keys, clicks on the pad, previews.
- They are **not** the Mute/Solo of the rows of the sequencer (those concern only the notes of the pattern): they are independent and can be used together.
- They are **saved in the kit** and can be undone with Ctrl+Z. They can be automated by the DAW ("Mute" and "Solo" parameters of each cell).
- **Bounce WAV** (sequencer and song) respects the mixer's Mute and Solo, like a muted track in the DAW. **Export lanes** and the separate tracks do not: each cell is always written, otherwise you would have empty files.
- Mute does not stop the samples: the cell keeps "running" in silence, so turning it back on in the middle of a loop does not lose the timing.

## Things to know

- The meters are **peak** meters (not RMS/loudness): they are used to avoid clipping and to see the ratios between the cells.
- The extra outputs (Out 2...) exist only in the plug-in, if the DAW has activated them; in the standalone everything goes to the Main.
- There is not yet a panel for **groups of channels** (buses), nor **pre/post-fader** sends: the sends are on the signal of the cell before its fader.
