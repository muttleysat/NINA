#include "PadGrid.h"

PadGrid::PadGrid (SampleLabProcessor& p) : processor (p)
{
    startTimerHz (30);
}

PadGrid::~PadGrid()
{
    if (heldNote >= 0)
        processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
}

void PadGrid::setSelected (int cell)
{
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), cell);
    if (cell == selected)
        return;
    selected = cell;
    repaint();
    if (onSelectionChanged)
        onSelectionChanged (selected);
}

juce::Rectangle<float> PadGrid::cellBounds (int index) const
{
    const auto& kit = processor.getKit();
    const float gap = 5.0f * theme::scaleFor (*this);
    const float cw = ((float) getWidth() - gap * (float) (kit.cols - 1)) / (float) kit.cols;
    const float ch = ((float) getHeight() - gap * (float) (kit.rows - 1)) / (float) kit.rows;
    const int r = index / kit.cols, c = index % kit.cols;
    return { (float) c * (cw + gap), (float) r * (ch + gap), cw, ch };
}

int PadGrid::cellAt (int x, int y) const
{
    for (int i = 0; i < processor.getKit().numCells(); ++i)
        if (cellBounds (i).expanded (2.0f).contains ((float) x, (float) y))
            return i;
    return -1;
}

void PadGrid::setHover (int cell)
{
    if (cell != hover)
    {
        hover = cell;
        repaint();
    }
}

void PadGrid::timerCallback()
{
    const int n = processor.getKit().numCells();
    bool changed = (int) lastActivity.size() != n;
    lastActivity.resize ((size_t) n, 0);
    for (int i = 0; i < n; ++i)
    {
        const int a = processor.getCellActivity (i);
        if (a != lastActivity[(size_t) i])
        {
            lastActivity[(size_t) i] = a;
            changed = true;
        }
    }
    if (changed)
        repaint();
}

void PadGrid::paint (juce::Graphics& g)
{
    const auto& kit = processor.getKit();
    const float s = theme::scaleFor (*this);

    for (int i = 0; i < kit.numCells(); ++i)
    {
        const auto& c = kit.cells[(size_t) i];
        const auto b = cellBounds (i);
        const bool filled = c.hasSample();
        const bool active = i < (int) lastActivity.size() && lastActivity[(size_t) i] > 0;
        const auto col = juce::Colour (c.colour);

        // Corpo
        juce::ColourGradient grad (filled ? col.withAlpha (0.35f) : theme::panelHi, b.getX(), b.getY(),
                                   filled ? col.withAlpha (0.18f) : theme::panel, b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 6.0f * s);

        if (active)
        {
            g.setColour (col.withAlpha (0.55f));
            g.fillRoundedRectangle (b, 6.0f * s);
        }

        // Bordo
        const bool sel = (i == selected);
        g.setColour (sel ? theme::accent : (i == hover ? theme::text.withAlpha (0.8f) : (filled ? col.withAlpha (0.8f) : theme::textDim.withAlpha (0.25f))));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f * s, sel ? 2.2f * s : 1.0f);

        // Etichette
        g.setColour (theme::text.withAlpha (filled ? 0.95f : 0.4f));
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.0f * s), juce::Font::bold));
        g.drawFittedText (c.name, b.reduced (5.0f * s, 3.0f * s).toNearestInt().withTrimmedTop ((int) (12 * s)), juce::Justification::centred, 2);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (juce::jmax (8.0f, 9.5f * s)));
        const juce::String id = juce::String::charToString ((juce::juce_wchar) ('A' + i / kit.cols)) + juce::String (i % kit.cols + 1);
        g.drawText (id, b.reduced (5.0f * s, 3.0f * s).toNearestInt(), juce::Justification::topLeft);
        g.drawText (theme::noteName (c.lowNote), b.reduced (5.0f * s, 3.0f * s).toNearestInt(), juce::Justification::topRight);

        if (c.missing)
        {
            g.setColour (theme::danger);
            g.drawText ("MISSING", b.reduced (5.0f * s, 3.0f * s).toNearestInt(), juce::Justification::bottomLeft);
        }
    }
}

void PadGrid::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.x, e.y);
    if (cell < 0)
        return;

    setSelected (cell);

    if (e.mods.isPopupMenu())
    {
        showContextMenu (cell);
        return;
    }

    // Anteprima: suona la nota della cella
    const auto& c = processor.getKit().cells[(size_t) cell];
    if (c.hasSample() && ! c.missing)
    {
        heldNote = c.lowNote;
        processor.getKeyboardState().noteOn (1, heldNote, 0.8f);
    }
}

void PadGrid::mouseUp (const juce::MouseEvent&)
{
    if (heldNote >= 0)
    {
        processor.getKeyboardState().noteOff (1, heldNote, 0.0f);
        heldNote = -1;
    }
}

void PadGrid::showContextMenu (int cell)
{
    juce::PopupMenu m;
    const bool filled = processor.getKit().cells[(size_t) cell].hasSample();
    m.addItem (1, "Load sample...");
    m.addItem (2, "Rename...", filled);
    m.addItem (3, "Clear cell", filled);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (cellBounds (cell).toNearestInt())),
                     [this, cell] (int r)
                     {
                         if (r == 1 && onLoadRequested) onLoadRequested (cell);
                         else if (r == 2) renameCell (cell);
                         else if (r == 3)
                         {
                             processor.clearCell (cell);
                             if (onKitChanged) onKitChanged();
                         }
                     });
}

void PadGrid::renameCell (int cell)
{
    auto* w = new juce::AlertWindow ("Rename cell", "Cell name:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", processor.getKit().cells[(size_t) cell].name);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PadGrid> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, cell] (int r)
    {
        if (r == 1 && safe != nullptr)
        {
            safe->processor.getKit().cells[(size_t) cell].name = w->getTextEditorContents ("name");
            safe->processor.cellEdited (cell);
            safe->repaint();
            if (safe->onKitChanged) safe->onKitChanged();
        }
    }), true);
}

void PadGrid::filesDropped (const juce::StringArray& files, int x, int y)
{
    setHover (-1);
    const int cell = cellAt (x, y);
    if (cell >= 0 && files.size() > 0 && onFileDropped)
    {
        // Piu' file: riempie le celle successive in ordine
        for (int i = 0; i < files.size() && cell + i < processor.getKit().numCells(); ++i)
            onFileDropped (cell + i, juce::File (files[i]));
    }
}

void PadGrid::itemDropped (const SourceDetails& d)
{
    setHover (-1);
    const int cell = cellAt (d.localPosition.x, d.localPosition.y);
    if (cell >= 0 && onBrowserDrop)
        onBrowserDrop (cell);
}
