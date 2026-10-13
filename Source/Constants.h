// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  Constants.h : constantes globales du plugin.
// ============================================================================
#include <array>
#include <cmath>

namespace cm
{
    constexpr int   kNumBands       = 24;     // bandes d'analyse / d'EQ automatique
    constexpr int   kNumManualBands = 5;      // bandes de l'EQ manuel (shelf, 3 cloches, shelf)
    constexpr int   kMaxSlots       = 64;     // nombre max d'instances qui communiquent
    constexpr int   kMaxChannels    = 2;      // mono ou stéréo

    constexpr float kBandMinHz      = 30.0f;  // centre de la 1re bande
    constexpr float kBandMaxHz      = 16000.0f; // centre de la dernière bande
    constexpr float kBankQ          = 3.0f;   // Q des filtres de l'EQ automatique

    constexpr float kFloorDb        = -100.0f; // niveau "silence"
    constexpr int   kFftOrder       = 12;      // 2^12 = 4096 points (bonne résolution dans le grave)
    constexpr int   kFftSize        = 1 << kFftOrder;
    constexpr int   kFftHop         = 1024;    // une analyse toutes les 1024 samples

    constexpr unsigned kSlotTimeoutMs = 500;   // une instance muette depuis 500 ms est ignorée

    // Répartition logarithmique des bandes (centres + bords).
    struct BandLayout
    {
        std::array<float, kNumBands>     centre {};
        std::array<float, kNumBands + 1> edge {};

        BandLayout()
        {
            const double ratio = std::pow ((double) kBandMaxHz / kBandMinHz, 1.0 / (kNumBands - 1));
            for (int i = 0; i < kNumBands; ++i)
                centre[(size_t) i] = (float) (kBandMinHz * std::pow (ratio, i));

            for (int i = 0; i < kNumBands; ++i)
                edge[(size_t) i] = (float) (centre[(size_t) i] / std::sqrt (ratio));
            edge[kNumBands] = (float) (centre[kNumBands - 1] * std::sqrt (ratio));
        }

        static const BandLayout& get()
        {
            static const BandLayout layout;   // construit une seule fois (thread-safe en C++11+)
            return layout;
        }
    };
}
