# 8. Scorciatoie, limiti e problemi

## Scorciatoie e gesti

| Gesto | Funzione |
|-------|----------|
| **Barra spaziatrice** | Play/Stop del sequencer |
| Clic su un pad | Seleziona e suona la cella |
| Tasto destro su un pad | Menu della cella (aggiungi al sequencer, carica, rinomina, svuota) |
| Doppio clic su un file del browser | Carica il campione nel pad selezionato |
| Trascinare un file su un pad | Carica il campione |
| Doppio clic su una manopola | Valore predefinito; sulle manopole bloccate degli step: toglie il blocco |
| Tasto destro su un controllo | Menu del MIDI learn |
| Trascinare su/giù uno step acceso | Velocity dello step |
| Tasto destro o Maiusc+clic su uno step | Seleziona senza accendere/spegnere |
| Clic sul nome di una riga | Fa suonare la cella e la seleziona |
| Passare il mouse su una voce di **+ Cell** | Fa suonare la cella per sceglierla ascoltando |

## Cosa non c'è ancora

- Il rilevamento del tempo dei loop è una **stima dalla durata** (non analizza i colpi): per loop non tagliati a battute esatte il BPM va scritto a mano.
- La sincronizzazione dei loop funziona sul campione **intero** (Start/End si riferiscono sempre all'originale): non c'è ancora un modo di allineare la griglia del loop (per esempio spostare l'inizio del primo colpo).
- Lo **slicing** dei loop (tagliare in slice e distribuirli sulle celle).
- **Quantizzazione** degli step e registrazione da tastiera nel sequencer.
- **Undo/redo**.
- Altri effetti: EQ, chorus, flanger, phaser, stutter e altri effetti creativi.
- Aggiungere **righe e celle** oltre le 48 della griglia.
- Uscite multiple verso la DAW (una per cella).
- La scheda **Mixer**.
- La versione **macOS** (il codice la prevede ma non è mai stata compilata né provata) e il formato AU.
- Sample da ingresso audio (campionamento da scheda): escluso per ora.

## Cose da verificare (non ancora provate)

- Funzionamento del VST3 in **Cubase 16**: apertura, ridimensionamento, nomi dei parametri, registrazione dell'automazione, MIDI in ingresso.
- Il **trascinamento** dei file verso Cubase.
- **Sync to tempo** e **Key lock** con loop reali nell'app (verificati solo con test automatici su segnali di prova).
- L'uso con una vera tastiera MIDI e un controller per il MIDI learn.

## Risoluzione dei problemi

**Non sento niente (standalone).** Apri **Options** e scegli il dispositivo di uscita. Controlla che il *Volume* nel modulo MASTER non sia a −60 dB e che la cella abbia un campione (non "MISSING").

**Il suono è troppo basso.** Prova **Normalize** sulla cella o **Kit → Normalize all cells**, poi alza **Level** o il **Volume** master (fino a +12 dB).

**Il plugin non compare in Cubase.** Verifica di aver copiato `Sample Lab.vst3` in `C:\Program Files\Common Files\VST3` e rifai la scansione dei plugin.

**Un'eco si sente su tutti gli step.** Il Delay è acceso sulla cella: tutte le note mandano l'eco. Spegni l'interruttore del Delay sulla cella (scheda FX) e usa il **Send** per step.

**Un effetto dello step non si sente.** Muovi la manopola dell'effetto: bloccare un valore lo accende per quello step. Controlla che la riga abbia **FX** acceso (altrimenti la riga suona senza effetti per nota) e che l'interruttore della carta non sia forzato su spento.

**Una cella mostra MISSING.** Il campione non si trova: rimetti la cartella accanto al file `.slkit` e riapri il kit.

**Il sequencer non parte nel plugin.** Se il trasporto della DAW suona, il sequencer lo segue. Se la DAW è ferma, premi Play nel plugin. Se continua a non partire, controlla che almeno una riga abbia degli step accesi.

**Alcuni controlli spariscono quando ridimensiono.** Le dimensioni seguono il lato più stretto della finestra, ma c'è un minimo (circa 1000×640): sotto non si scende. Se vedi controlli tagliati, segnalalo indicando le dimensioni.

**Il kit mostra un asterisco (*).** Ci sono modifiche non salvate: premi **Save**.

## Dove trovare i file di lavoro

- Kit salvati: dove hai scelto, in coppia `.slkit` + cartella.
- Esportazioni: cartella **Bounces** accanto al kit (o Documenti).
- Stato dello standalone: nelle impostazioni dell'applicazione di Windows (cartella `Sample Lab` in `AppData\Roaming`).
