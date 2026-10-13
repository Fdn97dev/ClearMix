// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Biquad.h"
#include "SharedBus.h"
#include "SpectrumAnalyzer.h"

class ClearMixAudioProcessor : public juce::AudioProcessor
{
public:
    ClearMixAudioProcessor();
    ~ClearMixAudioProcessor() override;

    // --- Cycle de vie audio ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    // --- Éditeur ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // --- Infos plugin ---
    const juce::String getName() const override { return "ClearMix"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // --- Sauvegarde du projet ---
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- Accès pour l'interface (lecture seule, via atomiques) ---
    juce::AudioProcessorValueTreeState apvts;
    int  getSlotIndex() const noexcept { return slotIndex; }
    double getSampleRateForUi() const noexcept { return currentSampleRate.load(); }
    const std::array<std::atomic<float>, cm::kNumBands>& getAutoCutDisplay() const noexcept { return autoCutDisplay; }
    const std::array<std::atomic<float>, cm::kNumBands>& getOwnLevelDisplay() const noexcept { return ownLevelDisplay; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Coefficients de l'EQ manuel d'après les paramètres actuels : [0]=HPF, [1]=shelf grave,
    // [2..4]=cloches, [5]=shelf aigu. Utilisé par le DSP ET par l'interface (pour dessiner la courbe).
    void computeManualCoeffs (double fs, std::array<cm::BiquadCoeffs, cm::kNumManualBands + 1>& out) const;

private:
    void updateAutoFilters (double fs);

    // Pointeurs bruts vers les paramètres (lecture atomique dans le thread audio).
    std::atomic<float> *pRole, *pPriority, *pGroup, *pLink, *pAutoOn, *pAmount, *pMaxCut,
                       *pAttack, *pRelease, *pOutGain, *pHpf;
    std::array<std::atomic<float>*, cm::kNumManualBands> pFreq {}, pGain {}, pQ {};

    // DSP
    cm::SpectrumAnalyzer analyzer;
    cm::BiquadCascade<cm::kNumBands>       autoBank;     // EQ automatique (24 cloches)
    cm::BiquadCascade<cm::kNumManualBands + 1> manualEq; // HPF + 5 bandes manuelles
    juce::SmoothedValue<float> outGainSmoothed;

    cm::BandArray currentCutDb {};   // coupe actuellement appliquée (lissée)
    std::array<std::atomic<float>, cm::kNumBands> autoCutDisplay, ownLevelDisplay;
    std::array<cm::TrackSnapshot, cm::kMaxSlots> othersBuffer {};   // pré-alloué : pas de new dans processBlock

    std::atomic<double> currentSampleRate { 44100.0 };
    int slotIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClearMixAudioProcessor)
};
