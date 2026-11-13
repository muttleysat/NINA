# 9. Slice: tagliare un loop sulle celle

La scheda **Slice** (ultima in basso, con l'icona delle forbici) taglia il campione di una cella in **slice** e le distribuisce sulle celle successive: un loop di batteria diventa una cella per colpo.

## Come funziona

1. In **Main** seleziona la cella con il loop (per esempio A10) e apri la scheda **Slice**. Il campione è quello della cella selezionata; la parte tagliata è quella tra **Start** ed **End** della cella.
2. Trova i punti di taglio:
   - **Detect** mette un marcatore su ogni **colpo** trovato. La manopola **Sensitivity** regola quanto è sensibile: più alta trova più colpi, anche deboli e ravvicinati. Se la cambi, il rilevamento si rifà da solo.
   - **Even: 1/2 … 1/32** taglia la regione in parti **uguali**.
   - **Clear** toglie tutti i marcatori (resta quello dell'inizio).
3. Rifinisci a mano sulla forma d'onda grande:
   - **Clic su una slice**: la senti (se **Audition** è acceso; accanto c'è il volume dell'anteprima).
   - **Trascina un marcatore** per spostarlo (la bandierina numerata in alto).
   - **Doppio clic** sulla forma d'onda per **aggiungere** un marcatore.
   - **Tasto destro** su un marcatore per **toglierlo**.
   - I pulsanti **− [ ] +** (in basso a destra) zoomano; con la rotella scorri quando sei zoomato, con Ctrl+rotella zoomi. Le parentesi arancioni sono Start ed End della cella, che si cambiano nella scheda Main.
   - Passando il mouse su una slice si vede la sua **durata in ms**.
4. La striscia **CELLS** mostra le celle che verranno riempite: in arancione con il **numero della slice**, la cella originale con il contorno bianco (**original**), le celle già occupate in rosso con un triangolo e la scritta **occupied** (verrebbero sostituite). **Clicca una cella** per far partire la distribuzione da lì.
5. **Keep original in A10** (acceso): le slice vanno nelle celle **dopo** l'originale, che resta com'è. Spento: partono dalla cella stessa del loop.
6. **Distribute to cells** riempie le celle. Se alcune sono occupate chiede conferma.

La scritta in basso a sinistra riassume tutto: numero di slice, da quale a quale cella, quante verranno sovrascritte.

## Cosa succede alle celle

- Ogni cella riceve lo **stesso file** del loop (nessuna copia dei dati: non occupa memoria in più) con il proprio **Start/End** dell'inizio e della fine della slice. Il nome è quello del loop più un numero (`amen 01`, `amen 02`…).
- Le celle sono celle normali: ognuna ha il suo tasto MIDI (dal **C1** in poi, come sempre), il suo inviluppo, gli effetti, e si possono aggiungere al sequencer. Per rifinire una slice usa Start/End nella scheda Main.
- I marcatori restano salvati con il kit: riaprendo la scheda Slice sulla stessa cella ritrovi i tagli.
- Se le slice sono più delle celle libere in fondo alla griglia (48 in tutto), vengono riempite solo quelle disponibili e la scritta in basso lo avvisa.

## Limiti

- Il rilevamento dei colpi funziona bene su percussioni con attacchi netti (batterie, loop ritmici). Su materiale tenuto o molto denso (pad, musica completa) trova molti punti o pochi: abbassa o alza **Sensitivity** e correggi a mano.
- I tagli cadono sul punto in cui sale il suono; non c'è ancora l'allineamento alla griglia del tempo.
- La scheda lavora su una cella alla volta e non ha ancora annullamento (undo): se sbagli una distribuzione, svuota le celle (tasto destro → Clear cell) e ripeti.
