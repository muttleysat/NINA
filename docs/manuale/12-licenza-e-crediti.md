# 12. Licenza e crediti

## La licenza di NINA

NINA è **software libero**, rilasciato con la licenza **GNU Affero General Public License, versione 3 (AGPL-3.0)**. Il testo completo è nel file `LICENSE` del progetto e nella finestra **Info → Licenses** del programma.

In pratica:
- puoi **usarlo, copiarlo e distribuirlo gratuitamente**;
- il **codice sorgente** è disponibile a chiunque riceva il programma (finestra **Info → Source code**);
- chi lo modifica e lo distribuisce deve rilasciare le sue modifiche con la stessa licenza.

Il **nome, il logo e l'icona di NINA** appartengono all'autore (Vinz Trombetta) e non sono coperti dalla licenza del codice.

## Le librerie usate

NINA è costruito con componenti di terzi, ognuno con la sua licenza (elenco completo nel file `THIRD_PARTY_NOTICES.md` e in **Info → Licenses → Third-party notices**):

| Componente | A cosa serve | Licenza |
|------------|--------------|---------|
| **JUCE** | Interfaccia, plugin e lettura/scrittura dei file audio | AGPL-3.0 |
| **Rubber Band Library** | Stretch dei loop (Sync to tempo, Key lock, slice) | GPL 2 o successiva |
| **Steinberg VST 3 SDK** | Il formato del plugin | GPL 3 |
| **Steinberg ASIO SDK** | I driver audio ASIO dello standalone (solo Windows) | Licenza Steinberg ASIO |

## Marchi

- **VST è un marchio di Steinberg Media Technologies GmbH**, registrato in Europa e in altri paesi.
- **ASIO è un marchio registrato e un software di Steinberg Media Technologies GmbH.** Lo standalone per Windows supporta i driver ASIO:

  ![ASIO Compatible](../img/asio_compatible_logo.jpg)

- Il logo **PayPal** è un marchio di PayPal, Inc. e compare solo sul pulsante della donazione volontaria.

## Versioni e aggiornamenti

Il numero di versione (per esempio **1.0.0**) è nella finestra **Info**. Le nuove versioni si pubblicano nelle **Release** di GitHub (`github.com/muttleysat/NINA/releases`): per aggiornare si scarica il nuovo installer e lo si esegue; **sostituisce** la versione precedente mantenendo le impostazioni. Il pulsante **Check for updates** nella finestra Info controlla se esiste una versione più recente. Il programma **non si collega mai da solo**: la richiesta parte solo quando premi il pulsante.

## Donazioni

NINA è gratuito e **uguale per tutti**. Se ti piace, nella finestra **Info** il pulsante **Support my work** (con il logo PayPal) apre la pagina per una **donazione libera e volontaria**: nessun obbligo, e non cambia nulla nel programma.
