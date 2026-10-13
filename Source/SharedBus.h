// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  SharedBus : mémoire partagée entre toutes les instances du plugin dans le
//  MÊME processus (même DAW). Chaque instance réserve un "slot" et y publie
//  son rôle, sa priorité et son spectre par bandes. Les autres le lisent.
//
//  Tout est basé sur std::atomic : jamais de mutex dans le thread audio.
// ============================================================================
#include "Constants.h"
#include "MaskingEngine.h"
#include <atomic>
#include <cstdint>

namespace cm
{
    struct SharedSlot
    {
        std::atomic<bool>     active   { false };
        std::atomic<bool>     linked   { true };
        std::atomic<int>      group    { 0 };
        std::atomic<int>      role     { (int) Role::Other };
        std::atomic<float>    priority { 0.5f };
        std::atomic<uint32_t> lastUpdateMs { 0 };
        std::array<std::atomic<float>, kNumBands> levelDb;

        SharedSlot() { for (auto& l : levelDb) l.store (kFloorDb); }
    };

    class SharedBus
    {
    public:
        static SharedBus& get();

        // Réserve un slot libre (thread message). Retourne -1 si tout est pris.
        int  acquireSlot();
        void releaseSlot (int index);

        SharedSlot&       slot (int i)       { return slots[(size_t) i]; }
        const SharedSlot& slot (int i) const { return slots[(size_t) i]; }

        // Publie mon état (thread audio, sans allocation).
        void publish (int index, Role role, float priority, int group, bool linked,
                      const BandArray& levels, uint32_t nowMs);

        // Copie les pistes actives du même groupe (hors moi). Retourne leur nombre.
        int  snapshotOthers (int self, int group, uint32_t nowMs, TrackSnapshot* out, int maxOut) const;

    private:
        std::array<SharedSlot, kMaxSlots> slots;
    };
}
