// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  SpectrumAnalyzer : mesure l'énergie du signal dans 24 bandes (FFT 4096,
//  fenêtre de Hann, une mesure toutes les 1024 samples).
//  Le résultat est en dB ~ "dBFS RMS dans la bande".
// ============================================================================
#include <juce_dsp/juce_dsp.h>
#include "Constants.h"

namespace cm
{
    class SpectrumAnalyzer
    {
    public:
        SpectrumAnalyzer();

        void prepare (double sampleRate);
        void reset();

        // Reçoit le signal gauche/droite (r peut être égal à l) ; mixe en mono.
        void process (const float* left, const float* right, int numSamples) noexcept;

        const std::array<float, kNumBands>& levels() const noexcept { return smoothed; }

    private:
        void analyseFrame() noexcept;

        juce::dsp::FFT fft { kFftOrder };
        juce::dsp::WindowingFunction<float> window { (size_t) kFftSize, juce::dsp::WindowingFunction<float>::hann };

        std::array<float, kFftSize>     ring {};
        std::array<float, kFftSize * 2> fftData {};
        int writePos = 0, hopCounter = 0;

        std::array<int, kNumBands> binLo {}, binHi {};
        std::array<float, kNumBands> smoothed {};
        float attackCoeff = 0.5f, releaseCoeff = 0.1f;
    };
}
