# Third-party notices

NINA is free software released under the **GNU Affero General Public License, version 3**
(see the `LICENSE` file). It is built on the following third-party components, which keep
their own licences and copyrights.

## JUCE 8.0.6

- Website: <https://juce.com>
- Copyright (c) Raw Material Software Limited.
- Licence used here: **GNU AGPL v3** (JUCE is dual-licensed AGPLv3 / commercial).
- JUCE includes, and NINA is built with, third-party code under permissive licences:
  FLAC and Ogg Vorbis (BSD-style, Xiph.Org Foundation), zlib (zlib licence), libpng
  (libpng licence) and libjpeg (Independent JPEG Group licence). Their notices are in the
  JUCE source tree (`modules/juce_audio_formats/codecs`, `modules/juce_graphics/image_formats`).

## Rubber Band Library 4.0.0

- Website: <https://breakfastquay.com/rubberband/>
- Copyright (c) 2007-2024 Particular Programs Ltd (trading as Breakfast Quay).
- Licence: **GNU General Public License, version 2 or later**. Used for the time-stretching
  of loops (Sync to tempo, Key lock, slices).
- It contains KISS FFT (BSD), the Speex resampler (BSD) and SIMD math code by Julien
  Pommier (zlib licence).

## Steinberg VST 3 SDK (included in JUCE)

- Copyright (c) Steinberg Media Technologies GmbH.
- Licence used here: **GNU General Public License v3** (the SDK is dual-licensed with the
  proprietary Steinberg VST 3 licence).
- **VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and
  other countries.**

## Steinberg ASIO SDK 2.3.4 (Windows standalone only)

- Website: <https://www.steinberg.net/asiosdk>
- Copyright (c) 2025 Steinberg Media Technologies GmbH.
- Licence used here: **GNU General Public License v3** (the SDK is dual-licensed with the
  proprietary Steinberg ASIO licence).
- **ASIO is a registered trademark and software of Steinberg Media Technologies GmbH.**
- ASIO driver support is compiled into the Windows standalone only when the SDK is present
  at build time. The SDK itself is not part of this repository (download it from the address
  above). The "ASIO Compatible" logo is supplied by Steinberg, is used unaltered and as required
  by the Steinberg ASIO usage guidelines.

## PayPal

- The PayPal logo is a trademark of PayPal, Inc. It appears only on the voluntary
  donation button in the About window; NINA has no other connection with PayPal.

## NINA

- Copyright (C) 2026 Vinz Trombetta. The NINA name, logo and icon are not covered by the
  AGPL licence of the source code.
