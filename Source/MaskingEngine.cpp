// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#include "MaskingEngine.h"
#include <algorithm>
#include <cmath>

namespace cm
{
namespace
{
    constexpr float kPi = 3.14159265358979f;

    // --- Règles de conflit -------------------------------------------------
    // "Quand ces deux rôles sont présents, ils se marchent dessus entre lo et hi Hz."
    // Symétriques ; Role::Any = joker. Le poids (0..1) dit à quel point le conflit est sérieux.
    struct Rule { Role a, b; float lo, hi, weight; };

    constexpr Role K = Role::Kick, S8 = Role::Sub808, B = Role::Bass, SN = Role::SnareClap,
                   HH = Role::HiHats, PC = Role::Percussion, LV = Role::LeadVocal,
                   BV = Role::BackVocals, ME = Role::Melody, PD = Role::Pad,
                   SA = Role::Sample, FX = Role::FX, ANY = Role::Any;

    const Rule kRules[] = {
        // ---- Sub / grave : la bataille Kick vs 808 ----
        { K,  S8, 30.f,   90.f, 1.00f },
        { K,  B,  35.f,  120.f, 0.90f },
        { S8, B,  30.f,  200.f, 1.00f },
        // ---- Zone de "boue" 100-400 Hz ----
        { S8, ME, 80.f,  300.f, 0.60f },
        { S8, PD, 80.f,  350.f, 0.70f },
        { S8, SA, 80.f,  300.f, 0.60f },
        { B,  ME, 100.f, 350.f, 0.70f },
        { B,  PD, 100.f, 350.f, 0.70f },
        { B,  SA, 100.f, 350.f, 0.70f },
        { K,  SN, 120.f, 250.f, 0.40f },
        // ---- Voix lead : médiums 1.5-5 kHz, présence ----
        { LV, ME, 1500.f, 5000.f, 0.90f },
        { LV, PD, 1000.f, 5000.f, 0.90f },
        { LV, SA, 1500.f, 5000.f, 0.90f },
        { LV, FX, 1500.f, 5000.f, 0.60f },
        { LV, SN, 1000.f, 4000.f, 0.50f },
        { LV, BV, 300.f,  4000.f, 0.60f },
        { LV, HH, 5000.f, 10000.f, 0.40f },   // sibilance vs hi-hats
        { LV, ME, 200.f,  500.f,  0.40f },
        { LV, PD, 200.f,  500.f,  0.50f },
        { LV, SA, 200.f,  500.f,  0.50f },
        { BV, ME, 1000.f, 4000.f, 0.60f },
        { BV, PD, 1000.f, 4000.f, 0.60f },
        { BV, SA, 1000.f, 4000.f, 0.60f },
        // ---- Aigus / air : hats, percussions afro, snare ----
        { HH, PC, 6000.f, 16000.f, 0.80f },
        { HH, SN, 3000.f, 9000.f,  0.50f },
        { PC, SN, 2000.f, 8000.f,  0.50f },
        { HH, ME, 8000.f, 16000.f, 0.40f },
        { HH, PD, 8000.f, 16000.f, 0.40f },
        { HH, SA, 8000.f, 16000.f, 0.40f },
        // ---- Filet de sécurité : bas-médium pour tout le monde ----
        { ANY, ANY, 200.f, 500.f, 0.35f }
    };

    bool matches (Role ruleRole, Role actual) { return ruleRole == Role::Any || ruleRole == actual; }

    // Bord adouci : 1 dans [lo,hi], redescend en cosinus sur une demi-octave.
    float shape (float f, float lo, float hi)
    {
        float d = 0.0f;
        if (f < lo)      d = std::log2 (lo / f);
        else if (f > hi) d = std::log2 (f / hi);
        if (d >= 0.5f) return 0.0f;
        return 0.5f * (1.0f + std::cos (2.0f * kPi * d));
    }

