// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  Biquad.h : filtres du 2e ordre (formules du "Audio EQ Cookbook" de Robert
//  Bristow-Johnson). Phase minimale -> LATENCE NULLE.
//  Écrit à la main (et pas avec juce::dsp::IIR) pour ne JAMAIS allouer de
//  mémoire dans le thread audio quand on change les coefficients.
// ============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>

namespace cm
{
    struct BiquadCoeffs
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;   // normalisés (a0 = 1)
    };

    namespace biquad
    {
        constexpr double kPi = 3.14159265358979323846;

        inline double clampFreq (double f, double fs) { return std::fmin (std::fmax (f, 10.0), fs * 0.49); }

        inline BiquadCoeffs normalise (double b0, double b1, double b2, double a0, double a1, double a2)
        {
            return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
        }

        inline BiquadCoeffs peaking (double fs, double f, double gainDb, double q)
        {
            if (std::fabs (gainDb) < 0.005) return {};            // filtre transparent
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * kPi * clampFreq (f, fs) / fs;
            const double alpha = std::sin (w0) / (2.0 * q), c = std::cos (w0);
            return normalise (1.0 + alpha * A, -2.0 * c, 1.0 - alpha * A,
                              1.0 + alpha / A, -2.0 * c, 1.0 - alpha / A);
        }

        inline BiquadCoeffs lowShelf (double fs, double f, double gainDb, double q)
        {
            if (std::fabs (gainDb) < 0.005) return {};
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * kPi * clampFreq (f, fs) / fs;
            const double alpha = std::sin (w0) / (2.0 * q), c = std::cos (w0);
            const double beta = 2.0 * std::sqrt (A) * alpha;
            return normalise (A * ((A + 1) - (A - 1) * c + beta),
                              2 * A * ((A - 1) - (A + 1) * c),
                              A * ((A + 1) - (A - 1) * c - beta),
                              (A + 1) + (A - 1) * c + beta,
                              -2 * ((A - 1) + (A + 1) * c),
                              (A + 1) + (A - 1) * c - beta);
        }

        inline BiquadCoeffs highShelf (double fs, double f, double gainDb, double q)
        {
            if (std::fabs (gainDb) < 0.005) return {};
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w0 = 2.0 * kPi * clampFreq (f, fs) / fs;
            const double alpha = std::sin (w0) / (2.0 * q), c = std::cos (w0);
            const double beta = 2.0 * std::sqrt (A) * alpha;
            return normalise (A * ((A + 1) + (A - 1) * c + beta),
                              -2 * A * ((A - 1) + (A + 1) * c),
                              A * ((A + 1) + (A - 1) * c - beta),
                              (A + 1) - (A - 1) * c + beta,
                              2 * ((A - 1) - (A + 1) * c),
                              (A + 1) - (A - 1) * c - beta);
        }

        inline BiquadCoeffs highPass (double fs, double f, double q)
        {
            const double w0 = 2.0 * kPi * clampFreq (f, fs) / fs;
            const double alpha = std::sin (w0) / (2.0 * q), c = std::cos (w0);
            return normalise ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + alpha, -2 * c, 1 - alpha);
        }

        // Réponse en fréquence (en dB) : sert à dessiner les courbes dans l'interface.
        inline double magnitudeDb (const BiquadCoeffs& k, double f, double fs)
        {
            const double w = 2.0 * kPi * f / fs;
            const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
            const auto num = k.b0 + k.b1 * z1 + k.b2 * z2;
            const auto den = 1.0 + k.a1 * z1 + k.a2 * z2;
            return 20.0 * std::log10 (std::max (std::abs (num / den), 1.0e-9));
        }
    }

    // Cascade de N biquads, état séparé par canal. Forme "transposée directe II"
    // en double précision (important pour les filtres graves à basse fréquence).
    template <int N>
    class BiquadCascade
    {
    public:
        std::array<BiquadCoeffs, (size_t) N> coeffs;

        void reset() { for (auto& ch : state) for (auto& s : ch) s = {}; }

        void process (int channel, float* data, int numSamples) noexcept
        {
            auto& st = state[(size_t) channel];
            for (int k = 0; k < N; ++k)
            {
                const auto& c = coeffs[(size_t) k];
                auto& s = st[(size_t) k];
                for (int i = 0; i < numSamples; ++i)
                {
                    const double x = data[i];
                    const double y = c.b0 * x + s.z1;
                    s.z1 = c.b1 * x - c.a1 * y + s.z2;
                    s.z2 = c.b2 * x - c.a2 * y;
                    data[i] = (float) y;
                }
            }
        }

    private:
        struct State { double z1 = 0.0, z2 = 0.0; };
        std::array<std::array<State, (size_t) N>, 2> state {};
    };
}
