#include "KitIO.h"

namespace sl::KitIO
{
juce::String sanitiseName (const juce::String& name)
{
    auto s = juce::File::createLegalFileName (name.trim()).trim();
    return s.isEmpty() ? juce::String ("Untitled") : s;
}

static juce::var cellToVar (const CellParams& c, const juce::String& samplePath)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("name", c.name);
    o->setProperty ("colour", juce::String::toHexString ((int) c.colour));
    o->setProperty ("sample", samplePath);
    o->setProperty ("start", c.start);
    o->setProperty ("end", c.end);
    o->setProperty ("levelDb", c.levelDb);
    o->setProperty ("pan", c.pan);
    o->setProperty ("tuneCents", c.tuneCents);
    o->setProperty ("pitchSemis", c.pitchSemis);
    o->setProperty ("reverse", c.reverse);
    o->setProperty ("mode", (int) c.mode);
    o->setProperty ("attack", c.attack);
    o->setProperty ("hold", c.hold);
    o->setProperty ("decay", c.decay);
    o->setProperty ("sustain", c.sustain);
    o->setProperty ("release", c.release);
    o->setProperty ("rootNote", c.rootNote);
    o->setProperty ("lowNote", c.lowNote);
    o->setProperty ("highNote", c.highNote);
    return juce::var (o);
}

juce::String toJson (const Kit& kit, bool relativePaths)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("schemaVersion", Kit::kSchemaVersion);
    root->setProperty ("name", kit.name);
    root->setProperty ("samplesFolder", kit.name);
    root->setProperty ("rows", kit.rows);
    root->setProperty ("cols", kit.cols);
    root->setProperty ("masterDb", kit.masterDb);

    juce::Array<juce::var> cells;
    for (const auto& c : kit.cells)
    {
        juce::String path = c.samplePath;
        if (relativePaths && path.isNotEmpty())
            path = juce::File (path).getFileName();
        cells.add (cellToVar (c, path));
    }
    root->setProperty ("cells", cells);

    return juce::JSON::toString (juce::var (root), false);
}

static float getF (const juce::var& o, const char* key, float def)
{
    return o.hasProperty (key) ? (float) (double) o[key] : def;
}

static int getI (const juce::var& o, const char* key, int def)
{
    return o.hasProperty (key) ? (int) o[key] : def;
}

bool fromJson (const juce::String& text, const juce::File& baseDir, Kit& kit, juce::String& error)
{
    const auto root = juce::JSON::parse (text);
    if (! root.isObject() || ! root["cells"].isArray())
    {
        error = "File kit non valido";
        return false;
    }

    const int version = getI (root, "schemaVersion", 1);
    if (version > Kit::kSchemaVersion)
    {
        error = "Il kit e' stato creato con una versione piu' recente di Sample Lab";
        return false;
    }

    Kit k;
    k.name = root["name"].toString();
    if (k.name.isEmpty())
        k.name = "Untitled";
    k.rows = juce::jlimit (1, 64, getI (root, "rows", 4));
    k.cols = juce::jlimit (1, 64, getI (root, "cols", 12));
    k.masterDb = getF (root, "masterDb", 0.0f);

    const auto folder = root["samplesFolder"].toString().isEmpty() ? k.name : root["samplesFolder"].toString();

    const auto* arr = root["cells"].getArray();
    const int count = juce::jmin (arr->size(), Kit::kMaxCells);
    k.cells.resize ((size_t) count);

    for (int i = 0; i < count; ++i)
    {
        const auto& o = (*arr)[i];
        auto& c = k.cells[(size_t) i];
        c.name = o["name"].toString();
        c.colour = (juce::uint32) o["colour"].toString().getHexValue32();
        if (c.colour == 0) c.colour = 0xff3ec1d3;
        c.start = getF (o, "start", 0.0f);
        c.end = getF (o, "end", 1.0f);
        c.levelDb = getF (o, "levelDb", 0.0f);
        c.pan = getF (o, "pan", 0.0f);
        c.tuneCents = getF (o, "tuneCents", 0.0f);
        c.pitchSemis = getF (o, "pitchSemis", 0.0f);
        c.reverse = (bool) o["reverse"];
        c.mode = (TriggerMode) juce::jlimit (0, 2, getI (o, "mode", 0));
        c.attack = getF (o, "attack", 0.0005f);
        c.hold = getF (o, "hold", 0.0f);
        c.decay = getF (o, "decay", 0.1f);
        c.sustain = getF (o, "sustain", 1.0f);
        c.release = getF (o, "release", 0.02f);
        const int defNote = juce::jlimit (0, 127, Kit::kFirstNote + i);
        c.rootNote = getI (o, "rootNote", defNote);
        c.lowNote = getI (o, "lowNote", defNote);
        c.highNote = getI (o, "highNote", defNote);

        const auto rel = o["sample"].toString();
        if (rel.isNotEmpty())
        {
            const juce::File f = juce::File::isAbsolutePath (rel)
                                     ? juce::File (rel)
                                     : baseDir.getChildFile (folder).getChildFile (rel);
            c.samplePath = f.getFullPathName();
            c.missing = ! f.existsAsFile();
        }
    }

    kit = std::move (k);
    return true;
}

