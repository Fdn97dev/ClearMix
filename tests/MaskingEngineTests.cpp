// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
// Tests unitaires du moteur de masquage (aucune dépendance JUCE).
// Lancer :  ctest --test-dir build/debug --output-on-failure
#include "MaskingEngine.h"
#include "SharedBus.h"
#include <cstdio>
#include <cstdlib>

using namespace cm;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf ("ECHEC : %s  (%s:%d)\n", msg, __FILE__, __LINE__); ++failures; } } while (0)

static int bandAt (float hz)
{
    const auto& l = BandLayout::get();
    int best = 0;
    for (int i = 0; i < kNumBands; ++i)
        if (std::fabs (std::log (l.centre[(size_t) i] / hz)) < std::fabs (std::log (l.centre[(size_t) best] / hz))) best = i;
    return best;
}

static TrackSnapshot makeTrack (Role r, float prio, float levelDb, int fromBand, int toBand)
{
    TrackSnapshot t; t.role = r; t.priority = prio; t.levelDb.fill (-100.0f);
    for (int b = fromBand; b <= toBand; ++b) t.levelDb[(size_t) b] = levelDb;
    return t;
}

int main()
{
    MaskingSettings st; st.strength = 1.0f; st.maxCutDb = 6.0f;

    // 1) Deux pistes silencieuses : aucune coupe.
    {
        auto me = makeTrack (Role::Sub808, 0.5f, -100.f, 0, kNumBands - 1);
        auto other = makeTrack (Role::Kick, 0.5f, -100.f, 0, kNumBands - 1);
        BandArray cut {}; MaskingEngine::computeTargetCuts (me, &other, 1, st, cut);
        for (float c : cut) CHECK (c == 0.0f, "silence -> pas de coupe");
    }
    // 2) 808 et kick dans le sub, mêmes niveaux/priorités : coupe dans le sub, rien dans les aigus.
    {
        const int sub = bandAt (50.f), air = bandAt (10000.f);
        auto me = makeTrack (Role::Sub808, 0.5f, -20.f, 0, kNumBands - 1);
        auto other = makeTrack (Role::Kick, 0.5f, -20.f, 0, kNumBands - 1);
        BandArray cut {}; MaskingEngine::computeTargetCuts (me, &other, 1, st, cut);
        CHECK (cut[(size_t) sub] < -1.0f, "sub : coupe attendue");
        CHECK (cut[(size_t) air] == 0.0f, "air : aucune coupe attendue entre 808 et kick");
        for (float c : cut) CHECK (c >= -st.maxCutDb - 1e-4f && c <= 0.0f, "coupe bornée");
    }
    // 3) La piste prioritaire coupe moins que l'autre.
    {
        const int sub = bandAt (50.f);
        auto kick = makeTrack (Role::Kick, 0.9f, -20.f, 0, kNumBands - 1);
        auto s808 = makeTrack (Role::Sub808, 0.3f, -20.f, 0, kNumBands - 1);
        BandArray cutKick {}, cut808 {};
        MaskingEngine::computeTargetCuts (kick, &s808, 1, st, cutKick);
        MaskingEngine::computeTargetCuts (s808, &kick, 1, st, cut808);
        CHECK (cut808[(size_t) sub] < cutKick[(size_t) sub], "la piste peu prioritaire doit etre plus creusee");
    }
    // 4) Si l'autre piste est beaucoup plus faible que moi, je ne me creuse presque pas.
    {
        const int sub = bandAt (50.f);
        auto me = makeTrack (Role::Sub808, 0.5f, -10.f, 0, kNumBands - 1);
        auto other = makeTrack (Role::Kick, 0.5f, -40.f, 0, kNumBands - 1);
        BandArray cut {}; MaskingEngine::computeTargetCuts (me, &other, 1, st, cut);
        CHECK (cut[(size_t) sub] > -0.01f, "autre piste beaucoup plus faible -> pas de coupe");
    }
    // 5) strength = 0 -> aucune coupe ; beaucoup de pistes -> toujours borné.
    {
        auto me = makeTrack (Role::Pad, 0.2f, -20.f, 0, kNumBands - 1);
        TrackSnapshot others[40];
        for (auto& o : others) o = makeTrack (Role::LeadVocal, 0.95f, -15.f, 0, kNumBands - 1);
        BandArray cut {};
        MaskingSettings off = st; off.strength = 0.0f;
        MaskingEngine::computeTargetCuts (me, others, 40, off, cut);
        for (float c : cut) CHECK (c == 0.0f, "strength=0 -> pas de coupe");
        MaskingEngine::computeTargetCuts (me, others, 40, st, cut);
        const int mid = bandAt (3000.f);
        CHECK (cut[(size_t) mid] < -5.0f, "pad vs 40 voix : forte coupe dans les mediums");
        for (float c : cut) CHECK (c >= -st.maxCutDb - 1e-4f, "borne max respectee");
    }
    // 6) Priorité effective bornée.
    {
        CHECK (MaskingEngine::effectivePriority (Role::LeadVocal, 1.0f) <= 1.0f, "prio <= 1");
        CHECK (MaskingEngine::effectivePriority (Role::FX, 0.0f) >= 0.05f, "prio >= 0.05");
    }
    // 7) Le bus partagé : acquisition, publication, filtrage par groupe et timeout.
    {
        auto& bus = SharedBus::get();
        const int a = bus.acquireSlot(), b = bus.acquireSlot(), c = bus.acquireSlot();
        CHECK (a >= 0 && b >= 0 && c >= 0 && a != b && b != c, "slots distincts");
        BandArray lv; lv.fill (-30.0f);
        bus.publish (a, Role::Kick, 0.7f, 0, true, lv, 1000);
        bus.publish (b, Role::Sub808, 0.6f, 0, true, lv, 1000);
        bus.publish (c, Role::Pad, 0.2f, 1, true, lv, 1000);   // autre groupe
        TrackSnapshot out[kMaxSlots];
        CHECK (bus.snapshotOthers (a, 0, 1100, out, kMaxSlots) == 1, "voit uniquement b (meme groupe)");
        CHECK (out[0].role == Role::Sub808, "role transmis");
        CHECK (bus.snapshotOthers (a, 0, 5000, out, kMaxSlots) == 0, "piste muette ignoree (timeout)");
        bus.releaseSlot (b);
        CHECK (bus.snapshotOthers (a, 0, 1100, out, kMaxSlots) == 0, "slot libere ignore");
        bus.releaseSlot (a); bus.releaseSlot (c);
    }

    if (failures == 0) std::printf ("Tous les tests sont passes.\n");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
