#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

// Griglia di celle (righe A.., colonne 1..). Selezione, anteprima al clic, drag & drop di file,
// menu contestuale con il tasto destro.
class PadGrid final : public juce::Component,
                      public juce::FileDragAndDropTarget,
                      public juce::DragAndDropTarget,
                      private juce::Timer
{
public:
    explicit PadGrid (SampleLabProcessor&);
    ~PadGrid() override;

    int getSelected() const { return selected; }
    void setSelected (int cell);

    std::function<void (int)> onSelectionChanged;
    std::function<void (int, const juce::File&)> onFileDropped;   // file da Esplora risorse/Finder
    std::function<void (int)> onBrowserDrop;                      // trascinamento dal browser interno
    std::function<void (int)> onLoadRequested;                    // "Load sample..."
    std::function<void()> onKitChanged;                           // rinomina/svuota ecc.

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragMove (const juce::StringArray&, int x, int y) override { setHover (cellAt (x, y)); }
    void fileDragExit (const juce::StringArray&) override { setHover (-1); }

    bool isInterestedInDragSource (const SourceDetails& d) override { return d.description == "SLFILE"; }
    void itemDragMove (const SourceDetails& d) override { setHover (cellAt (d.localPosition.x, d.localPosition.y)); }
    void itemDragExit (const SourceDetails&) override { setHover (-1); }
    void itemDropped (const SourceDetails& d) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> cellBounds (int index) const;
    int cellAt (int x, int y) const;
    void setHover (int cell);
    void showContextMenu (int cell);
    void renameCell (int cell);

    SampleLabProcessor& processor;
    int selected = 0;
    int hover = -1;
    int heldNote = -1;
    std::vector<int> lastActivity;
};
