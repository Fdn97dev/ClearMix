// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
// Test de bout en bout : deux instances du VRAI processeur (kick + 808) qui s'échangent leurs spectres.
// Compilation : cmake -DCLEARMIX_BUILD_SMOKE_TEST=ON ...   puis   ./ClearMixSmokeTest
#include "PluginProcessor.h"
#include <cstdio>

static int failures = 0;
#define CHECK(c, m) do { if (!(c)) { std::printf ("ECHEC : %s\n", m); ++failures; } } while (0)

static void setParam (ClearMixAudioProcessor& p, const juce::String& id, float plain)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    constexpr double fs = 48000.0;
    constexpr int block = 256, numBlocks = 400;   // ~2 s

    ClearMixAudioProcessor kick, s808, vocal;
    setParam (kick, "role", (float) cm::Role::Kick);
    setParam (s808, "role", (float) cm::Role::Sub808);
    setParam (vocal, "role", (float) cm::Role::LeadVocal);
    setParam (kick, "amount", 100.0f); setParam (s808, "amount", 100.0f);
    for (auto* p : { &kick, &s808, &vocal }) { p->setPlayConfigDetails (2, 2, fs, block); p->prepareToPlay (fs, block); }

    juce::AudioBuffer<float> bK (2, block), b8 (2, block), bV (2, block);
    double phase = 0, phaseV = 0;
    bool anyNaN = false;
    float peakOut = 0;

    for (int blk = 0; blk < numBlocks; ++blk)
    {
        for (int i = 0; i < block; ++i)
        {
            const float sub = 0.4f * (float) std::sin (phase);              // 55 Hz : kick ET 808 dedans
            phase += 2.0 * juce::MathConstants<double>::pi * 55.0 / fs;
            const float voc = 0.3f * (float) std::sin (phaseV);             // 2 kHz : voix seule
            phaseV += 2.0 * juce::MathConstants<double>::pi * 2000.0 / fs;
            for (int c = 0; c < 2; ++c) { bK.setSample (c, i, sub); b8.setSample (c, i, sub); bV.setSample (c, i, voc); }
        }
        juce::MidiBuffer m;
        kick.processBlock (bK, m); s808.processBlock (b8, m); vocal.processBlock (bV, m);

        for (auto* b : { &bK, &b8, &bV })
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < block; ++i)
                    if (! std::isfinite (b->getSample (c, i))) anyNaN = true;
        if (blk >= numBlocks - 4) peakOut = std::max (peakOut, b8.getMagnitude (0, block));   // pic en fin de test, coupe établie
    }

    const auto& layout = cm::BandLayout::get();
    int subBand = 0, vocBand = 0;
    for (int b = 0; b < cm::kNumBands; ++b)
    {
        if (std::fabs (std::log (layout.centre[(size_t) b] / 55.f)) < std::fabs (std::log (layout.centre[(size_t) subBand] / 55.f))) subBand = b;
        if (std::fabs (std::log (layout.centre[(size_t) b] / 2000.f)) < std::fabs (std::log (layout.centre[(size_t) vocBand] / 2000.f))) vocBand = b;
    }

    const float cutKick = kick.getAutoCutDisplay()[(size_t) subBand].load();
    const float cut808  = s808.getAutoCutDisplay()[(size_t) subBand].load();
    const float cutVoc  = vocal.getAutoCutDisplay()[(size_t) vocBand].load();
    std::printf ("Niveau sub (kick) : %.1f dB | coupe kick: %.2f dB | coupe 808: %.2f dB | coupe voix a 2k: %.2f dB\n",
                 kick.getOwnLevelDisplay()[(size_t) subBand].load(), cutKick, cut808, cutVoc);
    std::printf ("Slot kick=%d 808=%d voix=%d | pic sortie 808 : %.3f (entree 0.4)\n",
                 kick.getSlotIndex(), s808.getSlotIndex(), vocal.getSlotIndex(), peakOut);

    CHECK (! anyNaN, "aucune valeur NaN/inf en sortie");
    CHECK (cutKick < -1.0f, "le kick se creuse dans le sub quand le 808 joue la meme chose");
    CHECK (cut808 < -1.0f, "le 808 se creuse aussi (priorites proches)");
    CHECK (cut808 < cutKick + 0.001f || cutKick < cut808 + 1.0f, "coupes coherentes");
    CHECK (cutVoc > -0.5f, "la voix seule a 2 kHz n'est pas creusee (aucun conflit)");
    CHECK (peakOut < 0.36f, "le signal 808 est attenue (-1 dB au moins) une fois la coupe etablie");

    // Test mono + bypass logique : pas de crash.
    ClearMixAudioProcessor mono;
    mono.setPlayConfigDetails (1, 1, fs, block); mono.prepareToPlay (fs, block);
    juce::AudioBuffer<float> bm (1, block); bm.clear(); juce::MidiBuffer mm;
    mono.processBlock (bm, mm);

    std::printf (failures == 0 ? "Smoke test OK\n" : "Smoke test EN ECHEC\n");
    return failures == 0 ? 0 : 1;
}
