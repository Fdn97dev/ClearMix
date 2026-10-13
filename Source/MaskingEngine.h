// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  MaskingEngine : le "cerveau" du plugin. 100 % C++ standard (aucune dépendance
//  JUCE) -> testable seul et sûr pour le thread audio (aucune allocation).
//
//  Entrée : mon niveau par bande + celui des autres pistes (rôle, priorité).
//  Sortie : la coupe (en dB, <= 0) à appliquer sur chacune de mes bandes.
// ============================================================================
#include "Constants.h"
#include "Roles.h"

namespace cm
{
    // Photo d'une piste à un instant t (copiée depuis le bus partagé).
    struct TrackSnapshot
    {
        Role  role     = Role::Other;
        float priority = 0.5f;                       // priorité EFFECTIVE (0..1)
        std::array<float, kNumBands> levelDb {};     // énergie par bande en dB
    };

    struct MaskingSettings
    {
        float strength = 1.0f;   // 0..1 : intensité globale ("Amount")
        float maxCutDb = 6.0f;   // coupe maximale par bande (dB, valeur positive)
    };

    using BandArray = std::array<float, kNumBands>;

    class MaskingEngine
    {
    public:
        // Poids de conflit (0..1) entre 2 rôles à une fréquence donnée.
        static float conflictWeight (Role a, Role b, float freqHz);

        // Priorité effective = priorité du rôle + décalage utilisateur (slider 0..1, 0.5 = neutre).
        static float effectivePriority (Role role, float userPriority01);

        // Calcule les coupes cibles (dB <= 0) pour 'me' face à 'others'.
        static void computeTargetCuts (const TrackSnapshot& me,
                                       const TrackSnapshot* others, int numOthers,
                                       const MaskingSettings& settings,
                                       BandArray& cutDbOut);
    };
}
