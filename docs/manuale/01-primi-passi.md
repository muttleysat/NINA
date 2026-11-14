# 1. Primi passi

## Cosa ti serve

- Windows 10/11 (la versione per macOS è prevista ma non ancora provata).
- Per il plugin: una DAW che carichi i VST3, come Cubase.
- Per lo standalone: una scheda audio (anche quella del computer). Lo standalone supporta i driver **ASIO** della tua scheda, oltre a WASAPI e DirectSound.

## Installazione

Si usa il programma di installazione **`NINA-Setup-<versione>-win-x64.exe`** (si scarica dalle *Release* di GitHub). Contiene **tutto il necessario**: non devi scaricare nient'altro (nemmeno il «Visual C++ Redistributable»), e il supporto ASIO è già dentro lo standalone.

1. Avvia l'installer. Si apre nella lingua di Windows (italiano o inglese; si può cambiare dal menu iniziale).
2. Scegli i **componenti**: **NINA Standalone**, **NINA VST3** (per Cubase e le altre DAW), **manuale e licenze**. Per impostazione predefinita sono tutti selezionati.
3. Scegli **per chi installare**: **per tutti gli utenti** (chiede i permessi di amministratore; plugin in `C:\Program Files\Common Files\VST3\NINA.vst3`, standalone in `C:\Program Files\NINA`) oppure **solo per me** (senza amministratore; plugin in `%LOCALAPPDATA%\Programs\Common\VST3\NINA.vst3`, standalone in `%LOCALAPPDATA%\Programs\NINA`).
4. Lo standalone ha il collegamento nel **menu Start** (e, se vuoi, sul desktop).
5. In **Cubase**: riavvia la scansione dei plugin (*Studio → Gestione plugin VST → Aggiorna*): NINA compare tra gli strumenti.

**Aggiornare**: scarica il nuovo installer e avvialo: **sostituisce** la versione precedente, mantenendo le impostazioni e i kit. **Disinstallare**: *Impostazioni → App → NINA*, oppure il collegamento «Disinstalla NINA» nel menu Start; non tocca i tuoi kit, sequenze e song.

**ASIO e i driver.** NINA può usare il driver ASIO della tua scheda audio, ma i driver li fornisce il produttore della scheda. Senza un driver ASIO NINA suona tramite l'audio di Windows (WASAPI).

**Per sviluppatori**: l'installer si costruisce con `installer\build-installer.ps1` (vedi il README).

## Prima apertura dello standalone

1. Avvia l'app. Se non senti niente, apri **Options** in alto a sinistra e scegli il dispositivo audio di uscita (e, se vuoi suonare con una tastiera, il dispositivo MIDI in ingresso).
2. Lo standalone **parte sempre vuoto**: kit, sequenze e song non vengono ripristinati dalla sessione precedente (riaprili con **Kit**). Restano la dimensione della finestra, le impostazioni audio e l'elenco dei kit recenti.

## La finestra, in breve

```
┌────────────────────────────────────────────────────────────────────────────────┐
│ NINA   [<] nome kit [>] [Save] [Kit]            [Play] [BPM] [Size] [i]   │ barra superiore
├────────────┬──────────────────────────────────────────────┬────────────────────┤
│ Browser    │  Griglia 4×12 di celle (pad)                 │ Pannello della     │
│ dei file   │  ─────────────────────────────────────────── │ cella selezionata  │
│            │  Forma d'onda con Start / End                │ (modalità, note…)  │
├────────────┴──────────────────────────────────────────────┴────────────────────┤
│ Moduli: AMP · PITCH · MASTER   (oppure la scheda FX, oppure il sequencer)       │
├────────────────────────────────────────────────────────────────────────────────┤
│ Main   Seq   Song   Mixer   FX   Slice                                           │ schede
└────────────────────────────────────────────────────────────────────────────────┘
```

- **Barra superiore:** nome del kit, frecce per il kit precedente/successivo, **Save**, menu **Kit**, **Play/Stop** del sequencer, **BPM**, **Size** (ingrandisce o riduce la finestra a percentuali) e il pulsante **i**: apre la finestra **Info** con il logo, la versione, il copyright, il logo ASIO e i collegamenti al **codice sorgente** e alle **licenze** (vedi capitolo 12), il pulsante **Check for updates** (chiede a GitHub l'ultima versione pubblicata, **solo quando lo premi**: se ce n'è una più nuova ti porta alla pagina di download), e il pulsante **Support my work** con il logo PayPal: NINA è gratuito e uguale per tutti, e la donazione è **libera e volontaria** (apre PayPal nel browser; non cambia nulla nel programma).
- **Schede in basso** (pulsanti con icona; quello della scheda aperta è acceso): **Main** (celle e moduli), **Seq** (sequencer), **FX** (effetti), **Song** (concatenare i pattern in un brano), **Slice** (tagliare i loop sulle celle), **Mixer** (la console: un canale per cella con fader e meter).
- La finestra si può **ridimensionare** trascinando i bordi: tutti gli elementi si adattano.

## Il tuo primo beat in 5 minuti

1. **Carica i campioni.** Nel browser a sinistra scegli una cartella (il menu in alto sceglie Home, Desktop, Documenti, Musica o un disco). Selezionando un file lo **senti subito** (interruttore *Preview* in basso). Trascinalo su un pad, oppure fai doppio clic per caricarlo nel pad selezionato.
2. **Suona i pad.** Un clic su un pad lo fa suonare. Con una tastiera MIDI le note partono da **C1** per il pad A1 e salgono pad dopo pad.
3. **Aggiungi i pad al sequencer.** Tasto destro su un pad con campione → **Add to sequencer**. Ripeti per i pad che ti servono (oppure, nella scheda Seq, **+ Cell** → **Add all loaded cells**).
4. **Disegna il pattern.** Apri la scheda **Seq**: ogni pad aggiunto è una riga. Clicca gli step per accenderli.
5. **Premi Play** (o la barra spaziatrice).
6. **Salva il kit** con **Save**: viene creato un file `.slkit` e una cartella con lo stesso nome con dentro i campioni.

Per i dettagli di ogni parte, continua con i prossimi capitoli.
