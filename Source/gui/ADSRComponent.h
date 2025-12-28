/*
** Odin 2 Synthesizer Plugin
** Copyright (C) 2020 - 2021 TheWaveWarden
**
** Odin 2 is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Odin 2 is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "OdinKnob.h"
#include "OdinButton.h"
#include "OdinControlAttachments.h"
#include "TextLabel.h"

#define A_LOW_LIMIT 0.001
#define A_HIGH_LIMIT 10
#define A_DEFAULT 0.03
#define A_MID_VALUE 1

#define D_LOW_LIMIT A_LOW_LIMIT
#define D_HIGH_LIMIT A_HIGH_LIMIT
#define D_DEFAULT 1
#define D_MID_VALUE A_MID_VALUE

#define S_LOW_LIMIT 0
#define S_HIGH_LIMIT 1
#define S_DEFAULT 0.5
#define S_MID_VALUE 0.3

#define R_LOW_LIMIT A_LOW_LIMIT
#define R_HIGH_LIMIT 5.0
#define R_DEFAULT 0.03
#define R_MID_VALUE A_MID_VALUE
//==============================================================================
/*
 */
class ADSRComponent : public Component {
public:
	ADSRComponent(AudioProcessorValueTreeState &vts, const std::string &p_adsr_number);
	~ADSRComponent();

	void paint(juce::Graphics& g) override;
	void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    int m_draggedHandle = -1;
    TextLabel m_attack_label;
    TextLabel m_decay_label;
    TextLabel m_sustain_label;
    TextLabel m_release_label;

	OdinButton m_loop;
	OdinKnob m_attack;
	OdinKnob m_decay;
	OdinKnob m_sustain;
	OdinKnob m_release;

	std::string m_adsr_number;
	AudioProcessorValueTreeState &m_value_tree;

	std::unique_ptr<OdinKnobAttachment> m_attack_attach;
	std::unique_ptr<OdinKnobAttachment> m_decay_attach;
	std::unique_ptr<OdinKnobAttachment> m_sustain_attach;
	std::unique_ptr<OdinKnobAttachment> m_release_attach;

	std::unique_ptr<OdinButtonAttachment> m_loop_attach;

    // Interaction state
    enum DragHandle {
        None = -1,
        AttackPeak = 0,
        DecayEnd = 1,
        ReleaseEnd = 2
    };
    DragHandle m_dragged_handle = None;

    // Helper methods for geometry
    struct GraphGeometry {
        juce::Rectangle<float> area;
        float xStart;
        float xAttack;
        float xDecay;
        float xSustain; // Start of release
        float xRelease;
        float yBase;
        float yPeak;
        float ySustain;
    };
    GraphGeometry getGraphGeometry();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRComponent)
};
