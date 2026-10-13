// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#pragma once
// ============================================================================
//  Parameters.h : identifiants des paramètres (les "clés" de l'automation DAW).
//  Ne JAMAIS renommer un ID une fois le plugin distribué : ça casse les projets.
// ============================================================================
#include <juce_core/juce_core.h>

namespace cm::id
{
    inline const juce::String role      { "role" };
    inline const juce::String priority  { "priority" };
    inline const juce::String group     { "group" };
    inline const juce::String link      { "link" };
    inline const juce::String autoOn    { "auto_on" };
    inline const juce::String amount    { "amount" };
    inline const juce::String maxCut    { "max_cut" };
    inline const juce::String attack    { "attack" };
    inline const juce::String release   { "release" };
    inline const juce::String outGain   { "out_gain" };
    inline const juce::String hpf       { "hpf" };

    inline juce::String bandFreq (int i) { return "m" + juce::String (i + 1) + "_freq"; }
    inline juce::String bandGain (int i) { return "m" + juce::String (i + 1) + "_gain"; }
    inline juce::String bandQ    (int i) { return "m" + juce::String (i + 1) + "_q"; }
}
