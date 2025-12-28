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

#include "ADSRComponent.h"
#include "../ConfigFileManager.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "JsonGuiProvider.h"
#include "UIAssetManager.h"

//==============================================================================
ADSRComponent::ADSRComponent(AudioProcessorValueTreeState &vts, const std::string &p_adsr_number) :
    m_loop("loop_button", "", OdinButton::Type::loop),
    m_attack(OdinKnob::Type::knob_4x4a),
    m_decay(OdinKnob::Type::knob_4x4a),
    m_sustain(OdinKnob::Type::knob_4x4a),
    m_release(OdinKnob::Type::knob_4x4a),
    m_value_tree(vts),
    m_adsr_number(p_adsr_number),
    m_attack_label("A"),
    m_decay_label("D"),
    m_sustain_label("S"),
    m_release_label("R") {

    setOpaque(true);
	setWantsKeyboardFocus(false);

	addAndMakeVisible(m_attack_label);
	addAndMakeVisible(m_decay_label);
	addAndMakeVisible(m_sustain_label);
	addAndMakeVisible(m_release_label);

	m_attack_attach.reset(new OdinKnobAttachment(m_value_tree, ("env" + m_adsr_number + "_attack"), m_attack));
	m_decay_attach.reset(new OdinKnobAttachment(m_value_tree, "env" + m_adsr_number + "_decay", m_decay));
	m_sustain_attach.reset(new OdinKnobAttachment(m_value_tree, "env" + m_adsr_number + "_sustain", m_sustain));
	m_release_attach.reset(new OdinKnobAttachment(m_value_tree, "env" + m_adsr_number + "_release", m_release));

	m_loop.setClickingTogglesState(true);
	addAndMakeVisible(m_loop);
	m_loop.setAlwaysOnTop(true);
	m_loop.setTriggeredOnMouseDown(true);
	m_loop.setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colour());
	m_loop.setTooltip("Loops the envelopes attack\n and decay sections");

	addAndMakeVisible(m_attack);
	addAndMakeVisible(m_decay);
	addAndMakeVisible(m_sustain);
	addAndMakeVisible(m_release);

	m_attack.setRange(A_LOW_LIMIT, A_HIGH_LIMIT);
	m_attack.setSkewFactorFromMidPoint(A_MID_VALUE);
	m_attack.setTooltip("Attack\nDefines how long the envelope\ntakes to reach the top peak");
	m_attack.setTextValueSuffix(" s");

	m_decay.setRange(D_LOW_LIMIT, D_HIGH_LIMIT);
	m_decay.setSkewFactorFromMidPoint(D_MID_VALUE);
	m_decay.setTextValueSuffix(" s");
	m_decay.setTooltip("Decay\nDefines how long the\n envelope takes to fall "
	                   "from the top\n peak to the sustain level");

	m_sustain.setRange(S_LOW_LIMIT, S_HIGH_LIMIT);
	m_sustain.setSkewFactorFromMidPoint(S_MID_VALUE);
	m_sustain.setNumDecimalPlacesToDisplay(3);
	m_sustain.setTooltip("Sustain\nDefines the height of the evelope\nafter the "
	                     "decay section is finished");

	m_release.setRange(R_LOW_LIMIT, R_HIGH_LIMIT);
	m_release.setSkewFactorFromMidPoint(R_MID_VALUE);
	m_release.setTextValueSuffix(" s");
	m_release.setTooltip("Release\nDefines how long the envelope takes\n to fall "
	                     "back to zero after\nthe key is released");

	m_loop_attach.reset(new OdinButtonAttachment(m_value_tree, "env" + m_adsr_number + "_loop", m_loop));

	m_attack.setNumDecimalPlacesToDisplay(3);
	m_decay.setNumDecimalPlacesToDisplay(3);
	m_sustain.setNumDecimalPlacesToDisplay(3);
	m_release.setNumDecimalPlacesToDisplay(3);

	SET_CTR_KEY(m_attack);
	SET_CTR_KEY(m_decay);
	SET_CTR_KEY(m_sustain);
	SET_CTR_KEY(m_release);

	m_attack.onValueChange = [this] { repaint(); };
	m_decay.onValueChange = [this] { repaint(); };
	m_sustain.onValueChange = [this] { repaint(); };
	m_release.onValueChange = [this] { repaint(); };
}

