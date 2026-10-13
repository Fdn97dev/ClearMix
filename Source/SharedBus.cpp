// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#include "SharedBus.h"

namespace cm
{
SharedBus& SharedBus::get()
{
    static SharedBus bus;   // un seul exemplaire par processus (par binaire de plugin)
    return bus;
}

int SharedBus::acquireSlot()
{
    for (int i = 0; i < kMaxSlots; ++i)
    {
        bool expected = false;
        if (slots[(size_t) i].active.compare_exchange_strong (expected, true))
        {
            auto& s = slots[(size_t) i];
            s.linked.store (true);
            s.lastUpdateMs.store (0);
            for (auto& l : s.levelDb) l.store (kFloorDb);
            return i;
        }
    }
    return -1;
}

void SharedBus::releaseSlot (int index)
{
    if (index >= 0 && index < kMaxSlots)
        slots[(size_t) index].active.store (false);
}

void SharedBus::publish (int index, Role role, float priority, int group, bool linked,
                         const BandArray& levels, uint32_t nowMs)
{
    if (index < 0) return;
    auto& s = slots[(size_t) index];
    s.role.store ((int) role, std::memory_order_relaxed);
    s.priority.store (priority, std::memory_order_relaxed);
    s.group.store (group, std::memory_order_relaxed);
    s.linked.store (linked, std::memory_order_relaxed);
    for (size_t b = 0; b < (size_t) kNumBands; ++b)
        s.levelDb[b].store (levels[b], std::memory_order_relaxed);
    s.lastUpdateMs.store (nowMs, std::memory_order_release);
}

int SharedBus::snapshotOthers (int self, int group, uint32_t nowMs, TrackSnapshot* out, int maxOut) const
{
    int n = 0;
    for (int i = 0; i < kMaxSlots && n < maxOut; ++i)
    {
        if (i == self) continue;
        const auto& s = slots[(size_t) i];
        if (! s.active.load (std::memory_order_relaxed)) continue;
        if (! s.linked.load (std::memory_order_relaxed)) continue;
        if (s.group.load (std::memory_order_relaxed) != group) continue;

        const uint32_t last = s.lastUpdateMs.load (std::memory_order_acquire);
        if (last == 0 || (uint32_t) (nowMs - last) > kSlotTimeoutMs) continue;   // piste muette / en bypass

        auto& t = out[n++];
        t.role = (Role) s.role.load (std::memory_order_relaxed);
        t.priority = s.priority.load (std::memory_order_relaxed);
        for (size_t b = 0; b < (size_t) kNumBands; ++b)
            t.levelDb[b] = s.levelDb[b].load (std::memory_order_relaxed);
    }
    return n;
}
}