    // Table pré-calculée [roleA][roleB][bande] pour ne rien calculer dans le thread audio.
    struct ConflictTable
    {
        std::array<float, (size_t) kNumRoles * kNumRoles * kNumBands> w {};

        ConflictTable()
        {
            const auto& layout = BandLayout::get();
            for (int a = 0; a < kNumRoles; ++a)
                for (int b = 0; b < kNumRoles; ++b)
                    for (int band = 0; band < kNumBands; ++band)
                    {
                        float best = 0.0f;
                        const float f = layout.centre[(size_t) band];
                        for (const auto& r : kRules)
                        {
                            const bool direct  = matches (r.a, (Role) a) && matches (r.b, (Role) b);
                            const bool swapped = matches (r.a, (Role) b) && matches (r.b, (Role) a);
                            if (direct || swapped)
                                best = std::max (best, r.weight * shape (f, r.lo, r.hi));
                        }
                        w[index (a, b, band)] = best;
                    }
        }
        static size_t index (int a, int b, int band) { return ((size_t) a * kNumRoles + (size_t) b) * kNumBands + (size_t) band; }
        static const ConflictTable& get() { static const ConflictTable t; return t; }
    };

    float clamp01 (float x) { return std::min (1.0f, std::max (0.0f, x)); }
}

float MaskingEngine::conflictWeight (Role a, Role b, float freqHz)
{
    float best = 0.0f;
    for (const auto& r : kRules)
    {
        const bool direct  = matches (r.a, a) && matches (r.b, b);
        const bool swapped = matches (r.a, b) && matches (r.b, a);
        if (direct || swapped)
            best = std::max (best, r.weight * shape (freqHz, r.lo, r.hi));
    }
    return best;
}

float MaskingEngine::effectivePriority (Role role, float userPriority01)
{
    const float p = roleInfo (role).basePriority + (userPriority01 - 0.5f);
    return std::min (1.0f, std::max (0.05f, p));
}

void MaskingEngine::computeTargetCuts (const TrackSnapshot& me,
                                       const TrackSnapshot* others, int numOthers,
                                       const MaskingSettings& s, BandArray& cutOut)
{
    const auto& table = ConflictTable::get();
    constexpr float kAudibleFloorDb = -80.0f;   // en dessous : bande considérée vide
    constexpr float kAudibleRangeDb = 20.0f;

    for (int band = 0; band < kNumBands; ++band)
    {
        const float mine = me.levelDb[(size_t) band];
        const float myAudibility = clamp01 ((mine - kAudibleFloorDb) / kAudibleRangeDb);

        // Probabilité cumulée qu'AU MOINS une autre piste me masque dans cette bande :
        // 1 - produit(1 - c_i). Reste borné entre 0 et 1 même avec 40 pistes.
        float notMasked = 1.0f;

        if (myAudibility > 0.0f)
            for (int i = 0; i < numOthers; ++i)
            {
                const auto& o = others[i];
                const float conflict = table.w[ConflictTable::index ((int) me.role, (int) o.role, band)];
                if (conflict <= 0.0f) continue;

                const float oLevel = o.levelDb[(size_t) band];
                const float otherAudibility = clamp01 ((oLevel - kAudibleFloorDb) / kAudibleRangeDb);
                if (otherAudibility <= 0.0f) continue;

                // L'autre est-elle aussi forte (ou plus) que moi ? 12 dB plus faible -> 0, égale -> 1.
                const float competition = clamp01 ((oLevel - mine + 12.0f) / 12.0f);

                // Part de l'effort que je prends : priorités égales -> 0.5 chacun.
                const float share = o.priority / (o.priority + me.priority);

                const float c = conflict * myAudibility * otherAudibility * competition * share;
                notMasked *= (1.0f - clamp01 (c));
            }

        const float masking = 1.0f - notMasked;
        cutOut[(size_t) band] = -s.maxCutDb * clamp01 (s.strength) * masking;
    }
}
}
