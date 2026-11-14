# 1. Primi passi

## Cosa ti serve

- Windows 10/11 (la versione per macOS è prevista ma non ancora provata).
- Per il plugin: una DAW che carichi i VST3, come Cubase.
- Per lo standalone: una scheda audio (anche quella del computer). Con driver ASIO è possibile solo se hai compilato l'app con l'SDK ASIO (vedi il README del progetto).

## Installazione

Il progetto si compila dal codice sorgente (vedi il README nella cartella principale). Dopo la compilazione trovi:

- **Standalone:** `build\Nina_artefacts\Release\Standalone\NINA.exe`: si avvia con un doppio clic.
- **Plugin VST3:** la cartella `build\Nina_artefacts\Release\VST3\NINA.vst3`. Per farlo vedere a Cubase copiala in `C:\Program Files\Common Files\VST3` (servono i permessi di amministratore) e riavvia la scansione dei plugin della DAW.

## Prima apertura dello standalone

1. Avvia l'app. Se non senti niente, apri **Options** in alto a sinistra e scegli il dispositivo audio di uscita (e, se vuoi suonare con una tastiera, il dispositivo MIDI in ingresso).
2. Lo standalone ricorda lo stato dell'ultima sessione: all'avvio ritrovi i campioni e le impostazioni di come l'avevi lasciato.

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

- **Barra superiore:** nome del kit, frecce per il kit precedente/successivo, **Save**, menu **Kit**, **Play/Stop** del sequencer, **BPM**, **Size** (ingrandisce o riduce la finestra a percentuali) e il pulsante **i**: apre la finestra **Info** con il logo, la versione, il copyright e il logo ASIO.
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