ADSRComponent::~ADSRComponent() {
}

ADSRComponent::GraphGeometry ADSRComponent::getGraphGeometry() {
    auto bounds = getLocalBounds().toFloat();
    float h = bounds.getHeight();
    float w = bounds.getWidth();
    float graphHeight = h * 0.7f;

    juce::Rectangle<float> area(0, 0, w, graphHeight);

    // Read normalized slider values (0.0 to 1.0)
    double aNorm = m_attack.valueToProportionOfLength(m_attack.getValue());
    double dNorm = m_decay.valueToProportionOfLength(m_decay.getValue());
    double sNorm = m_sustain.valueToProportionOfLength(m_sustain.getValue());
    double rNorm = m_release.valueToProportionOfLength(m_release.getValue());

    float sectionWidth = w / 4.0f;

    float xStart = 0.0f;
    float yBase = graphHeight;
    float yPeak = 0.0f;

    float xAttack = xStart + (float)(aNorm * sectionWidth);

    float xDecay = xAttack + (float)(dNorm * sectionWidth);
    float ySustain = graphHeight - (float)(sNorm * graphHeight);

    float xSustain = xDecay + sectionWidth;

    float xRelease = xSustain + (float)(rNorm * sectionWidth);

    return { area, xStart, xAttack, xDecay, xSustain, xRelease, yBase, yPeak, ySustain };
}

void ADSRComponent::paint(juce::Graphics& g) {
	g.fillAll(juce::Colour::fromString("#202020"));
    g.reduceClipRegion(getLocalBounds());
    auto geo = getGraphGeometry();

	juce::Path p;
	p.startNewSubPath(geo.xStart, geo.yBase);

	// Attack Control: (xStart, yAttack) -> (0, 0).
	p.quadraticTo(geo.xStart, geo.yPeak, geo.xAttack, geo.yPeak);

	// Decay Control: (xAttack, ySustain).
	p.quadraticTo(geo.xAttack, geo.ySustain, geo.xDecay, geo.ySustain);

	// Sustain Plateau
	p.lineTo(geo.xSustain, geo.ySustain);

	// Release Control: (xSustain, yEnd) -> (xSustain, H).
	p.quadraticTo(geo.xSustain, geo.yBase, geo.xRelease, geo.yBase);

	// Close Path
	p.lineTo(geo.xRelease, geo.yBase);
	p.lineTo(geo.xStart, geo.yBase);
	p.closeSubPath();

    // Constrain blue fill and curve drawing strictly to the top 70% graph area
    // Actually the path is already constructed within graphHeight (geo.yBase is graphHeight)
    // But we can add a clip just in case, or to be explicit.
    // The instructions say "Constrain ... to the top 70% graph area".

    g.saveState();
    g.reduceClipRegion(geo.area.toNearestInt());

	// Draw Fill
	g.setColour(juce::Colour::fromString("#3d80b0").withAlpha(0.5f));
	g.fillPath(p);

	// Draw Stroke
	g.setColour(juce::Colour::fromString("#8ecae6"));
	g.strokePath(p, juce::PathStrokeType(2.0f));

	// Draw Control Handles (small white circles)

    // 1. Attack Peak
    float r = 3.0f; // Radius
	float d = 6.0f; // Diameter

    g.setColour(juce::Colours::white);

    // Attack Peak Node
    g.fillEllipse(geo.xAttack - r, geo.yPeak - r, d, d);

    // Decay End / Sustain Start Node
    g.fillEllipse(geo.xDecay - r, geo.ySustain - r, d, d);

    // Release End Node
    g.fillEllipse(geo.xRelease - r, geo.yBase - r, d, d);

    g.restoreState();

    if (m_draggedHandle != -1) {
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);

        juce::String text;
        float x = 0, y = 0;

        if (m_draggedHandle == 0) { // Attack
            text = m_attack.getTextFromValue(m_attack.getValue());
            x = geo.xAttack;
            y = geo.yPeak + 20;
        } else if (m_draggedHandle == 1) { // Decay
            text = m_decay.getTextFromValue(m_decay.getValue());
            x = geo.xDecay;
            y = (geo.ySustain < 30) ? geo.ySustain + 20 : geo.ySustain - 20;
        } else if (m_draggedHandle == 2) { // Sustain
            text = m_sustain.getTextFromValue(m_sustain.getValue());
            x = geo.xDecay;
            y = (geo.ySustain < 30) ? geo.ySustain + 20 : geo.ySustain - 20;
        } else if (m_draggedHandle == 3) { // Release
            text = m_release.getTextFromValue(m_release.getValue());
            x = geo.xRelease;
            y = geo.yBase - 20;
        }

        g.drawText(text, x - 30, y - 10, 60, 20, juce::Justification::centred);
    }
}

