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
    m_value_tree(vts),
    m_adsr_number(p_adsr_number),
    m_attack_label("A"),
    m_decay_label("D"),
    m_sustain_label("S"),
    m_release_label("R") {

	addAndMakeVisible(m_attack_label);
	addAndMakeVisible(m_decay_label);
	addAndMakeVisible(m_sustain_label);
	addAndMakeVisible(m_release_label);

	m_attack_attach.reset(new OdinSliderAttachment(m_value_tree, ("env" + m_adsr_number + "_attack"), m_attack));
	m_decay_attach.reset(new OdinSliderAttachment(m_value_tree, "env" + m_adsr_number + "_decay", m_decay));
	m_sustain_attach.reset(new OdinSliderAttachment(m_value_tree, "env" + m_adsr_number + "_sustain", m_sustain));
	m_release_attach.reset(new OdinSliderAttachment(m_value_tree, "env" + m_adsr_number + "_release", m_release));

	m_loop.setClickingTogglesState(true);
	addAndMakeVisible(m_loop);
	m_loop.setAlwaysOnTop(true);
	m_loop.setTriggeredOnMouseDown(true);
	m_loop.setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colour());
	m_loop.setTooltip("Loops the envelopes attack\n and decay sections");

	m_attack.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
	addAndMakeVisible(m_attack);
	m_decay.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
	addAndMakeVisible(m_decay);
	m_sustain.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
	addAndMakeVisible(m_sustain);
	m_release.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
	addAndMakeVisible(m_release);

	m_attack.setRange(A_LOW_LIMIT, A_HIGH_LIMIT);
	m_attack.setTooltip("Attack\nDefines how long the envelope\ntakes to reach the top peak");
	m_attack.setTextValueSuffix(" s");

	m_decay.setTextValueSuffix(" s");
	m_decay.setTooltip("Decay\nDefines how long the\n envelope takes to fall "
	                   "from the top\n peak to the sustain level");

	m_sustain.setNumDecimalPlacesToDisplay(3);
	m_sustain.setTooltip("Sustain\nDefines the height of the evelope\nafter the "
	                     "decay section is finished");

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

