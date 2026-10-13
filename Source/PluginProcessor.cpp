// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

using namespace cm;

// ----------------------------------------------------------------------------
//  Déclaration des paramètres
// ----------------------------------------------------------------------------
juce::AudioProcessorValueTreeState::ParameterLayout ClearMixAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    StringArray roleNames;
    for (const auto& r : roleTable()) roleNames.add (r.name);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::role, 1 }, "Role", roleNames, (int) Role::Other));

    StringArray groups;
    for (int i = 1; i <= 8; ++i) groups.add ("Groupe " + String (i));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id::group, 1 }, "Groupe de mix", groups, 0));

    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { id::link, 1 },   "Link", true));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { id::autoOn, 1 }, "Auto EQ", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::priority, 1 }, "Priorite",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::amount, 1 }, "Amount",
                 NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f, AudioParameterFloatAttributes().withLabel ("%")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::maxCut, 1 }, "Max Cut",
                 NormalisableRange<float> (0.0f, 12.0f, 0.1f), 6.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::attack, 1 }, "Attack",
                 NormalisableRange<float> (5.0f, 200.0f, 0.1f, 0.5f), 30.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::release, 1 }, "Release",
                 NormalisableRange<float> (20.0f, 1000.0f, 0.1f, 0.5f), 200.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::outGain, 1 }, "Output",
                 NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));

    NormalisableRange<float> freqRange (20.0f, 20000.0f, 0.1f);
    freqRange.setSkewForCentre (1000.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::hpf, 1 }, "HPF",
                 freqRange, 20.0f, AudioParameterFloatAttributes().withLabel ("Hz")));

    // EQ manuel : [0] shelf grave, [1..3] cloches, [4] shelf aigu
    const float defFreq[kNumManualBands] = { 80.0f, 250.0f, 1500.0f, 5000.0f, 10000.0f };
    const float defQ[kNumManualBands]    = { 0.7f, 1.0f, 1.0f, 1.0f, 0.7f };
    for (int i = 0; i < kNumManualBands; ++i)
    {
        const String n = "Manuel " + String (i + 1);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::bandFreq (i), 1 }, n + " Freq",
                     freqRange, defFreq[i], AudioParameterFloatAttributes().withLabel ("Hz")));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::bandGain (i), 1 }, n + " Gain",
                     NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
        NormalisableRange<float> qRange (0.1f, 10.0f, 0.01f);
        qRange.setSkewForCentre (1.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id::bandQ (i), 1 }, n + " Q", qRange, defQ[i]));
    }
    return { p.begin(), p.end() };
}

// ----------------------------------------------------------------------------
ClearMixAudioProcessor::ClearMixAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ClearMixState", createLayout())
{
    pRole     = apvts.getRawParameterValue (id::role);
    pPriority = apvts.getRawParameterValue (id::priority);
    pGroup    = apvts.getRawParameterValue (id::group);
    pLink     = apvts.getRawParameterValue (id::link);
    pAutoOn   = apvts.getRawParameterValue (id::autoOn);
    pAmount   = apvts.getRawParameterValue (id::amount);
    pMaxCut   = apvts.getRawParameterValue (id::maxCut);
    pAttack   = apvts.getRawParameterValue (id::attack);
    pRelease  = apvts.getRawParameterValue (id::release);
    pOutGain  = apvts.getRawParameterValue (id::outGain);
    pHpf      = apvts.getRawParameterValue (id::hpf);
    for (int i = 0; i < kNumManualBands; ++i)
    {
        pFreq[(size_t) i] = apvts.getRawParameterValue (id::bandFreq (i));
        pGain[(size_t) i] = apvts.getRawParameterValue (id::bandGain (i));
        pQ[(size_t) i]    = apvts.getRawParameterValue (id::bandQ (i));
    }

    for (auto& a : autoCutDisplay)    a.store (0.0f);
    for (auto& a : ownLevelDisplay)   a.store (kFloorDb);
    currentCutDb.fill (0.0f);

    slotIndex = SharedBus::get().acquireSlot();   // je m'enregistre auprès des autres instances
}

ClearMixAudioProcessor::~ClearMixAudioProcessor()
{
    SharedBus::get().releaseSlot (slotIndex);
}

bool ClearMixAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void ClearMixAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate);
    analyzer.prepare (sampleRate);
    autoBank.reset();
    manualEq.reset();
    currentCutDb.fill (0.0f);
    outGainSmoothed.reset (sampleRate, 0.02);
    outGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutGain->load()));
    computeManualCoeffs (sampleRate, manualEq.coeffs);
    updateAutoFilters (sampleRate);
}