void ADSRComponent::resized() {
    auto bounds = getLocalBounds();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    float graphHeight = h * 0.7f;
    float controlsHeight = h * 0.3f;
    float controlsY = graphHeight;

    // Position knobs
    float knobWidth = w / 4.0f;
    float knobSize = std::min(knobWidth, controlsHeight) * 0.8f;

    // Center knobs within their sections
    int kY = controlsY + (controlsHeight - knobSize) / 2 + 5; // Add some offset for label space?

    // Using 4x4 knobs, they are usually small.
    // Let's assume we want them centered vertically in the bottom area.
    // And labels above or below.
    // Let's put labels above the knobs.

    int labelHeight = 15;
    int kX_offset = (knobWidth - knobSize) / 2;

    // Attack
    m_attack_label.setBounds(0, controlsY, knobWidth, labelHeight);
    m_attack.setBounds(kX_offset, controlsY + labelHeight, knobSize, knobSize);

    // Decay
    m_decay_label.setBounds(knobWidth, controlsY, knobWidth, labelHeight);
    m_decay.setBounds(knobWidth + kX_offset, controlsY + labelHeight, knobSize, knobSize);

    // Sustain
    m_sustain_label.setBounds(knobWidth * 2, controlsY, knobWidth, labelHeight);
    m_sustain.setBounds(knobWidth * 2 + kX_offset, controlsY + labelHeight, knobSize, knobSize);

    // Release
    m_release_label.setBounds(knobWidth * 3, controlsY, knobWidth, labelHeight);
    m_release.setBounds(knobWidth * 3 + kX_offset, controlsY + labelHeight, knobSize, knobSize);

    // Loop button - let's put it in the top right corner of the graph area or somewhere unobtrusive
    // Or maybe overlay it on the graph.
    // Previous implementation: aligned with "D".
    // Let's put it top-right of graph for now, or maybe top-left.
    m_loop.setBounds(w - 20, 0, 20, 20); // Small button
}

void ADSRComponent::mouseDown(const juce::MouseEvent& e) {
    auto geo = getGraphGeometry();
    float mx = e.position.x;
    float my = e.position.y;

    float r = 5.0f; // Hit test radius slightly larger

    if (e.position.getDistanceFrom({geo.xAttack, geo.yPeak}) < r + 5.0f) {
        m_dragged_handle = AttackPeak;
    } else if (e.position.getDistanceFrom({geo.xDecay, geo.ySustain}) < r + 5.0f) {
        m_dragged_handle = DecayEnd;
    } else if (e.position.getDistanceFrom({geo.xRelease, geo.yBase}) < r + 5.0f) {
        m_dragged_handle = ReleaseEnd;
    } else {
        m_dragged_handle = None;
    }

    if (m_dragged_handle == AttackPeak) {
        m_draggedHandle = 0;
    } else if (m_dragged_handle == DecayEnd) {
        m_draggedHandle = 2; // Default to Sustain
    } else if (m_dragged_handle == ReleaseEnd) {
        m_draggedHandle = 3;
    } else {
        m_draggedHandle = -1;
    }
    repaint();
}

