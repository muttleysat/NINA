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

std::vector<int> PadGrid::selectedCells() const
{
    std::set<int> all = multi;
    all.insert (selected);
    std::vector<int> out;
    for (const int c : all)
        if (c >= 0 && c < processor.getKit().numCells())
            out.push_back (c);
    return out;
}

void PadGrid::selectAll()
{
    multi.clear();
    for (int i = 0; i < processor.getKit().numCells(); ++i)
        multi.insert (i);
    repaint();
}

void PadGrid::clearMultiSelection()
{
    if (multi.empty())
        return;
    multi.clear();
    repaint();
}

void PadGrid::clearSelectedCells()
{
    std::vector<int> toClear;
    for (const int c : selectedCells())
        if (processor.getKit().cells[(size_t) c].hasSample())
            toClear.push_back (c);
    if (toClear.empty())
        return;

    const int n = (int) toClear.size();
    juce::Component::SafePointer<PadGrid> safe (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle ("Clear cells")
                                      .withMessage (n == 1 ? "Remove the sample from the selected cell?"
                                                           : "Remove the samples from the " + juce::String (n) + " selected cells?")
                                      .withButton ("Clear")
                                      .withButton ("Cancel")
                                      .withAssociatedComponent (this),
                                  [safe, toClear] (int r)
                                  {
                                      if (r != 1 || safe == nullptr)
                                          return;
                                      for (const int c : toClear)
                                          safe->processor.clearCell (c);
                                      safe->multi.clear();
                                      safe->repaint();
                                      if (safe->onKitChanged)
                                          safe->onKitChanged();
                                  });
}

void PadGrid::setSelected (int cell)
{
    cell = juce::jlimit (0, juce::jmax (0, processor.getKit().numCells() - 1), cell);
    if (cell == selected)
        return;
    selected = cell;
    anchor = cell;
    repaint();
    if (onSelectionChanged)
        onSelectionChanged (selected);
}

float PadGrid::labelWidth() const
{
    return 22.0f * theme::scaleFor (*this);
}

juce::Rectangle<float> PadGrid::cellBounds (int index) const
{
    const auto& kit = processor.getKit();
    const float gap = 7.0f * theme::scaleFor (*this);
    const float x0 = labelWidth();
    const float cw = ((float) getWidth() - x0 - gap * (float) (kit.cols - 1)) / (float) kit.cols;
    const float ch = ((float) getHeight() - gap * (float) (kit.rows - 1)) / (float) kit.rows;
    const int r = index / kit.cols, c = index % kit.cols;
    return { x0 + (float) c * (cw + gap), (float) r * (ch + gap), cw, ch };
}