void ADSRComponent::paint(juce::Graphics& g) {
	// 3. Visual Style: Fill: #3d80b0 (approx 50% opacity), Stroke: #8ecae6 (2px thickness)
	// Z-Order: The graph draws on the component background, so it is naturally behind the sliders.

	auto bounds = getLocalBounds().toFloat();
	float w = bounds.getWidth();
	float h = bounds.getHeight();

	// 2. Draw the Graph
	// Read normalized slider values (0.0 to 1.0)
	double aNorm = m_attack.valueToProportionOfLength(m_attack.getValue());
	double dNorm = m_decay.valueToProportionOfLength(m_decay.getValue());
	double sNorm = m_sustain.valueToProportionOfLength(m_sustain.getValue());
	double rNorm = m_release.valueToProportionOfLength(m_release.getValue());

	// Map to component width/height
	// We'll divide the width into 4 logical sections for A, D, Sustain, R.
	// Attack, Decay, Release each take up to 1/4 of the width based on their value.
	// Sustain takes a fixed 1/4 width (or whatever is left if we were dynamic, but let's be consistent).
	float sectionWidth = w / 4.0f;

	float xStart = 0.0f;
	float yBase = h; // Bottom (Amplitude 0)
	float yPeak = 0.0f; // Top (Amplitude 1)

	// Attack Phase
	// Width = aNorm * sectionWidth
	float xAttack = xStart + (float)(aNorm * sectionWidth);
	float yAttack = yPeak;

	// Decay Phase
	// Width = dNorm * sectionWidth
	// End Level = sNorm (Amplitude) -> y = h - (sNorm * h)
	float xDecay = xAttack + (float)(dNorm * sectionWidth);
	float ySustain = h - (float)(sNorm * h);

	// Sustain Phase
	// Fixed width of sectionWidth? Or just a small plateau?
	// User said "Calculate the nodes...". Usually visualization shows sustain as a plateau.
	// Let's make it fixed width for visibility.
	float xSustain = xDecay + sectionWidth;

	// Release Phase
	// Width = rNorm * sectionWidth
	// End Level = 0
	float xRelease = xSustain + (float)(rNorm * sectionWidth);
	float yEnd = yBase;

	juce::Path p;
	p.startNewSubPath(xStart, yBase);

	// Attack Curve: Convex
	// Convex usually means "bulging up/out".
	// Start (0, H) -> End (xA, 0).
	// Control Point at (0, 0) (Top-Left corner of the box defined by start/end) makes it bulge towards top-left.
	// This results in a curve that rises fast and then flattens out (Logarithmic attack / "Charging").
	// Wait, if it flattens out at the top, that's concave down?
	// Let's verify "Convex" vs "Concave".
	// "Convex" in envelope terms usually refers to the shape `y = x^2` (slow start, fast end) vs `y = log(x)` (fast start, slow end).
	// `quadraticTo` with control point at corner:
	// If control is at (0, 0) for line (0,H)->(W,0):
	// Tangent at start is vertical up. Tangent at end is horizontal.
	// Shape is "Fast Rise -> Slow approach". This is often called "Logarithmic" or "Exponential".
	// The prompt asks for "Convex".
	// If the user means "Exponential Attack" (Fast start), then (0,0) is correct.
	// If the user means "Slow start, fast rise", then control point would be (xA, H).
	// Standard ADSR "Attack" is usually linear or exponential (fast start).
	// Let's assume the user wants the standard "Analog" look which is fast-start.
	// But let's check: "Use `quadraticTo` (not `lineTo`) to make the Attack convex and the Decay/Release concave."
	// Convex usually means the set of points below the graph is a convex set. (Bulging up).
	// Concave usually means the set of points above the graph is a convex set. (Bulging down).
	// So Attack Convex = Bulging Up (Fast start).
	// Decay Concave = Bulging Down (Sagging).
	// Release Concave = Bulging Down (Sagging).

	// So:
	// Attack Control: (xStart, yAttack) -> (0, 0).
	p.quadraticTo(xStart, yAttack, xAttack, yAttack);

	// Decay Control: (xAttack, ySustain).
	// Start (xAttack, 0). End (xDecay, ySustain).
	// Control at (xAttack, ySustain) pulls it towards the corner.
	// Tangent at start is vertical down. Tangent at end is horizontal.
	// This is "Fast Drop -> Slow approach". Correct for Concave/Exponential Decay.
	p.quadraticTo(xAttack, ySustain, xDecay, ySustain);

	// Sustain Plateau
	p.lineTo(xSustain, ySustain);

	// Release Control: (xSustain, yEnd) -> (xSustain, H).
	// Start (xSustain, ySustain). End (xRelease, H).
	// Tangent start vertical down. Tangent end horizontal.
	// Fast drop -> Slow approach. Correct.
	p.quadraticTo(xSustain, yEnd, xRelease, yEnd);

	// Close Path
	p.lineTo(xRelease, h);
	p.lineTo(xStart, h);
	p.closeSubPath();

	// Draw Fill
	g.setColour(juce::Colour::fromString("#3d80b0").withAlpha(0.5f));
	g.fillPath(p);

	// Draw Stroke
	g.setColour(juce::Colour::fromString("#8ecae6"));
	g.strokePath(p, juce::PathStrokeType(2.0f));
}

void ADSRComponent::resized() {
	GET_LOCAL_AREA(m_attack_label, "AttackLabel");
	GET_LOCAL_AREA(m_decay_label, "DecayLabel");
	GET_LOCAL_AREA(m_sustain_label, "SustainLabel");
	GET_LOCAL_AREA(m_release_label, "ReleaseLabel");

	GET_LOCAL_AREA(m_attack, "ADSRAttack");
	GET_LOCAL_AREA(m_decay, "ADSRDecay");
	GET_LOCAL_AREA(m_sustain, "ADSRSustain");
	GET_LOCAL_AREA(m_release, "ADSRRelease");

	// this one was position not conforming to the grid, so we need to move it half a cell to center align it with "D"
	GET_LOCAL_AREA(m_loop, "ADSRLoop");
	const auto grid = ConfigFileManager::getInstance().getOptionGuiScale();
	m_loop.setBounds(m_loop.getBounds().translated(grid / 2, 0));
}