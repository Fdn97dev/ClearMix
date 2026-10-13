// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#include "SpectrumAnalyzer.h"
#include <algorithm>

namespace cm
{
SpectrumAnalyzer::SpectrumAnalyzer() { smoothed.fill (kFloorDb); }

void SpectrumAnalyzer::prepare (double sampleRate)
{
    const auto& layout = BandLayout::get();
    const double binWidth = sampleRate / kFftSize;
    const int maxBin = kFftSize / 2 - 1;

    for (int b = 0; b < kNumBands; ++b)
    {
        // Chaque bande contient au moins 1 bin (important dans le grave où les bandes sont étroites).
        int lo = (int) std::lround (layout.edge[(size_t) b] / binWidth);
        int hi = (int) std::lround (layout.edge[(size_t) b + 1] / binWidth) - 1;
        lo = std::clamp (lo, 1, maxBin);
        hi = std::clamp (std::max (hi, lo), lo, maxBin);
        binLo[(size_t) b] = lo;
        binHi[(size_t) b] = hi;
    }

    // Lissage en dB entre deux analyses : attaque rapide (~40 ms), relâchement lent (~300 ms).
    const double hopSeconds = kFftHop / sampleRate;
    attackCoeff  = (float) (1.0 - std::exp (-hopSeconds / 0.040));
    releaseCoeff = (float) (1.0 - std::exp (-hopSeconds / 0.300));
    reset();
}

void SpectrumAnalyzer::reset()
{
    ring.fill (0.0f);
    smoothed.fill (kFloorDb);
    writePos = hopCounter = 0;
}

void SpectrumAnalyzer::process (const float* left, const float* right, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        ring[(size_t) writePos] = 0.5f * (left[i] + right[i]);
        writePos = (writePos + 1) & (kFftSize - 1);

        if (++hopCounter >= kFftHop)
        {
            hopCounter = 0;
            analyseFrame();
        }
    }
}

void SpectrumAnalyzer::analyseFrame() noexcept
{
    // Recopie le ring buffer dans l'ordre chronologique.
    for (int i = 0; i < kFftSize; ++i)
        fftData[(size_t) i] = ring[(size_t) ((writePos + i) & (kFftSize - 1))];
    std::fill (fftData.begin() + kFftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) kFftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());   // magnitudes dans fftData[0 .. N/2]

    // Normalisation : amplitude = mag * 4/N (Hann) ; puissance sinus = A²/2 ; l'étalement
    // d'une sinusoïde sur 3 bins (Hann) multiplie la somme par 1.5 -> on le divise.
    constexpr float norm = (4.0f / kFftSize) * (4.0f / kFftSize) / (2.0f * 1.5f);

    for (int b = 0; b < kNumBands; ++b)
    {
        float power = 0.0f;
        for (int k = binLo[(size_t) b]; k <= binHi[(size_t) b]; ++k)
            power += fftData[(size_t) k] * fftData[(size_t) k];

        const float db = std::max (kFloorDb, 10.0f * std::log10 (power * norm + 1.0e-12f));
        float& s = smoothed[(size_t) b];
        s += (db > s ? attackCoeff : releaseCoeff) * (db - s);
    }
}
}