int PadGrid::cellAt (int x, int y) const
{
    for (int i = 0; i < processor.getKit().numCells(); ++i)
        if (cellBounds (i).expanded (3.0f).contains ((float) x, (float) y))
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

void PadGrid::updateThumb (Thumb& t, const sl::SampleData* data, int width)
{
    t.data = data;
    t.width = width;
    t.peaks.clear();
    if (data == nullptr || width <= 0)
        return;

    const int n = data->numSamples();
    t.peaks.resize ((size_t) width);
    for (int x = 0; x < width; ++x)
    {
        const int a = (int) ((int64_t) x * n / width);
        const int b = juce::jmin (n, juce::jmax (a + 1, (int) ((int64_t) (x + 1) * n / width)));
        float p = 0.0f;
        for (int ch = 0; ch < data->numChannels(); ++ch)
        {
            const float* d = data->audio.getReadPointer (ch);
            for (int i = a; i < b; ++i)
                p = juce::jmax (p, std::abs (d[i]));
        }
        t.peaks[(size_t) x] = juce::jmin (1.0f, p);
    }
}

juce::String PadGrid::rangeText (const sl::CellParams& c)
{
    const int lo = juce::jmin (c.lowNote, c.highNote), hi = juce::jmax (c.lowNote, c.highNote);
    return lo == hi ? theme::noteName (lo) : theme::noteName (lo) + " - " + theme::noteName (hi);
}

PadGrid::RangeInfo PadGrid::rangeInfo (int cell) const
{
    const auto& kit = processor.getKit();
    RangeInfo info;
    const auto& me = kit.cells[(size_t) cell];
    const int lo = juce::jmin (me.lowNote, me.highNote), hi = juce::jmax (me.lowNote, me.highNote);

    for (int j = 0; j < kit.numCells(); ++j)
    {
        if (j == cell)
            continue;
        const auto& o = kit.cells[(size_t) j];
        if (! o.hasSample())
            continue;
        const int olo = juce::jmin (o.lowNote, o.highNote), ohi = juce::jmax (o.lowNote, o.highNote);
        if (me.hasSample())
        {
            if (olo <= hi && ohi >= lo)
                info.overlaps.push_back (j);
        }
        else if (info.coveredBy < 0 && lo >= olo && lo <= ohi)      // la nota di questo pad vuoto rientra nel range di j
            info.coveredBy = j;
    }
    return info;
}

juce::String PadGrid::getTooltip()
{
    const int i = cellAt (getMouseXYRelative().x, getMouseXYRelative().y);
    if (i < 0)
        return {};

    const auto& kit = processor.getKit();
    const auto& c = kit.cells[(size_t) i];
    const auto info = rangeInfo (i);
    juce::String t = theme::cellId (i, kit.cols) + ": key " + rangeText (c);
    if (info.coveredBy >= 0)
        t += "\nThis pad is empty but its key is covered by " + theme::cellId (info.coveredBy, kit.cols)
             + " (" + rangeText (kit.cells[(size_t) info.coveredBy]) + "): playing it triggers that cell";
    if (! info.overlaps.empty())
    {
        t += "\nPlays together with: ";
        for (size_t k = 0; k < info.overlaps.size(); ++k)
            t += (k ? ", " : "") + theme::cellId (info.overlaps[k], kit.cols);
        t += " (shared keys)";
    }
    return t;
}

void PadGrid::paint (juce::Graphics& g)
{
    const auto& kit = processor.getKit();
    const float s = theme::scaleFor (*this);
    thumbs.resize ((size_t) kit.numCells());

    // Lettere di riga
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (juce::jmax (10.0f, 13.0f * s), juce::Font::bold));
    for (int r = 0; r < kit.rows; ++r)
    {
        const auto first = cellBounds (r * kit.cols);
        g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + r)),
                    juce::Rectangle<float> (0.0f, first.getY(), labelWidth() - 4.0f * s, first.getHeight()),
                    juce::Justification::centredRight);
    }

    for (int i = 0; i < kit.numCells(); ++i)
    {
        const auto& c = kit.cells[(size_t) i];
        const auto b = cellBounds (i);
        const bool filled = c.hasSample();
        const bool active = i < (int) lastActivity.size() && lastActivity[(size_t) i] > 0;
        const bool sel = (i == selected);
        const bool inMulti = multi.count (i) > 0 && ! sel;
        const auto col = juce::Colour (c.colour);
        const float rad = 8.0f * s;

        // Corpo
        g.setColour (juce::Colour (0xff17191d));
        g.fillRoundedRectangle (b, rad);
        if (filled)
        {
            g.setGradientFill (juce::ColourGradient (col.withAlpha (0.20f), b.getX(), b.getY(),
                                                     col.withAlpha (0.03f), b.getX(), b.getBottom(), false));
            g.fillRoundedRectangle (b, rad);
        }
        const auto info = rangeInfo (i);
        if (! filled && info.coveredBy >= 0)
        {
            // Pad vuoto ma la sua nota suona un'altra cella (range esteso): banda del colore di quella cella
            const auto src = juce::Colour (kit.cells[(size_t) info.coveredBy].colour);
            g.setColour (src.withAlpha (0.13f));
            g.fillRoundedRectangle (b, rad);
            g.setColour (src.withAlpha (0.35f));
            for (float x = b.getX() - b.getHeight(); x < b.getRight(); x += 9.0f * s)
            {
                juce::Graphics::ScopedSaveState save (g);
                juce::Path clip;
                clip.addRoundedRectangle (b, rad);
                g.reduceClipRegion (clip);
                g.drawLine (x, b.getBottom(), x + b.getHeight(), b.getY(), 1.0f);
            }
        }
        if (active)
        {
            g.setColour (col.withAlpha (0.38f));
            g.fillRoundedRectangle (b, rad);
        }

        // Mini forma d'onda
        if (filled)
        {
            const auto data = processor.getCellData (i);
            const auto waveArea = b.reduced (6.0f * s, 4.0f * s).withTrimmedTop (b.getHeight() * 0.36f);
            auto& t = thumbs[(size_t) i];
            const int w = juce::jmax (1, (int) waveArea.getWidth());
            if (t.data != data.get() || t.width != w)
                updateThumb (t, data.get(), w);

            if (! t.peaks.empty())
            {
                const float mid = waveArea.getCentreY(), half = waveArea.getHeight() * 0.5f;
                juce::Path p;
                p.startNewSubPath (waveArea.getX(), mid);
                for (int x = 0; x < w; ++x)
                    p.lineTo (waveArea.getX() + (float) x, mid - t.peaks[(size_t) x] * half);
                for (int x = w - 1; x >= 0; --x)
                    p.lineTo (waveArea.getX() + (float) x, mid + t.peaks[(size_t) x] * half);
                p.closeSubPath();
                g.setColour (col.withAlpha (c.missing ? 0.2f : 0.9f));
                g.fillPath (p);
            }
        }

        // Selezione multipla: velo arancione e bordo, piu' sobri della cella corrente
        if (inMulti)
        {
            g.setColour (theme::accent.withAlpha (0.14f));
            g.fillRoundedRectangle (b, rad);
        }

        // Bordo; selezione con alone arancione
        if (inMulti)
        {
            g.setColour (theme::accent.withAlpha (0.85f));
            g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.8f * s);
        }
        else if (sel)
        {
            for (int k = 3; k >= 1; --k)
            {
                g.setColour (theme::accent.withAlpha (0.10f * (float) (4 - k)));
                g.drawRoundedRectangle (b.expanded ((float) k * 1.6f * s), rad + (float) k, 1.6f * s);
            }
            g.setColour (theme::accent);
            g.drawRoundedRectangle (b.reduced (0.5f), rad, 2.2f * s);
        }
        else
        {
            g.setColour (i == hover ? theme::text.withAlpha (0.8f) : (filled ? col.withAlpha (0.55f) : theme::border));
            g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.1f);
        }

        // Etichette: ID in grassetto, nome sotto
        const auto textArea = b.reduced (7.0f * s, 5.0f * s);
        g.setColour (theme::text.withAlpha (filled ? 1.0f : 0.5f));
        g.setFont (juce::FontOptions (juce::jmax (9.0f, 12.5f * s), juce::Font::bold));
        g.drawText (theme::cellId (i, kit.cols), textArea.withHeight (15.0f * s), juce::Justification::topLeft);

        if (filled)
        {
            g.setColour (theme::text.withAlpha (0.78f));
            g.setFont (juce::FontOptions (juce::jmax (8.0f, 11.0f * s)));
            g.drawText (c.name, textArea.withTrimmedTop (14.0f * s).withHeight (14.0f * s), juce::Justification::topLeft, true);
        }

        if (c.missing)
        {
            g.setColour (theme::danger);
            g.setFont (juce::FontOptions (juce::jmax (8.0f, 10.0f * s), juce::Font::bold));
            g.drawText ("MISSING", textArea, juce::Justification::bottomLeft);
        }
        else
        {
            // Tasto (o intervallo di tasti) che suona la cella; nei pad vuoti coperti da un'altra cella, quale cella li copre
            g.setFont (juce::FontOptions (juce::jmax (8.0f, 10.0f * s), juce::Font::bold));
            if (! filled && info.coveredBy >= 0)
            {
                g.setColour (juce::Colour (kit.cells[(size_t) info.coveredBy].colour).brighter (0.2f));
                g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\x90 ")) + theme::cellId (info.coveredBy, kit.cols),
                            textArea, juce::Justification::centred);
            }
            g.setColour (theme::text.withAlpha (0.55f));
            if (filled || info.coveredBy >= 0 || c.lowNote != c.highNote)
                g.drawText (rangeText (c), textArea, juce::Justification::bottomLeft);
        }

        // Avviso: due celle con campione condividono dei tasti e suonano insieme
        if (! info.overlaps.empty() && filled)
        {
            const auto tri = juce::Rectangle<float> (b.getRight() - 17.0f * s, b.getY() + 4.0f * s, 12.0f * s, 11.0f * s);
            juce::Path p;
            p.addTriangle (tri.getCentreX(), tri.getY(), tri.getRight(), tri.getBottom(), tri.getX(), tri.getBottom());
            g.setColour (theme::accent);
            g.fillPath (p);
            g.setColour (juce::Colour (0xff1a1006));
            g.setFont (juce::FontOptions (juce::jmax (7.0f, 9.0f * s), juce::Font::bold));
            g.drawText ("!", tri.translated (0.0f, 1.5f * s), juce::Justification::centred);
        }

        // Pallino arancione: la cella e' nel sequencer
        if (processor.activePattern().findLane (i) != nullptr)
        {
            g.setColour (theme::accent);
            g.fillEllipse (b.getRight() - 12.0f * s, b.getBottom() - 12.0f * s, 7.0f * s, 7.0f * s);
        }
    }
}

