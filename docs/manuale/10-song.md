# 10. Song (Pattern Sequencer)

La scheda **Song** mette in fila i pattern che hai creato nella scheda **Seq** per formare un brano intero: intro, strofa, ritornello... Ogni pezzo della catena è un **blocco** (un pattern suonato un certo numero di volte). I blocchi suonano uno dopo l'altro.

I **pattern restano nel kit** e si modificano nella scheda Seq: se cambi un pattern, cambia anche ovunque è usato nella song.

## Costruire la song

1. Nella scheda **Seq** prepara i pattern (fino a 16).
2. Apri la scheda **Song** e clicca **+ Block**: scegli il pattern da aggiungere in fondo alla catena. Il pattern attivo nella scheda Seq è segnato.
3. Ogni blocco è una scheda con:
   - il **pattern** (menu a tendina: puoi cambiarlo quando vuoi);
   - le **ripetizioni** (**−/+** o trascina il numero): un "giro" è un ciclo del pattern, lungo quanto la sua riga più lunga (di solito una battuta di 16 step a 1/16). In alto a destra della scheda vedi la durata in battute;
   - **<** e **>** per spostare il blocco prima o dopo;
   - **Dup** per duplicarlo (il duplicato va subito dopo);
   - **x** per toglierlo.
   I blocchi **vanno a capo** su più righe quando non stanno in una sola: quanti ne entrano per riga dipende dalla larghezza della finestra. Se le righe diventano troppe per l'altezza disponibile, le schede si **rimpiccioliscono da sole** (fino a una dimensione ancora leggibile); con un numero ancora più alto compare una **barra di scorrimento verticale**. Aggiungendo un blocco la vista scorre fino all'ultimo.
4. Sotto i blocchi, la **mappa della song** mostra tutta la catena in scala: ogni blocco è colorato come il suo pattern, con numero del pattern e ripetizioni, e c'è la **lunghezza totale** (battute, minuti:secondi al BPM corrente).
5. Il pulsante **⋯** accanto a **+ Block** offre **Add every pattern that has notes** (un blocco per ogni pattern con righe, in ordine) e **Clear the song**.

Il **nome** della song (campo accanto a Open/Save) si usa per i nomi dei file esportati.

## Suonare la song

- Il **Play** (barra in alto o barra spaziatrice) è legato alla **scheda aperta**: nella scheda **Song** suona la **catena dall'inizio**; in tutte le altre (Main, Seq, FX...) suona come sempre il **pattern** selezionato nella scheda Seq. Nella scheda Song lo ricorda la scritta «Play here plays the song». Se cambi scheda mentre sta suonando, la riproduzione continua com'era; il nuovo modo vale dal prossimo Play.
- Durante l'ascolto il blocco in riproduzione si illumina con una barra di avanzamento e sulla mappa corre il cursore.
- **Loop** acceso: alla fine la song ricomincia dal primo blocco. Spento: alla fine il Play si ferma (e il prossimo Play riparte dall'inizio).
- Nel **plugin**, quando la song è quella che suona, la posizione segue la DAW: la song parte dal punto zero del progetto. Con il Loop acceso si ripete quando la DAW supera la fine.
- Ogni pattern parte sempre dall'inizio quando comincia il suo blocco. Le note ancora tenute (modalità Gate o Loop) si chiudono alla fine di ogni blocco.
- Mentre la song suona puoi modificare i pattern nella scheda Seq: il cambiamento si sente subito.

## Salvare e aprire la song

- **Save song** scrive un file **.slsong** (testo leggibile) con la sola catena di blocchi, il nome e il Loop. Il file propone la cartella `Songs` accanto al kit.
- **Open song** carica una catena da un file .slsong al posto di quella attuale.
- La song è **salvata anche dentro il kit**: con **Save** del kit non perdi la catena. Il file .slsong serve per tenere più versioni o scambiare catene tra kit: i blocchi richiamano i pattern per numero (1–16), quindi funzionano con i pattern del kit in cui li apri.
- La song si annulla con **Ctrl+Z** come il resto (aggiungere, spostare, togliere blocchi, ripetizioni, Loop).

## Esportare la song

Gli stessi comandi della scheda Seq, ma sull'intera catena. La lunghezza è quella della song (non serve scegliere le battute).

| Comando | Cosa fa |
|---------|---------|
| **Export MIDI** | Tutta la song in un solo clip MIDI. Ratchet, nudge, swing e probabilità sono già risolti in note; reverse e lock degli effetti non esistono nel MIDI |
| **Bounce WAV** | Tutta la song in un unico file audio a 24 bit, con effetti, lock, delay sincronizzato ed effetti master |
| **Export Lanes** | Un file WAV per ogni cella suonata nella song (tutti della stessa lunghezza), con gli effetti della cella ma senza quelli master |
| **Drag MIDI / Drag audio / Drag lanes** | Come i tre comandi precedenti, ma trascinando direttamente sulle tracce della DAW |
| **Tail** (cut / loop / keep) | Come trattare le code di delay e riverbero: tagliate a fine song, rientranti all'inizio (per un loop) o conservate (file più lungo). È la stessa impostazione della scheda Seq |

Il tempo usato è quello corrente (DAW o riquadro BPM). Come per il sequencer, gli export usano i valori **correnti** dei controlli, non l'automazione della DAW.

## Cose da sapere

- La song ha al massimo **128 blocchi**, ciascuno con **1–99** ripetizioni.
- Un pattern senza righe suona come silenzio per la durata di una battuta.
- La durata di un giro è quella della riga **più lunga** del pattern: una riga più corta si ripete dentro il giro.
- La riproduzione parte sempre dall'inizio: non c'è ancora la partenza da un blocco scelto né il loop di una sola parte.
- Una song fatta di pattern diversi non cambia il BPM: il tempo resta unico per tutto il brano.