// ----------------------------------------------------------------------------
void ClearMixAudioProcessor::computeManualCoeffs (double fs, std::array<BiquadCoeffs, kNumManualBands + 1>& out) const
{
    out[0] = (pHpf->load() <= 20.5f) ? BiquadCoeffs {} : biquad::highPass (fs, pHpf->load(), 0.7071);   // 20 Hz = HPF désactivé
    out[1] = biquad::lowShelf (fs, pFreq[0]->load(), pGain[0]->load(), pQ[0]->load());
    for (int i = 1; i < kNumManualBands - 1; ++i)
        out[(size_t) i + 1] = biquad::peaking (fs, pFreq[(size_t) i]->load(), pGain[(size_t) i]->load(), pQ[(size_t) i]->load());
    const size_t last = kNumManualBands - 1;
    out[kNumManualBands] = biquad::highShelf (fs, pFreq[last]->load(), pGain[last]->load(), pQ[last]->load());
}

void ClearMixAudioProcessor::updateAutoFilters (double fs)
{
    const auto& layout = BandLayout::get();
    for (int b = 0; b < kNumBands; ++b)
        autoBank.coeffs[(size_t) b] = biquad::peaking (fs, layout.centre[(size_t) b], currentCutDb[(size_t) b], kBankQ);
}

// ----------------------------------------------------------------------------
//  THREAD AUDIO : pas d'allocation, pas de mutex, pas d'I/O.
// ----------------------------------------------------------------------------
void ClearMixAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = std::min (buffer.getNumChannels(), kMaxChannels);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;

    const double fs = currentSampleRate.load();
    const auto nowMs = (uint32_t) juce::Time::getMillisecondCounter();
    auto& bus = SharedBus::get();

    // 1) ANALYSE du signal d'entrée (avant EQ -> pas de boucle de rétroaction entre pistes)
    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (numCh > 1 ? 1 : 0);
    analyzer.process (inL, inR, n);
    const auto& myLevels = analyzer.levels();
    for (size_t b = 0; b < (size_t) kNumBands; ++b) ownLevelDisplay[b].store (myLevels[b], std::memory_order_relaxed);

    // 2) PUBLICATION de mon état pour les autres instances
    const Role role = (Role) juce::jlimit (0, kNumRoles - 1, (int) std::lround (pRole->load()));
    const float prio = MaskingEngine::effectivePriority (role, pPriority->load());
    const int group = (int) std::lround (pGroup->load());
    const bool linked = pLink->load() > 0.5f;
    bus.publish (slotIndex, role, prio, group, linked, myLevels, nowMs);

    // 3) CALCUL des coupes cibles
    BandArray target {};
    if (pAutoOn->load() > 0.5f && linked)
    {
        TrackSnapshot me;
        me.role = role; me.priority = prio; me.levelDb = myLevels;
        const int numOthers = bus.snapshotOthers (slotIndex, group, nowMs, othersBuffer.data(), kMaxSlots);

        MaskingSettings st;
        st.strength = pAmount->load() / 100.0f;
        st.maxCutDb = pMaxCut->load();
        MaskingEngine::computeTargetCuts (me, othersBuffer.data(), numOthers, st, target);
    }

    // 4) LISSAGE attack/release (une fois par bloc)
    const float atk = 1.0f - std::exp (-(float) n / (float) (fs * pAttack->load() * 0.001));
    const float rel = 1.0f - std::exp (-(float) n / (float) (fs * pRelease->load() * 0.001));
    for (size_t b = 0; b < (size_t) kNumBands; ++b)
    {
        float& c = currentCutDb[b];
        c += (target[b] < c ? atk : rel) * (target[b] - c);   // plus négatif = plus de coupe = attaque
        autoCutDisplay[b].store (c, std::memory_order_relaxed);
    }

    // 5) MISE À JOUR des filtres puis FILTRAGE
    updateAutoFilters (fs);
    computeManualCoeffs (fs, manualEq.coeffs);
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        autoBank.process (ch, data, n);
        manualEq.process (ch, data, n);
    }

    // 6) GAIN DE SORTIE (lissé pour éviter les clics)
    outGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (pOutGain->load()));
    for (int i = 0; i < n; ++i)
    {
        const float g = outGainSmoothed.getNextValue();
        for (int ch = 0; ch < numCh; ++ch) buffer.getWritePointer (ch)[i] *= g;
    }
}

// ----------------------------------------------------------------------------
juce::AudioProcessorEditor* ClearMixAudioProcessor::createEditor() { return new ClearMixAudioProcessorEditor (*this); }

void ClearMixAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void ClearMixAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ClearMixAudioProcessor(); }
