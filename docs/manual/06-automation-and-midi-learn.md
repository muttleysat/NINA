# 6. Automation and MIDI learn

## Automation from the DAW (VST3 plug-in)

All the controls of NINA are **parameters that the DAW can automate**, for reading and writing, with **readable names** (not CC numbers).

### How they appear in the DAW

Each cell has its own parameters, arranged in groups. For example:

- `A1 Level`, `A1 Pan`, `A1 Tune`, `A1 Pitch`, `A1 Start`, `A1 End`, `A1 Reverse`, `A1 Trigger Mode`
- Envelope: `A1 Attack`, `A1 Hold`, `A1 Decay`, `A1 Sustain`, `A1 Release`
- Effects: `A1 Saturation Drive`, `A1 Filter Cutoff`, `A1 Filter Type`, `A1 Delay Time`, `A1 Delay Sync`, `A1 Delay Division`, `A1 Reverb Mix`… and the switches `A1 Delay On`…
- Master: `Master Level` and the same effects with the prefix `Master`.

There are **38–40 parameters per cell** for the 48 cells, plus those of the master: about 1,950 in total. The **values have units and text**: "2400 Hz", "Gate", "On", "1/8".

The parameters are grouped by cell and effect (for example *Cell A1 → A1 Filter → …*), so they are ordered and searchable in the automation menu.

### Recording automation

Moving a control of NINA while the DAW is recording automation (Write, Touch, Latch), the movement is recorded. Moving a knob tells the DAW the start and the end of the gesture. The recorded automation is **played back** by moving the knobs.

### Things to know

- **The parameter IDs are stable**: they will not change between versions, so the automation in your projects does not break.
- Automation also acts in the DAW's **offline export**.
- NINA's **Bounce WAV** and **Export lanes** use the **current** values of the controls, **not** the automation of the DAW.
- The lock values of the steps (chapter 4) are not DAW parameters: they are inside the pattern.
- The note mapping (Root, Key low, Key high) cannot be automated.

> **Verification status:** the behaviour of names, groups and recording in **Cubase 16** has not been tried yet; it has been verified with automated tests. If something does not appear as described, please report it.

## MIDI learn

MIDI learn connects a knob or a fader of your MIDI controller to a control of NINA.

### Assigning

1. **Right click** on the control (knob, switch, menu...).
2. Choose **MIDI Learn**: the control is highlighted with "MOVE CC...".
3. **Move the knob** of the controller: the first time it sends a value, the assignment is made. The label **CC74** (the number of your controller) appears on the control.

### Removing

Right click on the control → **Clear MIDI mapping**. To cancel a learning in progress, choose the item again in the menu.

### How it behaves

- The control follows the controller **over the whole range** (0–127). **Switches** turn on from 64 upwards; **choice menus** (for example the filter type) pick the item according to the value.
- A parameter has **only one** CC; the same CC can move **several parameters**.
- The assignments are **saved with the kit** (`.slkit`) and in the DAW project.
- In the plug-in the CCs come from the DAW (for example from a MIDI track that sends them to the instrument); in the standalone they come from the MIDI device chosen in **Options**.
- MIDI learn applies to the controls of NINA; the notes that play the cells (chapter 2) are separate.

### Not available yet

Pitch bend and aftertouch, MIDI channel filter, relative mode of encoders, "soft takeover" (the control jumps to the value of the controller) and a panel listing all the assignments.
