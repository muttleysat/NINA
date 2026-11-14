# 3. Effects

Open the **FX** tab at the bottom. At the top left choose **what** to apply the effects to:

- **Cell A1 …** (the name of the selected cell): the effects of that cell.
- **Master**: the effects on the whole kit, after all the cells.

Each effect is a **card** with a switch at the top right (on/off) and its knobs. The cards are laid out on **two rows**, in the order of the chain (the FX tab uses the whole width: in this tab the browser, the cell panel and the waveform are hidden, and the grid stays at the top to choose the cell). The knobs can be adjusted even when the effect is off: the values stay stored.

## The effects

| Effect | Controls | What it does |
|--------|----------|--------------|
| **Saturation** | Drive (0–36 dB), Mix | Soft distortion (saturation) |
| **LoFi** | Bits (1–16), Rate (1–40×), Mix | Reduces the bit depth and the sample rate: a "dirty" digital sound |
| **Filter** | Type (Lowpass/Highpass/Bandpass), Cutoff (20 Hz–20 kHz), Reso | Resonant filter |
| **EQ** | Low (±15 dB), Mid Hz (100 Hz–8 kHz), Mid dB (±15 dB), Mid Q (0.3–8), High (±15 dB) | Three-band equaliser, with the **response curve** drawn above the knobs (20 Hz–20 kHz, ±18 dB; it updates while you turn the knobs, also with automation): **Low** is a *shelf* (raises or lowers everything below 120 Hz), **Mid** is a *bell* (choose the frequency, how much to raise or lower it and how narrow it is with **Q**), **High** is a *shelf* above 6 kHz |
| **Compressor** | Thresh, Ratio, Attack, Release, Makeup | Compressor |
| **Stutter** | Division (1/1…1/64), Decay, Mix | **Tempo-synced** *beat repeat*. When on, it works like this: **at every new hit of the cell** it records the portion of sound that starts there, as long as the chosen division (1/16 at 120 BPM = 125 ms); meanwhile the hit is heard normally, then the portion **repeats in a cycle** until another hit arrives or you switch it off. If you turn it on while a **loop or a break** is already playing, it immediately repeats the last portion of what you were hearing. **Decay** lowers each repetition relative to the previous one (0 = all equal), **Mix** balances repetition and normal sound (1 = only the repetition). On the **Master** it applies to the whole kit (new hits of any cell re-arm it) |
| **Chorus** | Rate (0.05–5 Hz), Depth, Mix | Doubles the sound with a slightly delayed copy (about 15 ms) whose pitch sways: a "wide" effect, fuller and more stereo. **Rate** is the speed of the oscillation, **Depth** how much it sways, **Mix** the amount of the copy |
| **Flanger** | Rate, Depth, Fdbk (feedback, 0–0.9), Mix | A copy with a very short delay (0.2–2.8 ms) that moves: it creates the characteristic comb "whoosh" that rises and falls. **Fdbk** sends the output back into the effect and makes it more metallic and resonant |
| **Phaser** | Rate, Depth, Fdbk, Mix | Four all-pass filters that move between 200 Hz and 4 kHz, summed with the original sound: a "swirling" effect, softer than the flanger. **Fdbk** increases the depth of the notches |
| **Delay** | Sync, musical value, Time, Feedback, Mix | Echo |
| **Reverb** | Size, Damp, Mix | Reverb |

## How the effects of a cell work

Order of the chain: Saturation > LoFi > Filter > **EQ** > Compressor > **Chorus > Flanger > Phaser** > Delay > Reverb > **Stutter** (Stutter is last: it repeats what you hear from the cell, tails included). Stutter repeats at most 0.75 seconds: at very slow tempos the longest divisions are shortened.

Understanding this avoids surprises, especially in the sequencer.

1. **Saturation, LoFi and Filter** work **on each single note**: each hit has its own filter. Two close notes do not influence each other.
2. **EQ**, **Compressor**, **Chorus**, **Flanger** and **Phaser** work on the **sum** of all the notes of the cell (in the order of the chain). The sends to the delay and the reverb leave before the EQ. The EQ cannot be locked by the sequencer steps.
3. **Delay and Reverb** are **sends**: each note sends an amount of its sound to the effect, and the effect adds the echo or the reverb. The **Mix** knob is the amount sent by default. The echo of a note **keeps sounding** after the end of the note (up to about 4 seconds after the last send).

If an effect is off on the cell, that cell does not use it, **except** for the sequencer steps that turn it on with a *lock* (chapter 4).

## Tempo-synced delay

In the Delay card:

- **Sync** on: the echo time follows the **BPM** (of the DAW, or the internal one of the standalone) and you choose it as a musical value from the menu: from **1/1** to **1/64**, also dotted (**1/8.**) and triplets (**1/8T**).
- **Sync** off: the time is set in milliseconds with the **Time** knob (up to 2000 ms).

When the tempo of the project changes, the echo adapts with a smooth transition ("tape" effect).

> The BPM of the standalone is typed in the box at the top (double click). In the plug-in it comes from the DAW and cannot be edited.

## Master effects

With **Master** selected the same effects act on the final mix: Saturation, LoFi and Filter on the overall signal, then Compressor; **Delay and Reverb** send the mix (with **Mix** as the amount) and add echo and reverb.

## Tips

- For a filter that works only on some hits (for example a cutoff that closes on certain steps), turn the Filter off on the cell and use the *locks* of the steps in the sequencer.
- For an echo on some hits only: leave the Delay **off** on the cell, set Time/Feedback once (also from the step panel) and move **Send** on the step.
- If a sound becomes too loud with saturation, lower the **Level** of the cell or use the compressor's **Makeup** with care.
