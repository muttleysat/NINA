# Contributing

NINA is a personal project and, for now, **I am not accepting pull requests**: the direction of
the program and its code are decided by the author. Thank you for understanding.

What is welcome:

- **Bug reports**: open an issue with the NINA version (Info window), your Windows version, the
  DAW (for the plugin) and the steps to reproduce the problem. Attach the kit, sequence or song
  file if it helps (remove anything private first).
- **Ideas**: open an issue describing what you would like to do with NINA, not how to code it.
- **Security problems**: please follow [SECURITY.md](SECURITY.md) instead of opening an issue.

You are free to **fork** the project and modify it under the terms of the GNU AGPL-3.0 (see
[LICENSE](LICENSE)): the source code is yours to study and change. The **name, logo and icon of
NINA** are not covered by the licence: please use a different name and logo for a modified
version.

## Building

See [README.md](README.md). In short: JUCE and Rubber Band are downloaded by CMake, the Windows
standalone and the VST3 are built with Visual Studio Build Tools, and the tests
(`NinaTests`) run from the command line.
