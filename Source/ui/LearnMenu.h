#pragma once

#include "../PluginProcessor.h"

// Menu del tasto destro su un controllo: MIDI Learn / Clear MIDI mapping, con l'assegnazione corrente.
// Si collega a un componente (anche con figli, come lo slider di una manopola) e a una funzione che
// restituisce il parametro attuale (cambia con la cella selezionata).
class LearnMenu final : public juce::MouseListener
{
public:
    using ParamGetter = std::function<juce::RangedAudioParameter*()>;

    LearnMenu (SampleLabProcessor& p, juce::Component& comp, ParamGetter g)
        : processor (p), target (&comp), getParam (std::move (g))
    {
        comp.addMouseListener (this, true);
    }

    ~LearnMenu() override
    {
        if (target != nullptr)
            target->removeMouseListener (this);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu() || target == nullptr)
            return;

        auto* param = getParam();
        if (param == nullptr)
            return;

        auto& learn = processor.getMidiLearn();
        const int cc = learn.ccFor (param);
        const bool learning = learn.isLearning (param);

        juce::PopupMenu m;
        m.addSectionHeader (param->getName (40));
        m.addItem (-1, cc >= 0 ? "Assigned to CC " + juce::String (cc) : juce::String ("No MIDI assignment"), false);
        m.addSeparator();
        m.addItem (1, learning ? "Cancel MIDI learn (move a controller...)" : "MIDI Learn");
        m.addItem (2, "Clear MIDI mapping", cc >= 0);

        juce::Component::SafePointer<juce::Component> safe (target);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                         [this, param, learning, safe] (int r)
                         {
                             if (safe == nullptr)
                                 return;
                             auto& l = processor.getMidiLearn();
                             if (r == 1)
                             {
                                 if (learning) l.cancelLearn();
                                 else l.startLearn (param);        // si assegna il prossimo CC ricevuto
                             }
                             else if (r == 2)
                             {
                                 l.clear (param);
                                 processor.markDirty();
                             }
                         });
    }

private:
    SampleLabProcessor& processor;
    juce::Component::SafePointer<juce::Component> target;
    ParamGetter getParam;
};
