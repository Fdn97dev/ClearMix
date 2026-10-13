// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  Roles.h : les "rôles" d'une piste dans un mix moderne (trap / afro / RnB).
//  Chaque rôle a une priorité de base : plus elle est haute, moins la piste
//  se fait creuser par les autres.
// ============================================================================
#include <array>

namespace cm
{
    enum class Role : int
    {
        Kick = 0, Sub808, Bass, SnareClap, HiHats, Percussion,
        LeadVocal, BackVocals, Melody, Pad, Sample, FX, Other,
        Count,
        Any = Count          // joker, utilisé uniquement dans les règles de conflit
    };

    constexpr int kNumRoles = (int) Role::Count;

    struct RoleInfo
    {
        const char* name;
        float basePriority;  // 0..1
    };

    inline const std::array<RoleInfo, kNumRoles>& roleTable()
    {
        static const std::array<RoleInfo, kNumRoles> t { {
            { "Kick",                   0.70f },
            { "808 / Sub",              0.65f },
            { "Basse",                  0.55f },
            { "Snare / Clap",           0.60f },
            { "Hi-Hats",                0.45f },
            { "Percussions (afro)",     0.40f },
            { "Voix lead",              0.95f },
            { "Backs / Ad-libs",        0.60f },
            { "Melodie (keys, guitare, lead)", 0.50f },
            { "Pad / Nappe",            0.25f },
            { "Sample / Loop",          0.35f },
            { "FX",                     0.20f },
            { "Autre",                  0.40f }
        } };
        return t;
    }

    inline const RoleInfo& roleInfo (Role r) { return roleTable()[(size_t) r]; }
}
