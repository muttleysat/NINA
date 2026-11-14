[English](REQUIREMENTS.md) · **Italiano**

# NINA – Requisiti di prodotto

Versione: 1.6 (bozza) – 2026-11-12
Stato: documento vivo, da estendere iterativamente.

## 1. Visione

**NINA** (Drumbox Sequencer Powerhouse) è un plugin **VST3** di tipo drum sampler/sample player, ispirato a Native Instruments Battery. Permette di caricare singoli campioni o loop su una griglia di celle, mapparli su una tastiera MIDI, modificarli in modo non distruttivo (forma d'onda, dinamica, tune, time-stretch) e processarli con effetti per cella e globali. I kit si salvano come file `.slkit` collegato a una cartella con i sample, che restano file audio normali recuperabili direttamente.

## 2. Ambito della v1

**Dentro**: formato VST3 **e applicazione standalone**, **Windows e macOS** (macOS Intel + Apple Silicon, build universale), uso personale, tutto quanto descritto ai punti 4–10.
**Fuori (per ora)**: AU/AAX, Linux, distribuzione commerciale, libreria di kit/sample inclusa, sintesi, registrazione audio, scambio di kit con formati di terzi (es. Battery `.kt`).

## 3. Glossario

- **Cella**: slot della griglia che contiene un campione e i suoi parametri.
- **Kit**: insieme di celle + parametri + effetto globale + sample associati.
- **Loop**: campione con durata musicale (battute/beat), sincronizzabile al tempo del progetto.
- **One-shot**: campione singolo non ritmico.

## 4. Interfaccia grafica

Ispirata all'organizzazione di Battery, ma con grafica e asset originali (nessuna copia di risorse o marchi NI).

| ID | Requisito | Priorità |
|----|-----------|----------|
| UI-1 | Finestra unica ridimensionabile con tre aree: **browser** a sinistra, **griglia celle** in alto/centro, **pannello di editing** in basso. | Must |
| UI-2 | Il browser dà accesso all'intero file system (dischi, cartelle, root del PC), con albero cartelle, lista file, filtro per estensione audio, ricerca per nome. | Must |
| UI-3 | Preascolto (audition) del file selezionato nel browser, con volume regolabile e, per i loop, opzione di preascolto sincronizzato al tempo DAW. | Must |
| UI-4 | Caricamento in cella tramite drag & drop dal browser (e da Esplora risorse) o doppio clic sulla cella selezionata. | Must |
| UI-5 | Griglia di celle di default **4 righe (A–D) × 12 colonne = 48 celle**, con possibilità di **aggiungere/rimuovere righe e celle** (righe etichettate A, B, C…; colonne numerate). Ogni cella mostra nome, colore assegnabile, indicatore di attività e mute/solo. | Must |
| UI-9 | Layout di riferimento (screenshot Battery): barra superiore con nome kit, indicatore/campo BPM e Sync; colonna sinistra con tab *Library / Files* (e tag di categoria per i campioni); sotto la griglia, la **waveform** con marker S/E e controlli rapidi (Tune, Reverse, Pan, Level, Key Range, selezione layer); in basso la **catena effetti** a moduli affiancati; schede inferiori (Main, Effects, Modulation, Setup, Editor, Master) per i diversi pannelli. Da ripensare con grafica originale. | Should |
| UI-11 | **Menu contestuale (tasto destro) sulla cella**: Aggiungi/Rimuovi dal sequencer, Rinomina, Colore, Assegna nota MIDI (MIDI learn), Assegna uscita, Duplica, Svuota cella, Mostra file nel browser. | Must |
| UI-12 | **Finestra ridimensionabile** sia nel plugin VST3 sia nello standalone, trascinando gli angoli/bordi (e, nello standalone, anche con massimizza/schermo intero). Dimensione minima (proposta 1000×640 px) e massima (fino allo schermo intero), proporzioni libere con vincoli minimi. Nel VST3 il ridimensionamento passa dal supporto nativo della DAW (verificato in Cubase 16). Dimensione e posizione sono ricordate tra una sessione e l'altra e salvate con il progetto della DAW. | Must |
| UI-12b | **Fallback di ridimensionamento a percentuali**: se la DAW (Cubase o altre) non consente il ridimensionamento libero della finestra del plugin, o lo gestisce male, il plugin offre un controllo **Dimensione finestra** (menu in barra superiore e scorciatoia) con scelte predefinite in percentuale rispetto alla dimensione base: es. **75%, 100%, 125%, 150%, 200%** (più eventuali valori personalizzati). Il cambio di percentuale ridimensiona la finestra e scala tutta l'interfaccia in blocco, con gli stessi adattamenti del layout di UI-13 (nessun taglio, testi/controlli proporzionali). La dimensione scelta è ricordata e salvata col progetto. Le percentuali sono limitate a quelle che entrano nello schermo, con avviso/riduzione automatica. Disponibile anche nello standalone. | Must |
| UI-13 | **Layout adattivo**: quando la finestra cambia dimensione tutti gli elementi si adattano di conseguenza, senza tagli né sovrapposizioni: la griglia delle celle si ridisegna (celle più grandi o più piccole, mantenendo le 12 colonne), la waveform e il sequencer occupano lo spazio disponibile, i pannelli degli effetti si riorganizzano o si compattano, il browser a sinistra e le altre colonne hanno larghezza proporzionale con limiti min/max. Testi, manopole e icone scalano insieme. A dimensioni ridotte i pannelli meno usati diventano schede o si comprimono. | Must |
| UI-14 | **Scalabilità e nitidezza**: grafica vettoriale (nessuna bitmap fissa) per restare nitida a ogni dimensione e densità di pixel (HiDPI/Retina, scala 100–300%); fattore di zoom dell'interfaccia impostabile (es. 75%–200%) indipendente dalla dimensione finestra. | Must |
| UI-15 | **Struttura efficiente**: organizzazione chiara in aree fisse (browser, griglia, editor/sequencer/effetti a schede, barra superiore), le funzioni usate più spesso a un clic di distanza, nessun menu annidato profondo; stato attuale sempre visibile (cella selezionata, kit, BPM, pattern in play). Possibilità di nascondere/mostrare il browser e di ingrandire il pannello attivo (editor, sequencer) a pieno spazio. | Must |
| UI-16 | **Accessibilità e uso rapido**: scorciatoie da tastiera per le azioni principali (play/stop, cambio scheda, zoom waveform, undo/redo), tooltip su tutti i controlli, doppio clic per il reset dei valori, modifica dei valori con rotella e inserimento numerico diretto, selezione multipla di celle e step con drag e Maiusc/Ctrl. | Should |
| UI-17 | **Grafica accattivante**: tema scuro coerente con palette controllata e colori di accento per cella/categoria, manopole e fader curati, waveform con gradiente e ombreggiature leggere, animazioni brevi e fluide (trigger delle celle, meter), icone coerenti, tipografia leggibile a ogni dimensione. Opzione per un tema chiaro/alternativo come estensione futura. | Should |
| UI-18 | **Prestazioni UI**: ridisegno fluido (60 fps di riferimento) anche con 48+ celle e il sequencer attivi, anche durante il ridimensionamento; rendering con accelerazione GPU quando disponibile (Direct2D su Windows, Metal su macOS), senza influenzare il thread audio. | Should |
| UI-10 | Colori per cella assegnabili manualmente e per categoria (kick, snare, hat, ecc.). | Could |
| UI-6 | Selezionando una cella, il pannello di editing mostra i parametri di **quella** cella nella stessa finestra (nessuna finestra separata). | Must |
| UI-7 | Selezione "Kit/Globale" per lavorare sugli effetti e parametri dell'intero kit. | Must |
| UI-8 | Scalabilità UI (HiDPI / zoom) e tema scuro. | Should |

## 5. Gestione campioni e mapping MIDI

| ID | Requisito | Priorità |
|----|-----------|----------|
| MAP-0 | **Mapping automatico di default**: la prima cella (A1) è assegnata a **C1** (MIDI 36, convenzione C3 = 60 usata da Cubase) e ogni cella successiva alla nota seguente, in ordine orizzontale per riga (A1…A12, B1…B12, …). Con la griglia 4×12 si coprono C1–B4 (note 36–83). Aggiungendo celle il mapping prosegue; il valore di partenza è modificabile globalmente. | Must |
| MAP-1 | Ogni cella ha un **tasto root** e un **range di tasti** (nota bassa–alta). Di default il range coincide con la sola nota assegnata automaticamente; l'utente può cambiare **manualmente** la nota di una cella (o il range) in qualsiasi momento. Con range di più note il campione viene trasposto rispetto al root (uso melodico). Le celle manuali non vengono toccate se si riapplica il mapping automatico. | Must |
| MAP-2 | Assegnazione rapida con **MIDI learn**: premere un tasto sulla tastiera per assegnare la cella. | Must |
| MAP-3 | Range di **velocity** per cella e sensibilità alla velocity (volume, eventuale filtro). | Should |
| MAP-4 | Più celle sulla stessa nota (layer) e **gruppi di esclusione** (es. hi-hat chiuso/aperto, choke). | Should |
| MAP-5 | Modalità polifonica / monofonica per cella. | Must |
| MAP-6 | Formati supportati in import: WAV, AIFF, FLAC, MP3, OGG (WAV/AIFF 16/24/32 float, fino a 192 kHz). Resampling automatico alla frequenza del progetto. | Must |

### 5.1 MIDI learn dei parametri

Ogni parametro controllabile (manopole, fader, interruttori) può essere assegnato a un controllo di un dispositivo MIDI (manopola, fader, pulsante di un controller), oltre alla normale automazione della DAW.

| ID | Requisito | Priorità |
|----|-----------|----------|
| ML-1 | **Assegnazione con MIDI learn**: tasto destro su un parametro, voce "MIDI Learn"; il parametro entra in modalità di ascolto (evidenziato), si muove un controllo sul controller e l'assegnazione viene creata sul primo messaggio ricevuto (CC, pitch bend, aftertouch di canale). Annullabile con Esc o un nuovo clic. | Must |
| ML-2 | Voci del menu contestuale del parametro: **MIDI Learn**, **Clear MIDI mapping**, e l'indicazione dell'assegnazione corrente (es. "CC 74, ch. 1"). I parametri assegnati hanno un indicatore visibile sul controllo. | Must |
| ML-3 | Il valore del parametro segue il controllo MIDI nell'intero intervallo (0–127 mappato su min–max del parametro, rispettando la scala logaritmica/skew). Gli interruttori si attivano con valori ≥ 64 o come pulsante a scatto (toggle) con opzione. | Must |
| ML-4 | **Modalità dei controlli**: assoluta (default) e relativa (encoder infiniti: segno e complemento a due, offset binario). Opzione per parametro. | Should |
| ML-5 | **Soft takeover (pickup)**: dopo un cambio di cella o di kit, il parametro non salta al valore del controller finché questo non incrocia il valore corrente, per evitare salti bruschi. Opzione attivabile. | Should |
| ML-6 | **Parametri della cella selezionata**: assegnazione "per cella selezionata" (la manopola del controller agisce sui parametri della cella attualmente selezionata, es. Level, Tune, Cutoff) oppure "fissa su una cella" (agisce sempre sulla cella scelta). | Should |
| ML-7 | **Pannello "MIDI Mappings"**: elenco di tutte le assegnazioni (parametro, controllo, canale, modalità) con modifica ed eliminazione, "Clear all" e filtro per canale/dispositivo. | Should |
| ML-8 | **Salvataggio**: le assegnazioni vengono salvate nel kit `.slkit`, e si possono esportare/importare come **preset di mappatura** indipendente (per riusare lo stesso controller con altri kit). In più, una mappatura globale dell'applicazione ricordata tra le sessioni. | Must |
| ML-9 | **Uso nel plugin**: i messaggi MIDI arrivano dalla DAW; in Cubase 16 i CC raggiungono il plugin come eventi MIDI se la traccia riceve da un dispositivo MIDI, da verificare con un prototipo. In alternativa l'assegnazione passa dal sistema di automazione/MIDI mapping della DAW (parametri esposti con ID stabili, NF-6). **Nello standalone** i messaggi arrivano direttamente dai dispositivi MIDI selezionati (SA-2). | Must |
| ML-10 | Lo stesso messaggio MIDI può controllare più parametri; i messaggi di nota (usati per il mapping delle celle, MAP-0/MAP-2) restano separati dal MIDI learn dei parametri. Il feedback verso il controller (LED/motorizzati) è fuori ambito per la v1. | Could |
| ML-11 | L'uso del MIDI learn non deve causare click: i valori ricevuti sono smussati e applicati in modo sicuro rispetto al thread audio. | Must |

## 6. Editing del campione (non distruttivo)

L'originale su disco non viene mai modificato; le modifiche sono parametri salvati nel kit.

| ID | Requisito | Priorità |
|----|-----------|----------|
| ED-1 | Visualizzazione forma d'onda con zoom e scroll, stereo/mono. | Must |
| ED-2 | **Start e End** regolabili con marker trascinabili sulla waveform; snap opzionale agli zero crossing. | Must |
| ED-3 | Loop point (sustain loop) opzionale, con crossfade. | Could |
| ED-4 | **Dinamica**: inviluppo di ampiezza **AHDSR** (attack, hold, decay, sustain, release), pan, fade-in/fade-out, inversione di fase. Volume e normalizzazione: vedi ED-8…ED-12. | Must |
| ED-5 | **Reverse** (riproduzione al contrario). | Should |
| ED-6 | Compressore/transient shaper per cella (rientra nella catena effetti, sez. 7). | Should |
| ED-7 | Undo/redo sulle modifiche dei parametri. | Should |
| ED-8 | **Volume per cella** (Level): fader/manopola per ogni cella, da -∞ a +12 dB, con valore numerico in dB, sempre visibile nel pannello della cella (come il Level di Battery) e regolabile con un gesto rapido anche dalla cella nella griglia (es. rotella del mouse). Doppio clic per tornare a 0 dB. Automatizzabile dalla DAW. | Must |
| ED-9 | **Metering**: indicatore di livello (picco) per ogni cella durante la riproduzione e meter sul master e sulle uscite, con indicazione di clip. | Should |
| ED-10 | **Normalizzazione non distruttiva**: calcola un guadagno di compensazione per ogni campione, memorizzato nel kit (`normalizeGain`) e applicato in aggiunta al Level; il file audio non viene mai modificato. Il guadagno è ricalcolato quando cambiano start/end. | Must |
| ED-11 | **Modalità di misura** selezionabili per cella e per il kit: **Picco** (porta il picco massimo a un valore target, es. -1 dBFS), **RMS** (livello medio della parte udibile tra start/end, adatto ai colpi di batteria) e **Loudness LUFS** (per loop e campioni lunghi, ≥ 1 s). Il target è impostabile (default: -1 dBFS picco, -18 dB RMS, -14 LUFS). Con campioni molto brevi la modalità LUFS è disabilitata o sostituita da RMS. | Must |
| ED-12 | **Normalizza tutto il kit**: un comando che applica la normalizzazione a tutte le celle con campione (o alle selezionate) per farle suonare allo stesso volume percepito, con anteprima del guadagno calcolato per cella e possibilità di annullare con un solo undo. Opzione di normalizzare **automaticamente ogni nuovo campione** al caricamento. Un interruttore **Normalizza** on/off per cella permette di escludere un campione (es. una hat volutamente più bassa). | Must |
| ED-13 | **Limite di sicurezza**: il guadagno di normalizzazione viene limitato (es. max +24 dB) con avviso per campioni quasi silenziosi, e il picco risultante non può superare 0 dBFS senza limiter sulla cella. Il volume per cella si somma, in dB, alla normalizzazione, alla velocity e al volume del kit. | Should |

## 7. Effetti

Gli effetti sono disponibili **per cella** e a livello **globale (kit)**, con stessa interfaccia, bypass e dry/wet.

| ID | Requisito | Priorità |
|----|-----------|----------|
| FX-1 | Architettura: catena di effetti per cella → mixer di kit → catena globale → uscita. | Must |
| FX-2 | **Standard**: EQ parametrico, filtro (LP/HP/BP), compressore, limiter, saturazione/drive, delay, riverbero. | Must |
| FX-3 | **Creativi**: bit crusher/sample-rate reducer, distorsione/waveshaper, chorus, flanger, phaser, tape stop, stutter/gater, ring modulator. (Elenco iniziale, da raffinare.) | Should |
| FX-4 | Ordine degli effetti nella catena modificabile; numero di slot per cella (proposta: 4) e globale (proposta: 6). | Should |
| FX-5 | Send/aux per riverbero e delay condivisi tra celle. | Could |
| FX-8 | **Sync al tempo degli effetti ritmici**: gli effetti con un tempo o una frequenza (delay, stutter/gater, tremolo, auto-pan, LFO di chorus/flanger/phaser, filtro con LFO) hanno un interruttore **Free / Sync**. In Free il valore è in ms o Hz; in Sync si sceglie un valore musicale: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, con varianti puntate (.) e terzine (T). Il tempo è il **BPM della DAW** (anche con tempo variabile) o, nello standalone, il BPM interno (SA-3). | Must |
| FX-9 | **Sincronizzazione di fase**: LFO, stutter e gater si agganciano alla posizione di trasporto (battuta/beat) e ripartono allineati a ogni Play o salto di posizione; opzione "Retrigger on note" per ripartire a ogni nota. Il cambio di tempo non produce click (smussamento). Nel delay, modalità **Tape** (il cambio di tempo cambia anche l'intonazione) o **Clean** (nessuna variazione di pitch). | Should |
| FX-6 | **Uscite multiple**: ogni cella è assegnabile a una uscita della DAW (stereo principale + uscite aggiuntive, proposta 16 stereo). L'assegnazione è salvata nel kit. Gli effetti per cella sono applicati prima dell'uscita assegnata; la catena globale agisce solo sulle celle instradate sul master. | Must |

## 8. Loop, sync e time-stretch

| ID | Requisito | Priorità |
|----|-----------|----------|
| LP-1 | Per ogni cella, modalità **Loop Sync**: il campione viene stirato per adattarsi al **BPM del progetto** letto dalla DAW (tempo variabile incluso). | Must |
| LP-2 | Definizione della lunghezza musicale del loop (n. di beat/battute) con rilevamento automatico del BPM e correzione manuale. | Must |
| LP-3 | Partenza del loop quantizzata (nessuna / beat / battuta) e allineamento alla posizione di trasporto DAW. | Should |
| LP-4 | Modalità di time-stretch con qualità selezionabile (es. ritmico/percussivo vs. tonale). | Must |
| LP-5 | Sync disattivabile: il loop può suonare a velocità originale (o a velocità variabile col pitch) senza sync. | Must |
| LP-6 | Slicing dei loop: vedi sezione 8.1. | Must |

### 8.1 Slicing dei loop

| ID | Requisito | Priorità |
|----|-----------|----------|
| SL-1 | **Slice automatici** su un loop: per rilevamento dei transienti (con sensibilità regolabile) o per griglia (divisione in N parti uguali / valore ritmico: 1/4, 1/8, 1/16…). | Must |
| SL-2 | **Slice manuali**: aggiungere, spostare ed eliminare marker sulla waveform, con snap a zero crossing o alla griglia ritmica. Gli slice automatici sono poi modificabili a mano. | Must |
| SL-3 | **Distribuzione sulle celle**: funzione "Distribuisci slice" che crea una cella per ogni slice, **in ordine orizzontale**, a partire da una cella di destinazione scelta, procedendo lungo la riga (A1→A12) e passando alla successiva (B1…), così da occupare note MIDI consecutive. Ogni cella riceve start/end, nome (es. `Loop_01`), e riferisce lo stesso file sorgente (nessuna copia del sample). | Must |
| SL-4 | Opzioni di distribuzione: cella di partenza, sovrascrivere o saltare le celle occupate, avviso/aggiunta di righe se le celle non bastano, assegnazione automatica di note MIDI consecutive, e modalità trigger (one-shot/gate) e uscita per le celle create. | Should |
| SL-5 | Le celle create dagli slice mantengono un collegamento al loop originale: se il loop viene risincronizzato, gli slice seguono il tempo del progetto. | Should |
| SL-6 | Modalità alternativa "slice su singola cella": il loop resta in una cella e gli slice sono suonati da note MIDI consecutive (stile MPC Chop). | Could |
| SL-7 | Possibilità di annullare l'intera distribuzione con un solo undo. | Should |

## 9. Tune, pitch e modalità di riproduzione

| ID | Requisito | Priorità |
|----|-----------|----------|
| PT-1 | **Tune** (intonazione fine, ±100 cent) e **Pitch** (semitoni, ±24) per cella. | Must |
| PT-2 | **Relazione pitch / time-stretch** (comportamento tipo *Key Lock / Master Tempo* di VirtualDJ): quando un campione è sincronizzato al tempo del progetto, il time-stretch adatta la durata **preservando l'intonazione originale**, senza stravolgere il timbro. Il pitch percepito resta quello impostato da Tune/Pitch dell'utente. | Must |
| PT-2b | Selettore per cella **Key Lock** on/off: con Key Lock off la velocità di riproduzione è accoppiata al pitch (varispeed, come un giradischi); con on (default per i loop sincronizzati) l'intonazione è indipendente dal tempo. | Should |
| PT-2c | Limiti di stretch consigliati per mantenere qualità (es. ±50% / ±8 BPM tipici); oltre, avviso o scelta del motore di stretch più adatto. | Could |
| PT-3 | Pitch bend MIDI e keytracking (la nota suonata trasposta rispetto al root) attivabili per cella. | Should |
| PT-4 | **Modalità di trigger stile Akai MPC**, selezionabile per cella e valida sia per one-shot sia per loop: | Must |
| | • **Note On (one-shot)**: il campione suona fino in fondo, il Note Off è ignorato. | |
| | • **Note Off / gate**: il campione suona solo finché il tasto è premuto; al rilascio parte la fase di release dell'inviluppo. | |
| | • **Loop**: il campione si ripete finché il tasto è premuto. | |
| PT-5 | Round-robin/alternanza e randomizzazione leggera (pitch, velocity) per cella. | Could |

### 9.1 Sequencer a step (fino a 64 step)

Sequencer interno che suona le celle del kit e consente di creare una **frase MIDI** da portare nella DAW.

| ID | Requisito | Priorità |
|----|-----------|----------|
| SQ-1 | Pattern fino a **64 step**; lunghezza regolabile (1–64), divisione ritmica per lane (SQ-14), swing. Il sequencer ha **una riga (lane) per ogni cella**, ciascuna con i propri 64 step e la propria lunghezza (poliritmie). Nel sequencer compaiono **solo le celle aggiunte dall'utente** (SQ-12). | Must |
| SQ-2 | Sincronizzato al **trasporto e al tempo della DAW** (play/stop/posizione, tempo variabile), con opzione di lasciare il pattern in play libero. Pattern multipli (almeno 16 slot per kit), selezionabili anche da note MIDI. | Must |
| SQ-3 | Parametri per step: **on/off, velocity, nota/cella, durata (gate), probabilità, ratchet/ripetizioni, micro-timing (nudge)**. | Must |
| SQ-4 | **Modalità di riproduzione per step**: one-shot / gate, reverse, selezione slice (per loop slicciati), pitch/tune di step, start offset. | Must |
| SQ-5 | **Effetti per step** (*parameter lock*, stile Elektron): ogni step può sovrascrivere i parametri degli effetti della cella (es. filtro, drive, bit crusher, delay send) solo per quello step; i valori non bloccati restano quelli della cella. | Must |
| SQ-5b | **Livelli diversi dello stesso parametro tra step e tra celle**: ogni cella ha il proprio filtro (e i propri effetti) indipendente. Un parametro come il cutoff può avere un valore diverso a ogni step della stessa lane, e valori diversi in lane diverse (es. kick con cutoff 400 Hz, snare con cutoff 3 kHz, hat con cutoff 9 kHz). Il valore tra uno step e l'altro è a scalino oppure interpolato (opzione "Slide", con tempo di glide regolabile per step). | Must |
| SQ-5c | **Editing di più lane insieme**: selezionando più celle/lane, un parametro (es. cutoff) si può impostare o modulare per tutte con un gesto (valore assoluto, offset relativo o rampa da una cella all'altra), e un lock si può copiare su altri step e altre lane. Vista a barre del parametro scelto per tutte le lane. | Should |
| SQ-6 | Editor griglia nella stessa finestra (pannello a schede), con editing rapido di velocity/probabilità, copia/incolla step e pattern, randomizzazione e clear. | Must |
| SQ-7 | **Esportazione frase MIDI nella DAW**: (a) **drag & drop** del pattern dal plugin alla traccia o all'area di progetto di Cubase come clip MIDI (file `.mid` generato al volo); (b) esportazione su file `.mid`; (c) *facoltativo, non pianificato*: uscita MIDI in tempo reale del plugin da instradare su un'altra traccia, da provare in Cubase 16 solo se utile. L'esportazione conserva note, velocity, durata e probabilità/ratchet "risolti" in eventi MIDI fissi. | Must |
| SQ-8 | Il sequencer pilota le celle direttamente (non solo via MIDI interno); le note MIDI in ingresso dalla DAW continuano a funzionare in parallelo. Possibile modalità "registra da tastiera" (step recording) con quantizzazione. | Should |
| SQ-9 | Pattern salvati nel kit `.slkit` insieme al resto dei parametri. | Must |
| SQ-10 | **Esporta audio (Bounce pattern)**: dal sequencer, comando "Esporta WAV" che genera un file audio del pattern **completo di tutto ciò che il sequencer fa** (reverse, parameter lock degli effetti, pitch, ratchet, probabilità, effetti di cella e globali). Il file si importa poi in Cubase come qualsiasi audio, accanto alla clip MIDI esportata (SQ-7). | Must |
| SQ-10a | Opzioni di export: numero di ripetizioni del pattern (1, 2, 4… o lunghezza in battute), **coda finale** per riverbero/delay (automatica o in secondi), formato **WAV** a 16/24 bit o 32 float (in alternativa **AIFF** o **FLAC**), frequenza di campionamento (quella del progetto o scelta), normalizzazione opzionale. | Must |
| SQ-10b | **Tempo**: il pattern viene renderizzato al BPM corrente della DAW o a un BPM scelto manualmente. Il BPM e il numero di battute vengono scritti nel nome file (es. `NomeKit_Pattern01_120bpm_4bar.wav`) e, se supportato, nei metadati del WAV. | Must |
| SQ-10c | **Stem**: oltre al mix stereo completo, opzione di esportare un file per ogni cella o uscita assegnata (kick, snare, hat…), tutti della stessa lunghezza per allinearsi in Cubase. | Should |
| SQ-10d | Il rendering avviene **offline**, più veloce del tempo reale, senza dover far suonare la DAW, con barra di avanzamento e possibilità di annullare. | Should |
| SQ-10e | Posizione di salvataggio scelta dall'utente; di default la cartella `Bounces/` accanto al file `.slkit` del kit. Nome predefinito: nome kit + nome pattern. Drag & drop del WAV generato direttamente su una traccia di Cubase, come per la clip MIDI. | Should |
| SQ-12 | **Menu contestuale (tasto destro) sulla cella** nella griglia: voce **"Aggiungi al sequencer"** (diventa "Rimuovi dal sequencer" se la cella è già presente). Il sequencer contiene così solo le celle scelte dall'utente, cella per cella, nell'ordine in cui le aggiunge. Rimuovere una riga chiede conferma se contiene step attivi; il pattern della riga viene conservato finché non si svuota esplicitamente. | Must |
| SQ-13 | Gestione di molte righe: scroll verticale, altezza righe regolabile, riordino per trascinamento (senza cambiare la nota MIDI della cella), mute/solo per riga, pagine di 16 step con scroll orizzontale. Cliccando una cella nella griglia, il sequencer porta in primo piano la sua riga. Vista di dettaglio a riga singola con barre di velocity e probabilità. Le righe vuote non occupano spazio nel salvataggio del kit. | Must |
| SQ-14 | **Risoluzione/quantizzazione per lane**: ogni riga ha la propria divisione ritmica, scelta tra 1/4, 1/8, 1/16, 1/32, 1/64 e le rispettive terzine (1/8T, 1/16T, 1/32T) e quintine. Gli step della riga durano quella divisione, quindi 64 step a 1/16 coprono 4 battute e a 1/32 coprono 2. Righe con risoluzioni diverse possono convivere nello stesso pattern (es. kick a 1/16, hat a 1/32) e restano allineate al tempo della DAW. Cambiando la risoluzione di una riga con step già inseriti si può scegliere se **mantenere la posizione musicale** (gli step si spostano per restare allo stesso punto del tempo) o **mantenere gli indici** degli step. | Must |
| SQ-15 | **Quantizzazione degli step**: funzione "Quantizza" per lane (o per le righe selezionate) che porta gli step e il micro-timing (nudge, registrazione da tastiera) sulla griglia scelta, con **intensità** da 0 a 100% (quantizzazione parziale) e opzione "mantieni swing". Quantizzazione automatica in tempo reale durante lo step recording (SQ-8), con griglia selezionabile. | Should |
| SQ-16 | **Swing per lane** oltre a quello globale, con valore regolabile per riga. | Could |
| SQ-11 | **Seme casuale per pattern**: probabilità, randomizzazione e ratchet casuali usano un seme memorizzato nel pattern, così riproduzione ed export sono ripetibili e identici. Opzione per cambiare seme o lasciarlo casuale a ogni play. | Should |

*Nota di fattibilità*: un plugin VST3 non può scrivere direttamente sulla timeline della DAW; per questo l'esportazione passa da drag & drop di file MIDI o da uscita MIDI. Il MIDI non può rappresentare reverse ed effetti per step, che sono invece inclusi nell'esportazione audio WAV (SQ-10).

### 9.2 Versione standalone (Windows e macOS)

Oltre al VST3, NINA è distribuito come **applicazione autonoma** per Windows e macOS, con lo stesso motore audio, la stessa interfaccia e gli stessi kit del plugin. Utile per usare il sequencer e il sampler senza DAW e per preparare pattern e kit da portare poi in Cubase.

| ID | Requisito | Priorità |
|----|-----------|----------|
| SA-1 | Un'unica base di codice produce **VST3 e standalone** (target separati dello stesso progetto), con interfaccia e funzioni identiche. Applicazione nativa per Windows (x64) e macOS (Intel + Apple Silicon). | Must |
| SA-2 | **Impostazioni audio/MIDI**: scelta del driver e del dispositivo audio (WASAPI/ASIO su Windows, CoreAudio su Mac), frequenza di campionamento, dimensione buffer, assegnazione delle uscite del dispositivo alle uscite delle celle (FX-6), selezione dei dispositivi MIDI in ingresso (tastiera/pad) con MIDI learn. | Must |
| SA-3 | **Trasporto interno** (senza DAW): Play/Stop, **BPM impostabile**, tempo di misura, tap tempo, metronomo opzionale. Il sequencer, i loop sincronizzati (LP-1) e gli slice usano questo tempo al posto di quello della DAW. | Must |
| SA-4 | **Sync esterno** opzionale: ricezione di **MIDI Clock** (start/stop/tempo) da un dispositivo o da un'altra app; invio del MIDI Clock come possibile estensione. | Could |
| SA-5 | Apertura, salvataggio e kit recenti (KIT-8, KIT-9) identici al plugin. Stesso formato `.slkit`, quindi i kit sono intercambiabili tra standalone e VST3. All'avvio, ripristino dell'ultimo kit e delle impostazioni audio. | Must |
| SA-6 | Tutte le esportazioni (clip MIDI, WAV, stem, SQ-7 e SQ-10) disponibili anche in standalone; il drag & drop su una DAW funziona come per il plugin, ove supportato dal sistema. | Must |
| SA-7 | Registrazione dell'uscita (**Record** audio live) in un WAV, oltre al Bounce offline del pattern. | Should |
| SA-8 | **Ableton Link** per la sincronizzazione del tempo con altre app e DAW sulla stessa rete. | Could |
| SA-9 | Gestione sicura del dispositivo audio: avviso se manca o cambia, nessun crash alla disconnessione, ripristino automatico delle impostazioni. | Must |
| SA-10 | Menu dell'applicazione (File, Modifica, Opzioni) nativo su Windows e macOS, con scorciatoie da tastiera (apri/salva kit, play/stop con barra spaziatrice, undo/redo). | Should |

## 10. Kit: salvataggio e caricamento

Formato: un **file kit `.slkit`** (testo JSON leggibile) collegato a una **cartella di sample** normale, dalla quale i campioni si possono recuperare direttamente con Esplora risorse/Finder. Il file e la cartella stanno affiancati:

```
Kits/
├── MioKit.slkit            # JSON: versione schema, nome, celle, mapping, parametri, effetti, tempi, riferimento alla cartella
└── MioKit/                 # cartella con lo stesso nome del kit: sample copiati (file audio originali, apribili ovunque)
    ├── Kick Funk 7.wav
    ├── Snare Funk 1.wav
    └── ...
```

| ID | Requisito | Priorità |
|----|-----------|----------|
| KIT-1 | Al salvataggio, l'utente dà un **nome al kit** nell'applicazione; sia il file sia la cartella assumono **esattamente quel nome** (`<NomeKit>.slkit` e cartella `<NomeKit>/`, affiancati nella stessa posizione). I sample usati vengono **copiati nella cartella**, come file audio originali non rinominati né convertiti. Caratteri non validi per il file system vengono sostituiti; se nome o cartella esistono già, richiesta di conferma per sovrascrivere o scelta di un altro nome. | Must |
| KIT-1b | Rinominando il kit con "Salva con nome", file e cartella vengono creati con il nuovo nome; il nome del kit mostrato nella barra superiore coincide sempre con quello del file. | Must |
| KIT-2 | `kit.json` versionato (`schemaVersion`) con migrazione dalle versioni precedenti. | Must |
| KIT-3 | Il file `.slkit` memorizza i sample con **percorsi relativi** alla cartella del kit (compatibili Windows/macOS). Il kit viene salvato anche nello stato del plugin della DAW (recall del progetto) con lo stesso riferimento. | Must |
| KIT-3b | Possibilità di scegliere una cartella di sample diversa (es. condivisa da più kit) e di **salvare il kit come copia** in una nuova posizione, con copia o collegamento dei sample. | Should |
| KIT-4 | Sample duplicati (stesso contenuto) copiati una sola volta (hash). Nomi file in conflitto risolti con suffisso numerico. | Should |
| KIT-5 | Gestione sample mancanti o cartella spostata: avviso, ricerca automatica accanto al file `.slkit` e possibilità di rilocare la cartella. | Must |
| KIT-8 | **Apri / Salva / Salva con nome / Nuovo kit** dal menu del plugin (barra superiore, accanto al nome del kit), con finestra di selezione file `.slkit`. Avviso se ci sono modifiche non salvate prima di aprire un altro kit. | Must |
| KIT-9 | **Kit recenti**: un menu a tendina (nel nome del kit in alto) elenca gli **ultimi kit aperti o salvati** (ultimi 5, più recente in alto), con il nome del kit e il percorso al passaggio del mouse. Selezionando una voce il kit si carica. I kit non più presenti sono segnalati e rimovibili dall'elenco; è disponibile "Svuota recenti". L'elenco è memorizzato nelle impostazioni utente e condiviso tra tutte le istanze del plugin. | Must |
| KIT-7 | Poiché i sample sono file indipendenti, le modifiche (start/end, effetti, ecc.) restano **non distruttive** nel `.slkit` e non alterano mai i file audio. | Must |
| KIT-6 | Salvataggio/caricamento di **singola cella** (preset di cella). | Could |

## 11. Requisiti non funzionali

| ID | Requisito |
|----|-----------|
| NF-1 | Latenza trascurabile: nessuna allocazione/lock/IO nel thread audio; sample caricati in RAM (streaming da disco per sample lunghi come estensione futura). |
| NF-2 | Polifonia minima 64 voci totali; CPU contenuta con 16 celle attive e effetti. |
| NF-3 | Elaborazione audio a 32-bit float interna; supporto 44.1–192 kHz e buffer variabili. |
| NF-4 | Stabilità: nessun crash su file corrotti o non supportati; caricamento in thread separato. |
| NF-5 | Compatibilità con le DAW principali su Windows e macOS (DAW di riferimento: **Cubase 16** (Windows e macOS); verifiche secondarie su Reaper, Ableton Live, FL Studio; Logic Pro non carica VST3 e per ora è escluso). Validazione con il validator VST3 di Steinberg. |
| NF-8 | Kit `.slkit` identici e interscambiabili tra Windows e macOS (percorsi relativi, endianness e codifica nomi file gestite). |
| NF-6 | Tutti i parametri automatizzabili dalla DAW (con ID stabili). |
| NF-7 | Lingua UI: inglese (italiano come possibile traduzione futura). |

### 11.1 Automazione e parametri esposti alla DAW

| ID | Requisito | Priorità |
|----|-----------|----------|
| AU-1 | **Tutti i controlli sono parametri automatizzabili** della DAW, in lettura e in scrittura (modalità Read, Write, Touch, Latch in Cubase): per ogni cella Start, End, Level, Pan, Tune, Pitch, Reverse, modalità di trigger, inviluppo AHDSR, tutti i parametri degli effetti con i loro interruttori on/off; più i parametri del master e degli effetti globali. I valori cambiati dall'interfaccia vengono registrati dalla DAW, quelli automatizzati si vedono muovere sull'interfaccia. | Must |
| AU-2 | **Nomi riconoscibili, non numeri di CC**: ogni parametro ha un nome chiaro, un'unità e un valore leggibile (es. "Cutoff - 2.4 kHz", "Mode - Gate", "Level - -6 dB"). Nell'elenco dell'automazione di Cubase compaiono come "A1 Filter Cutoff" e non come "CC 45". | Must |
| AU-3 | **Gerarchia a gruppi**: i parametri sono raggruppati per cella ed effetto (es. *Cell A1 > Filter > Cutoff*, *Master > Reverb > Mix*) tramite le unità/gruppi di parametri VST3, così da essere ordinati e cercabili nel menu della DAW. | Must |
| AU-4 | **ID stabili e retrocompatibili**: l'identificatore di ogni parametro è derivato dalla cella e dal nome del parametro e non cambia tra le versioni. I parametri aggiunti in futuro non alterano quelli esistenti, per non rompere i progetti già salvati con automazioni. | Must |
| AU-5 | **Copertura delle celle**: i parametri sono esposti per tutte le 48 celle della griglia (4×12), 38 per cella più 26 del master, circa 1.850 in totale, organizzati in gruppi per non appesantire il menu della DAW. Il limite di celle è fissato a 48 finché non sarà possibile aggiungere righe; per un numero maggiore si dovrà rivedere questa scelta. | Must |
| AU-6 | **Gesture corrette**: ogni modifica dall'interfaccia segnala alla DAW inizio e fine del gesto (touch/release), così che Write, Touch e Latch registrino correttamente. I valori automatizzati sono smussati nel tempo per evitare click. | Must |
| AU-7 | **Parametri discreti con testo**: interruttori e selettori (tipo di filtro, modalità di trigger, sync on/off, valore di sync 1/16, ecc.) hanno valori testuali leggibili nella DAW. | Must |
| AU-8 | **Parametri "cella selezionata"** (opzionale): un piccolo set di parametri generici (es. Level, Tune, Cutoff) che agiscono sempre sulla cella selezionata, per controllarla velocemente da un controller o dai Quick Controls di Cubase. L'automazione registrata su questi dipende dalla selezione ed è segnalata come tale. | Could |
| AU-9 | **Interazione con il sequencer**: il valore automatizzato dalla DAW è il valore di base del parametro; i parameter lock del sequencer (SQ-5) lo sovrascrivono solo nello step in cui sono attivi. L'ordine di priorità è documentato e visibile (indicatore sul controllo con lock attivo). | Must |
| AU-10 | **Collegamento con il MIDI learn** (sez. 5.1): il MIDI learn assegna i controller agli stessi parametri e passa dallo stesso sistema, quindi funziona anche nello standalone (dove l'automazione della DAW non esiste). | Must |
| AU-11 | **Verifica in Cubase 16**: i parametri compaiono per nome nell'elenco dell'automazione e nei Quick Controls, si registrano e si riproducono in tutte le modalità; verifica anche con una seconda DAW. | Must |

## 12. Proposta tecnica (da confermare)

- **Linguaggio e framework**: C++17/20 con **JUCE** (VST3, GUI, gestione audio/MIDI/file). Build con CMake.
- **Principio sulle librerie**: uso personale e mai commercializzato, quindi si scelgono le **migliori librerie disponibili a prescindere dalla licenza** (GPL/AGPL incluse). Unico vincolo: restano escluse le librerie a pagamento/proprietarie che richiedono l'acquisto (es. zplane élastique) e il plugin non va redistribuito, altrimenti le licenze GPL/AGPL imporrebbero la pubblicazione del sorgente.
- **Time-stretch/pitch**: **Rubber Band** (motore R3/"Finer", la migliore qualità open source, con modalità real-time e preset ritmico/tonale) come motore principale; **Signalsmith Stretch** come alternativa leggera per confronti A/B e per slice molto corti.
- **Analisi audio (BPM, beat, transienti)**: **Essentia** (AGPL, beat tracking e onset detection allo stato dell'arte) per rilevamento automatico del BPM e degli slice; **aubio** come alternativa più leggera.
- **Decodifica file**: moduli audio di JUCE (WAV, AIFF, FLAC, OGG) più **libsndfile**/**minimp3** per MP3 e formati aggiuntivi, uniformi su Windows e macOS.
- **DSP effetti**: modulo `juce_dsp` per filtri/EQ/compressore, più implementazioni di riferimento di alta qualità per riverbero (es. Dattorro/Zita), saturazione con oversampling e filtri ladder (es. modelli Moog/TPT); scelta puntuale nella fase di sviluppo degli effetti.
- **Resampling**: **libsamplerate** (Secret Rabbit Code) o r8brain-free-src per sample-rate conversion di massima qualità.
- **Serializzazione kit**: JSON (nlohmann/json o `juce::var`).
- **Build multi-piattaforma**: CMake + CI (es. GitHub Actions) con runner Windows e macOS; la build per Mac richiede macOS (o CI) e, per uso locale, firma ad-hoc del bundle VST3.
- **Slicing**: rilevamento transienti con onset detection (spectral flux) e beat tracking per il BPM; candidate librerie: aubio, o implementazione propria.
- **Uscite multiple**: bus audio multipli in JUCE (AudioProcessor con più output bus); attenzione al supporto variabile tra DAW.
- **Marchio**: "Battery" e il suo aspetto sono di Native Instruments; ci si ispira a concetto e layout, ma nome, grafica e asset saranno originali.

## 13. Domande aperte

**Risolte (v0.3)**: pitch con Key Lock stile VirtualDJ (PT-2); griglia 4×12 espandibile fino a **128 celle** (UI-5); uso personale; Windows + macOS; uscite per cella (FX-6); slicing in v1 con distribuzione orizzontale per riga (sez. 8.1); mapping automatico da C1 una nota per cella, con override manuale (MAP-0/MAP-1); AU/Logic escluso; DAW di riferimento Cubase 16.

Formato kit deciso: file `.slkit` (JSON) + cartella di sample collegata (sez. 10).

**Ancora aperte**: nessuna.
Struttura del sequencer decisa: una riga per cella, aggiunta con il tasto destro sulla cella (SQ-1, SQ-12).

Esportazione verso Cubase decisa: clip MIDI (drag & drop o file `.mid`) per le note, file audio WAV/AIFF/FLAC per il suono con effetti, da importare nella DAW dall'esterno. L'uscita MIDI in tempo reale (SQ-7c) resta una prova facoltativa.

## 14. Roadmap indicativa

1. **MVP**: plugin VST3 (Win + Mac) con griglia 4×12, browser file, mapping MIDI, trigger one-shot/gate, start/end, AHDSR, tune/pitch, uscite per cella, salvataggio kit.
2. **Effetti**: catena per cella e globale (standard poi creativi).
3. **Loop**: sync al BPM, time-stretch con Key Lock.
4. **Slicing**: slice automatici/manuali e distribuzione sulle celle.
5. **Sequencer**: pattern a 64 step, parametri per step, parameter lock degli effetti, esportazione MIDI ed esportazione audio WAV.
6. **Standalone**: build autonoma Windows/macOS con trasporto interno, impostazioni audio/MIDI e record. Il target standalone si imposta fin dallo scheletro del progetto (JUCE) e serve anche come banco di prova rapido durante lo sviluppo, senza passare dalla DAW.
7. **MIDI learn dei parametri** (sez. 5.1): assegnazione con tasto destro, pannello delle mappature, salvataggio nel kit; da avviare quando i parametri degli effetti sono stabili.
8. **Rifinitura**: layer/gruppi, undo, ottimizzazioni, test in Cubase 16 (e altre DAW).
