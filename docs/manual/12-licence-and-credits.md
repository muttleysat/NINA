# 12. Licence and credits

## The licence of NINA

NINA is **free software**, released under the **GNU Affero General Public License, version 3 (AGPL-3.0)**. The full text is in the `LICENSE` file of the project and in the **Info → Licenses** window of the program.

In practice:
- you can **use, copy and distribute it free of charge**;
- the **source code** is available to anyone who receives the program (**Info → Source code** window);
- whoever modifies and distributes it must release their changes under the same licence.

The **name, logo and icon of NINA** belong to the author (Vinz Trombetta) and are not covered by the licence of the code.

## The libraries used

NINA is built with third-party components, each with its own licence (full list in the `THIRD_PARTY_NOTICES.md` file and in **Info → Licenses → Third-party notices**):

| Component | What it is for | Licence |
|-----------|----------------|---------|
| **JUCE** | Interface, plug-in and reading/writing of audio files | AGPL-3.0 |
| **Rubber Band Library** | Stretching of loops (Sync to tempo, Key lock, slices) | GPL 2 or later |
| **Steinberg VST 3 SDK** | The plug-in format | GPL 3 |
| **Steinberg ASIO SDK** 2.3.4 | The ASIO audio drivers of the standalone (Windows only) | GPL 3 (dual licence with the proprietary Steinberg one) |

## Trademarks

- **VST is a trademark of Steinberg Media Technologies GmbH**, registered in Europe and other countries.
- **ASIO is a registered trademark and software of Steinberg Media Technologies GmbH.** The Windows standalone supports ASIO drivers:

  ![ASIO Compatible](../img/asio_compatible_logo.jpg)

- The **PayPal** logo is a trademark of PayPal, Inc. and appears only on the voluntary donation button.

## Versions and updates

The version number (for example **1.0.0**) is in the **Info** window. New versions are published in the GitHub **Releases** (`github.com/muttleysat/NINA/releases`): to update, download the new installer and run it; it **replaces** the previous version keeping your settings. The **Check for updates** button in the Info window checks whether a newer version exists. The program **never connects by itself**: the request is made only when you press the button.

## Donations

NINA is free and **the same for everyone**. If you like it, in the **Info** window the **Support my work** button (with the PayPal logo) opens the page for a **free and voluntary donation**: no obligation, and nothing changes in the program.