bool save (Kit& kit, const juce::File& slkitFile, juce::String& error)
{
    const auto name = sanitiseName (slkitFile.getFileNameWithoutExtension());
    const auto dir = slkitFile.getParentDirectory();
    const auto file = dir.getChildFile (name + ".slkit");
    const auto samplesDir = dir.getChildFile (name);

    if (! samplesDir.createDirectory().wasOk())
    {
        error = "Impossibile creare la cartella " + samplesDir.getFullPathName();
        return false;
    }

    for (auto& c : kit.cells)
    {
        if (! c.hasSample())
            continue;

        const juce::File src (c.samplePath);
        if (! src.existsAsFile())
            continue;                           // sample mancante: si conserva il riferimento

        if (src.getParentDirectory() == samplesDir)
            continue;                           // gia' nella cartella del kit

        auto dst = samplesDir.getChildFile (src.getFileName());
        for (int n = 2; dst.existsAsFile() && ! dst.hasIdenticalContentTo (src); ++n)
            dst = samplesDir.getChildFile (src.getFileNameWithoutExtension() + "_" + juce::String (n) + src.getFileExtension());

        if (! dst.existsAsFile() && ! src.copyFileTo (dst))
        {
            error = "Impossibile copiare il sample " + src.getFileName();
            return false;
        }

        c.samplePath = dst.getFullPathName();
        c.missing = false;
    }

    kit.name = name;
    kit.file = file;

    if (! file.replaceWithText (toJson (kit, true), false, false, "\n"))
    {
        error = "Impossibile scrivere " + file.getFullPathName();
        return false;
    }
    return true;
}

bool load (const juce::File& slkitFile, Kit& kit, juce::String& error)
{
    if (! slkitFile.existsAsFile())
    {
        error = "File non trovato: " + slkitFile.getFullPathName();
        return false;
    }

    Kit k;
    if (! fromJson (slkitFile.loadFileAsString(), slkitFile.getParentDirectory(), k, error))
        return false;

    // Il nome del kit coincide sempre con il nome del file (KIT-1b)
    k.name = slkitFile.getFileNameWithoutExtension();
    k.file = slkitFile;

    // I percorsi relativi sono risolti rispetto alla cartella col nome del file
    for (auto& c : k.cells)
    {
        if (! c.hasSample())
            continue;
        const juce::File f (c.samplePath);
        if (! f.existsAsFile())
        {
            const auto alt = slkitFile.getParentDirectory().getChildFile (k.name).getChildFile (f.getFileName());
            if (alt.existsAsFile())
            {
                c.samplePath = alt.getFullPathName();
                c.missing = false;
            }
        }
    }

    kit = std::move (k);
    return true;
}
}