void PadGrid::mouseDown (const juce::MouseEvent& e)
{
    const int cell = cellAt (e.x, e.y);
    if (cell < 0)
        return;

    // Tasto destro: se la cella non e' gia' nella selezione, la selezione diventa solo lei
    if (e.mods.isPopupMenu())
    {
        if (! isInSelection (cell))
        {
            multi.clear();
            setSelected (cell);
        }
        repaint();
        showContextMenu (cell);
        return;
    }

    // Maiusc+clic: intervallo dall'ultima cella cliccata (nessun suono); l'ancora resta dov'era per poter riallargare l'intervallo
    if (e.mods.isShiftDown())
    {
        const int keep = anchor;
        const int from = juce::jmin (keep, cell), to = juce::jmax (keep, cell);
        multi.clear();
        for (int i = from; i <= to; ++i)
            if (i != cell)
                multi.insert (i);
        setSelected (cell);
        anchor = keep;
        repaint();
        return;
    }

    // Ctrl (Cmd su Mac) + clic: aggiunge o toglie la cella dalla selezione (nessun suono)
    if (e.mods.isCommandDown())
    {
        if (isInSelection (cell))
        {
            if (cell == selected)
            {
                if (! multi.empty())                       // la cella corrente esce: ne diventa corrente un'altra della selezione
                {
                    const int next = *multi.rbegin();
                    multi.erase (next);
                    setSelected (next);
                }
            }
            else
                multi.erase (cell);
        }
        else
        {
            multi.insert (selected);                       // la cella corrente resta nella selezione; quella cliccata diventa corrente
            setSelected (cell);
        }
        anchor = cell;
        repaint();
        return;
    }

    // Clic semplice: selezione singola e anteprima
    multi.clear();
    anchor = cell;
    setSelected (cell);

    // Anteprima: suona la nota della cella
    const auto& c = processor.getKit().cells[(size_t) cell];
    if (c.hasSample() && ! c.missing)
    {
        heldNote = c.lowNote;
        processor.getKeyboardState().noteOn (1, heldNote, 0.8f);
    }
    repaint();
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
    const auto sel = selectedCells();
    if (sel.size() > 1)
    {
        int withSample = 0, inSeq = 0;
        for (const int c : sel)
        {
            withSample += processor.getKit().cells[(size_t) c].hasSample() ? 1 : 0;
            inSeq += processor.activePattern().findLane (c) != nullptr ? 1 : 0;
        }
        juce::PopupMenu mm;
        mm.addItem (1, "Add " + juce::String (withSample) + " selected to sequencer", withSample > 0);
        mm.addItem (2, "Remove " + juce::String (inSeq) + " selected from sequencer", inSeq > 0);
        mm.addSeparator();
        mm.addItem (3, "Clear " + juce::String (withSample) + " selected cell" + (withSample == 1 ? "" : "s"), withSample > 0);
        mm.addSeparator();
        mm.addItem (4, "Select only this cell");
        mm.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (cellBounds (cell).toNearestInt())),
                          [this, sel, cell] (int r)
                          {
                              if (r == 1)
                              {
                                  for (const int c : sel)
                                      if (processor.getKit().cells[(size_t) c].hasSample())
                                          processor.addLaneForCell (c);
                                  repaint();
                                  if (onKitChanged) onKitChanged();
                              }
                              else if (r == 2)
                              {
                                  for (const int c : sel)
                                      processor.removeLaneForCell (c);
                                  repaint();
                                  if (onKitChanged) onKitChanged();
                              }
                              else if (r == 3)
                                  clearSelectedCells();
                              else if (r == 4)
                              {
                                  multi.clear();
                                  setSelected (cell);
                                  repaint();
                              }
                          });
        return;
    }

    juce::PopupMenu m;
    const bool filled = processor.getKit().cells[(size_t) cell].hasSample();
    const bool inSequencer = processor.activePattern().findLane (cell) != nullptr;
    m.addItem (4, inSequencer ? "Remove from sequencer" : "Add to sequencer", filled || inSequencer);
    m.addSeparator();
    m.addItem (1, "Load sample...");
    m.addItem (2, "Rename...", filled);
    m.addItem (3, "Clear cell", filled);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (cellBounds (cell).toNearestInt())),
                     [this, cell, inSequencer] (int r)
                     {
                         if (r == 1 && onLoadRequested) onLoadRequested (cell);
                         else if (r == 2) renameCell (cell);
                         else if (r == 3)
                         {
                             processor.clearCell (cell);
                             if (onKitChanged) onKitChanged();
                         }
                         else if (r == 4)
                         {
                             if (inSequencer) processor.removeLaneForCell (cell);
                             else processor.addLaneForCell (cell);
                             repaint();
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
