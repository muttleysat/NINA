[English](README.md) · **Italiano**

# NINA

Sampler/drum machine VST3 + standalone (Windows e macOS), ispirato a Native Instruments Battery, con sequencer a step. Uso personale.

- **Manuale d'uso:** [docs/manuale/README.md](docs/manuale/README.md) (primi passi, celle, effetti, sequencer, kit, automazione, esportazione)
- Requisiti: [docs/REQUISITI.md](docs/REQUISITI.md)
- Architettura: [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md)

## Schermate

![Main: griglia di celle, forma d'onda, parametri](docs/img/screenshot-main.png)
![Sequencer a step con parameter lock](docs/img/screenshot-sequencer.png)

| Mixer | Song |
|---|---|
| ![Mixer](docs/img/screenshot-mixer.png) | ![Song](docs/img/screenshot-song.png) |
| ![FX](docs/img/screenshot-fx.png) | ![Slice](docs/img/screenshot-slice.png) |

*Kit di esempio con suoni sintetici, generato dall'app stessa.*

## Download

Dall'ultima [Release](https://github.com/muttleysat/NINA/releases/latest): `NINA-Setup-<versione>-win-x64.exe` (Standalone con ASIO, VST3, manuale) per Windows e `NINA-<versione>-mac-universal.dmg` (Standalone, VST3, Audio Unit; Intel + Apple Silicon) per macOS.

### Installazione su macOS

La build per macOS **non è firmata né notarizzata** (progetto gratuito, senza account Apple Developer), quindi Gatekeeper la blocca al primo avvio.

1. Apri il `.dmg` e copia `NINA.app` in `/Applications`, `NINA.vst3` in `~/Library/Audio/Plug-Ins/VST3` e `NINA.component` in `~/Library/Audio/Plug-Ins/Components`.
2. Rimuovi il flag di quarantena (una volta sola):
   ```bash
   xattr -cr /Applications/NINA.app ~/Library/Audio/Plug-Ins/VST3/NINA.vst3 ~/Library/Audio/Plug-Ins/Components/NINA.component
   ```
   In alternativa, prova ad aprire l'app e poi vai in Impostazioni di Sistema > Privacy e sicurezza > **Apri comunque**.
3. Riavvia il DAW e ripeti la scansione dei plug-in.

## Build (macOS)

Servono Xcode e CMake (`brew install cmake`). Compila **fuori** dalle cartelle sincronizzate con iCloud (Documenti, Scrivania), altrimenti il codesign fallisce con "detritus not allowed":

```bash
cmake -S . -B ~/nina-build -G Xcode
cmake --build ~/nina-build --config Release
```

Output in `~/nina-build/Nina_artefacts/Release/` (`Standalone`, `VST3`, `AU`; universale arm64 + x86_64). `installer/build-dmg.sh` crea il `.dmg`; con un Developer ID, imposta `SIGN_ID` e `NOTARY_PROFILE` per firmarlo e notarizzarlo.

## Build (Windows)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output in `build/Nina_artefacts/Release/` (cartelle `VST3` e `Standalone`).

## Installer (Windows)

`installer\build-installer.ps1` costruisce `installer\Output\NINA-Setup-<versione>-win-x64.exe` con [Inno Setup 6](https://jrsoftware.org/isinfo.php) (`winget install JRSoftware.InnoSetup`). Prima compila il progetto in Release. L'installer contiene lo standalone (con ASIO), il plugin VST3, il manuale e le licenze; il runtime di Visual C++ è collegato staticamente, quindi non servono altre installazioni. La versione si legge da `CMakeLists.txt`. Le immagini della procedura guidata si rigenerano con `python installer/make_wizard_images.py`.

## Licenza

NINA è software libero sotto licenza **GNU AGPL-3.0** (vedi il file [LICENSE](LICENSE)); le librerie di terzi e i loro riconoscimenti sono in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Il nome, il logo e l'icona di NINA non sono coperti dalla licenza del codice. VST è un marchio di Steinberg Media Technologies GmbH.

## ASIO (solo standalone su Windows)

![ASIO Compatible](docs/img/asio_compatible_logo.jpg)



I driver ASIO richiedono l'SDK Steinberg (versione **2.3.4 o successiva**, in doppia licenza: contratto Steinberg oppure **GPL 3**, che è quella compatibile con NINA). L'SDK non è nel repository: si scarica a parte. Per abilitare ASIO:

1. Scarica l'ASIO SDK da <https://www.steinberg.net/asiosdk>.
2. Estrai il contenuto in `third_party/asiosdk/` (deve esistere `third_party/asiosdk/common/iasiodrv.h`).
3. Riesegui `cmake -S . -B build ...`: la configurazione stampa "ASIO SDK trovato".

Senza SDK lo standalone usa solo WASAPI/DirectSound. Nel VST3 la scheda audio è gestita dalla DAW, quindi ASIO non serve.

## Test

```powershell
cmake --build build --config Release --target NinaTests
build\NinaTests_artefacts\Release\NinaTests.exe
```

Coprono motore di riproduzione (mapping, velocity, level, start/end, pitch, reverse, one-shot/gate/loop, layer, voice stealing) e formato kit (salvataggio con cartella omonima, ricarica, sample mancanti, versione schema).

## Stato

Fase 3 (loop e tempo) avviata: per ogni cella **Sync to tempo**, **Key lock** e **Loop BPM** (stimato dalla durata): il loop segue il BPM della DAW senza cambiare intonazione (Rubber Band).

Sequencer (primo passo): scheda **Seq** con una riga per ogni cella aggiunta (tasto destro su una cella > "Add to sequencer"), step con velocity, probabilità, ratchet, nudge, gate, pitch e parameter lock sugli effetti; esportazione MIDI e bounce WAV. MIDI learn sui parametri (tasto destro su un controllo) e delay sincronizzato al BPM.

Fase 2 (effetti) avviata: scheda **FX** con sei effetti per cella e per il kit (Saturation, LoFi, Filter, Compressor, Delay, Reverb), ciascuno con interruttore; delay e riverbero continuano a suonare dopo la fine della nota.

Fase 1 (MVP sampler) in corso: griglia 4×12 con mapping da C1, browser dei file, drag & drop sui pad, editor di cella (forma d'onda con Start/End, AHDSR, Level, Pan, Tune, Pitch, modalità one-shot/gate/loop, reverse, range note), kit `.slkit` con cartella dei sample omonima, apri/salva/kit recenti, menu di ridimensionamento a percentuali. Mancano: verifica in Cubase 16 (comprese le uscite multiple per cella, fatte nel motore), build su macOS.
