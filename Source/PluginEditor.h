// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ----------------------------------------------------------------------------
//  Look & feel sombre, rotatifs minimalistes.
// ----------------------------------------------------------------------------
class ClearMixLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClearMixLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

// ----------------------------------------------------------------------------
//  Grand affichage central : spectres, courbe auto (rouge), courbe manuelle,
//  poignées déplaçables (glisser = freq/gain, molette = Q, double-clic = reset).
// ----------------------------------------------------------------------------
class EqDisplay : public juce::Component, private juce::Timer
{
public:
    explicit EqDisplay (ClearMixAudioProcessor&);
    ~EqDisplay() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override { repaint(); }

    juce::Rectangle<float> plotArea() const;
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float dbToY (float db) const;
    float yToDb (float y) const;
    float levelToY (float db) const;
    juce::Point<float> handlePos (int i) const;
    int  findHandle (juce::Point<float>) const;

    void setParam (const juce::String& paramId, float plainValue);
    void beginGesture (int handle);
    void endGesture (int handle);

    juce::Path buildSpectrumPath (const float* levels, bool closeBottom) const;

    ClearMixAudioProcessor& proc;
    int dragging = -1, hover = -1;   // 0..4 = bandes manuelles, 5 = HPF

    static constexpr float kCurveRangeDb = 18.0f;
};

// ----------------------------------------------------------------------------
class ClearMixAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ClearMixAudioProcessorEditor (ClearMixAudioProcessor&);
    ~ClearMixAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;   // détruit AVANT le slider
    };

    void setupKnob (Knob&, const juce::String& text, const juce::String& paramId);

    ClearMixAudioProcessor& proc;
    ClearMixLookAndFeel lnf;   // doit survivre aux composants qui l'utilisent

    juce::Label title, status;
    juce::ComboBox roleBox, groupBox;
    juce::ToggleButton linkButton { "Link" }, autoButton { "Auto EQ" };
    Knob kPriority, kAmount, kMaxCut, kAttack, kRelease, kOutput;
    EqDisplay display;

    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BtAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CbAtt> roleAtt, groupAtt;
    std::unique_ptr<BtAtt> linkAtt, autoAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClearMixAudioProcessorEditor)
};
