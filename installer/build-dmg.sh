#!/bin/bash
# Crea NINA-<versione>-mac-universal.dmg (Standalone + VST3 + AU).
# Uso:  installer/build-dmg.sh
# Firma/notarizzazione opzionali:
#   SIGN_ID="Developer ID Application: Nome (TEAMID)"  NOTARY_PROFILE=nome-profilo  installer/build-dmg.sh
# (profilo creato con: xcrun notarytool store-credentials nome-profilo)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# La build va FUORI da cartelle sincronizzate con iCloud (Documents/Desktop), altrimenti il codesign fallisce.
BUILD="${BUILD:-$HOME/nina-build}"
VER=$(grep -m1 -E '^project\(NINA' "$ROOT/CMakeLists.txt" | grep -oE 'VERSION [0-9.]+' | cut -d' ' -f2)
cmake -S "$ROOT" -B "$BUILD" -G Xcode >/dev/null
cmake --build "$BUILD" --config Release --target Nina_Standalone Nina_VST3 Nina_AU -j 8
ART="$BUILD/Nina_artefacts/Release"
STAGE="$(mktemp -d)/NINA"; mkdir -p "$STAGE"
cp -R "$ART/Standalone/NINA.app" "$ART/VST3/NINA.vst3" "$ART/AU/NINA.component" "$STAGE/"
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
cat > "$STAGE/LEGGIMI.txt" <<TXT
Installazione:
- NINA.app          -> /Applications
- NINA.vst3         -> ~/Library/Audio/Plug-Ins/VST3
- NINA.component    -> ~/Library/Audio/Plug-Ins/Components
TXT
if [ -n "${SIGN_ID:-}" ]; then
  for b in "$STAGE/NINA.app" "$STAGE/NINA.vst3" "$STAGE/NINA.component"; do
    codesign --force --deep --options runtime --timestamp --sign "$SIGN_ID" "$b"
  done
fi
OUT="$ROOT/installer/Output"; mkdir -p "$OUT"
DMG="$OUT/NINA-$VER-mac-universal.dmg"; rm -f "$DMG"
hdiutil create -volname "NINA $VER" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
if [ -n "${SIGN_ID:-}" ]; then
  codesign --sign "$SIGN_ID" --timestamp "$DMG"
  if [ -n "${NOTARY_PROFILE:-}" ]; then
    xcrun notarytool submit "$DMG" --keychain-profile "$NOTARY_PROFILE" --wait
    xcrun stapler staple "$DMG"
  fi
else
  echo "ATTENZIONE: DMG non firmato (solo uso personale; Gatekeeper lo bloccherà su altri Mac)."
fi
echo "Creato: $DMG"
