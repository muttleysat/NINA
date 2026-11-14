#pragma once

#include "Kit.h"

namespace sl::KitIO
{
// Serializza il kit in JSON. Con relativePaths = true i sample sono scritti come nome file relativo
// alla cartella del kit (formato .slkit); altrimenti come percorsi assoluti (stato del progetto DAW).
juce::String toJson (const Kit& kit, bool relativePaths);

// Legge il JSON. I percorsi relativi si risolvono in <baseDir>/<samplesFolder>/<file>.
// Non carica i campioni audio (lo fa il chiamante); imposta 'missing' se il file non esiste.
bool fromJson (const juce::String& text, const juce::File& baseDir, Kit& kit, juce::String& error);

// Salva "<dir>/<Nome>.slkit" e copia i sample usati in "<dir>/<Nome>/" (stesso nome del kit).
// Aggiorna kit.name, kit.file e i percorsi dei sample (che puntano alla copia).
bool save (Kit& kit, const juce::File& slkitFile, juce::String& error);

// Apre un file .slkit (non carica i campioni audio).
bool load (const juce::File& slkitFile, Kit& kit, juce::String& error);

// File .slseq: i 16 pattern del sequencer (con il pattern attivo), senza suoni e senza song
juce::String sequencesToJson (const Kit& kit);
bool sequencesFromJson (const juce::String& text, Kit& kit, juce::String& error);     // sostituisce kit.patterns e kit.activePattern

// File .slsong: la sola catena di pattern (i pattern restano nel kit; i blocchi li richiamano per indice)
juce::String songToJson (const Song& song);
bool songFromJson (const juce::String& text, Song& song, juce::String& error);

// Rende valido il nome come nome file/cartella (caratteri non ammessi sostituiti con '_')
juce::String sanitiseName (const juce::String& name);
}
