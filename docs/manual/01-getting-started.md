# 1. Getting started

## What you need

- Windows 10/11, 64-bit (the macOS version is planned but not tested yet).
- For the plug-in: a DAW that loads VST3, such as Cubase.
- For the standalone: an audio device (the computer's own will do). The standalone supports the **ASIO** drivers of your audio interface, as well as WASAPI and DirectSound.

## Installation

Use the installer **`NINA-Setup-<version>-win-x64.exe`** (download it from the GitHub *Releases*). It contains **everything you need**: there is nothing else to download (not even the "Visual C++ Redistributable"), and ASIO support is already inside the standalone.

1. Run the installer. It opens in the language of Windows (Italian or English; you can change it from the first menu).
2. Choose the **components**: **NINA Standalone**, **NINA VST3** (for Cubase and other DAWs), **manual and licences**. All are selected by default.
3. Choose **who to install for**: **all users** (asks for administrator permission; plug-in in `C:\Program Files\Common Files\VST3\NINA.vst3`, standalone in `C:\Program Files\NINA`) or **only me** (no administrator needed; plug-in in `%LOCALAPPDATA%\Programs\Common\VST3\NINA.vst3`, standalone in `%LOCALAPPDATA%\Programs\NINA`).
4. The standalone gets a shortcut in the **Start menu** (and on the desktop, if you want).
5. In **Cubase**: rescan the plug-ins (*Studio → VST Plug-in Manager → Update*): NINA appears among the instruments.

**Updating**: download the new installer and run it: it **replaces** the previous version and keeps your settings and kits. **Uninstalling**: *Settings → Apps → NINA*, or the "Uninstall NINA" shortcut in the Start menu; your kits, sequences and songs are not touched.

**ASIO and drivers.** NINA can use the ASIO driver of your audio interface, but the drivers are supplied by the manufacturer of the interface. Without an ASIO driver NINA plays through the Windows audio system (WASAPI).

**For developers**: the installer is built with `installer\build-installer.ps1` (see the README).

## First launch of the standalone

1. Start the app. If you hear nothing, open **Options** at the top left and choose the audio output device (and, if you want to play with a keyboard, the MIDI input device).
2. The standalone **always starts empty**: kits, sequences and songs are not restored from the previous session (open them again with **Kit**). The window size, the audio settings and the list of recent kits are kept.

## The window at a glance

```
┌────────────────────────────────────────────────────────────────────────────────┐
│ NINA   [<] kit name [>] [Save] [Kit]            [Play] [BPM] [Size] [i]         │ top bar
├────────────┬──────────────────────────────────────────────┬────────────────────┤
│ File       │  4×12 grid of cells (pads)                   │ Panel of the       │
│ browser    │  ─────────────────────────────────────────── │ selected cell      │
│            │  Waveform with Start / End                   │ (mode, notes…)     │
├────────────┴──────────────────────────────────────────────┴────────────────────┤
│ Modules: AMP · PITCH · MASTER   (or the FX tab, or the sequencer)               │
├────────────────────────────────────────────────────────────────────────────────┤
│ Main   Seq   Song   Mixer   FX   Slice                                           │ tabs
└────────────────────────────────────────────────────────────────────────────────┘
```

- **Top bar:** kit name, arrows for the previous/next kit, **Save**, the **Kit** menu, the sequencer's **Play/Stop**, **BPM**, **Size** (enlarges or reduces the window in percentages) and the **i** button: it opens the **Info** window with the logo, the version, the copyright, the ASIO logo and the links to the **source code** and the **licences** (see chapter 12), the **Check for updates** button (it asks GitHub for the latest published version, **only when you press it**: if there is a newer one it takes you to the download page), and the **Support my work** button with the PayPal logo: NINA is free and identical for everyone, and a donation is **free and voluntary** (it opens PayPal in the browser; nothing changes in the program).
- **Tabs at the bottom** (buttons with an icon; the one of the open tab is lit): **Main** (cells and modules), **Seq** (sequencer), **FX** (effects), **Song** (chain patterns into a track), **Slice** (cut loops onto the cells), **Mixer** (the console: one channel per cell with fader and meter).
- The window can be **resized** by dragging its edges: all elements adapt.

## Your first beat in 5 minutes

1. **Load the samples.** In the browser on the left choose a folder (the menu at the top picks Home, Desktop, Documents, Music or a drive). Selecting a file **plays it right away** (*Preview* switch at the bottom). Drag it onto a pad, or double-click to load it into the selected pad.
2. **Play the pads.** One click on a pad plays it. With a MIDI keyboard the notes start from **C1** for pad A1 and go up pad by pad.
3. **Add the pads to the sequencer.** Right click on a pad with a sample → **Add to sequencer**. Repeat for the pads you need (or, in the Seq tab, **+ Cell** → **Add all loaded cells**).
4. **Draw the pattern.** Open the **Seq** tab: every added pad is a row. Click the steps to switch them on.
5. **Press Play** (or the space bar).
6. **Save the kit** with **Save**: a `.slkit` file is created together with a folder of the same name containing the samples.

For the details of each part, continue with the next chapters.