void ADSRComponent::mouseDrag(const juce::MouseEvent& e) {
    if (m_dragged_handle == None) return;

    if (m_dragged_handle == DecayEnd) {
        juce::Point<float> startPos = e.getMouseDownPosition().toFloat();
        if (std::abs(e.position.x - startPos.x) > std::abs(e.position.y - startPos.y)) {
             m_draggedHandle = 1; // Decay
        } else {
             m_draggedHandle = 2; // Sustain
        }
    }

    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float sectionWidth = w / 4.0f;
    float graphHeight = bounds.getHeight() * 0.7f;

    if (m_dragged_handle == AttackPeak) {
        // Dragging Attack Peak (X-axis): Updates m_attack value.
        // xAttack = xStart + (aNorm * sectionWidth)
        // aNorm = (xAttack - xStart) / sectionWidth
        float xStart = 0.0f;
        float aNorm = (e.position.x - xStart) / sectionWidth;
        aNorm = std::max(0.0f, std::min(1.0f, aNorm));

        // Convert norm to value
        // valueToProportionOfLength is 0..1 -> value
        // But OdinKnob wraps Slider, so we can use standard Slider methods if range is linear?
        // Wait, Slider::valueToProportionOfLength is value -> proportion.
        // Slider::proportionOfLengthToValue is proportion -> value.
        double newVal = m_attack.proportionOfLengthToValue(aNorm);
        m_attack.setValue(newVal, juce::sendNotificationSync);

    } else if (m_dragged_handle == DecayEnd) {
        // Dragging Sustain Start (X-axis): Updates m_decay value.
        // Dragging Sustain Start (Y-axis): Updates m_sustain value.

        // X-axis
        // xDecay = xAttack + (dNorm * sectionWidth)
        // dNorm = (xDecay - xAttack) / sectionWidth
        // We need current xAttack
        double aNorm = m_attack.valueToProportionOfLength(m_attack.getValue());
        float xAttack = 0.0f + (float)(aNorm * sectionWidth);

        float dNorm = (e.position.x - xAttack) / sectionWidth;
        dNorm = std::max(0.0f, std::min(1.0f, dNorm));
        double newDecay = m_decay.proportionOfLengthToValue(dNorm);
        m_decay.setValue(newDecay, juce::sendNotificationSync);

        // Y-axis
        // ySustain = graphHeight - (sNorm * graphHeight)
        // sNorm = (graphHeight - ySustain) / graphHeight
        float sNorm = (graphHeight - e.position.y) / graphHeight;
        sNorm = std::max(0.0f, std::min(1.0f, sNorm));
        double newSustain = m_sustain.proportionOfLengthToValue(sNorm);
        m_sustain.setValue(newSustain, juce::sendNotificationSync);

    } else if (m_dragged_handle == ReleaseEnd) {
        // Dragging Release End (X-axis): Updates m_release value.
        // xRelease = xSustain + (rNorm * sectionWidth)
        // We need current xSustain
        // xSustain = xDecay + sectionWidth
        double aNorm = m_attack.valueToProportionOfLength(m_attack.getValue());
        double dNorm = m_decay.valueToProportionOfLength(m_decay.getValue());
        float xAttack = (float)(aNorm * sectionWidth);
        float xDecay = xAttack + (float)(dNorm * sectionWidth);
        float xSustain = xDecay + sectionWidth;

        float rNorm = (e.position.x - xSustain) / sectionWidth;
        rNorm = std::max(0.0f, std::min(1.0f, rNorm));
        double newRelease = m_release.proportionOfLengthToValue(rNorm);
        m_release.setValue(newRelease, juce::sendNotificationSync);
    }
    repaint();
}

void ADSRComponent::mouseUp(const juce::MouseEvent& e) {
    m_dragged_handle = None;
    m_draggedHandle = -1;
    repaint();
}

void ADSRComponent::mouseExit(const juce::MouseEvent& e) {
    m_draggedHandle = -1;
    repaint();
}
