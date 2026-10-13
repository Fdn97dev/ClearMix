// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (C) 2026 ClearMix contributors
#include "PluginEditor.h"
#include "Parameters.h"

using namespace cm;

namespace
{
    const juce::Colour kBg      { 0xff12141a };
    const juce::Colour kPanel   { 0xff1b1e27 };
    const juce::Colour kAccent  { 0xff2ee6c8 };   // turquoise : courbe finale / sélection
    const juce::Colour kCut     { 0xffff6a4d };   // orange-rouge : coupes automatiques
    const juce::Colour kGrid    { 0xff2b2f3b };
    const juce::Colour kText    { 0xffb7bdcc };

    constexpr float kMinF = 20.0f, kMaxF = 20000.0f;
}

// ============================================================================
//  LookAndFeel
// ============================================================================
ClearMixLookAndFeel::ClearMixLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, kText);
    setColour (juce::ComboBox::backgroundColourId, kPanel);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, kGrid);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::ToggleButton::textColourId, kText);
    setColour (juce::ToggleButton::tickColourId, kAccent);
}

void ClearMixLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path track, value;
    track.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (c.x, c.y, radius, radius, 0.0f, startAngle, angle, true);

    g.setColour (kGrid);
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (kAccent);
    g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (juce::Colours::white);
    g.drawLine (c.x + std::sin (angle) * radius * 0.35f, c.y - std::cos (angle) * radius * 0.35f,
                c.x + std::sin (angle) * radius * 0.85f, c.y - std::cos (angle) * radius * 0.85f, 2.0f);
}

// ============================================================================
//  EqDisplay
// ============================================================================
EqDisplay::EqDisplay (ClearMixAudioProcessor& p) : proc (p)
{
    startTimerHz (30);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> EqDisplay::plotArea() const { return getLocalBounds().toFloat().reduced (8.0f).withTrimmedLeft (22.0f).withTrimmedBottom (14.0f); }

float EqDisplay::freqToX (float f) const
{
    const auto a = plotArea();
    return a.getX() + a.getWidth() * std::log (f / kMinF) / std::log (kMaxF / kMinF);
}
float EqDisplay::xToFreq (float x) const
{
    const auto a = plotArea();
    const float t = juce::jlimit (0.0f, 1.0f, (x - a.getX()) / a.getWidth());
    return kMinF * std::pow (kMaxF / kMinF, t);
}
float EqDisplay::dbToY (float db) const { const auto a = plotArea(); return a.getCentreY() - (db / kCurveRangeDb) * a.getHeight() * 0.5f; }
float EqDisplay::yToDb (float y) const  { const auto a = plotArea(); return (a.getCentreY() - y) / (a.getHeight() * 0.5f) * kCurveRangeDb; }
float EqDisplay::levelToY (float db) const { const auto a = plotArea(); return juce::jmap (juce::jlimit (-90.0f, -10.0f, db), -90.0f, -10.0f, a.getBottom(), a.getY()); }

juce::Point<float> EqDisplay::handlePos (int i) const
{
    auto& s = proc.apvts;
    if (i == kNumManualBands)   // HPF : poignée en haut de l'écran
        return { freqToX (s.getRawParameterValue (id::hpf)->load()), plotArea().getY() + 9.0f };
    return { freqToX (s.getRawParameterValue (id::bandFreq (i))->load()),
             dbToY   (s.getRawParameterValue (id::bandGain (i))->load()) };
}

int EqDisplay::findHandle (juce::Point<float> p) const
{
    int best = -1; float bestD = 14.0f;
    for (int i = 0; i <= kNumManualBands; ++i)
    {
        const float d = handlePos (i).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

juce::Path EqDisplay::buildSpectrumPath (const float* levels, bool closeBottom) const
{
    const auto& layout = BandLayout::get();
    const auto a = plotArea();
    juce::Path p;
    for (int b = 0; b < kNumBands; ++b)
    {
        const float x = freqToX (layout.centre[(size_t) b]), y = levelToY (levels[b]);
        if (b == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    p = p.createPathWithRoundedCorners (10.0f);
    if (closeBottom)
    {
        p.lineTo (freqToX (layout.centre[kNumBands - 1]), a.getBottom());
        p.lineTo (freqToX (layout.centre[0]), a.getBottom());
        p.closeSubPath();
    }
    return p;
}

void EqDisplay::paint (juce::Graphics& g)
{
    const auto a = plotArea();
    const double fs = proc.getSampleRateForUi();

    g.setColour (kPanel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    // --- Grille ---
    g.setFont (10.0f);
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = freqToX (f);
        g.setColour (kGrid);
        g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        g.setColour (kText.withAlpha (0.6f));
        g.drawText (f >= 1000.f ? juce::String (f / 1000.f) + "k" : juce::String ((int) f),
                    (int) x - 20, (int) a.getBottom() + 1, 40, 12, juce::Justification::centred);
    }
    for (float db : { -12.f, -6.f, 0.f, 6.f, 12.f })
    {
        g.setColour (std::abs (db) < 0.01f ? kGrid.brighter (0.5f) : kGrid);
        g.drawHorizontalLine ((int) dbToY (db), a.getX(), a.getRight());
        g.setColour (kText.withAlpha (0.6f));
        g.drawText (juce::String ((int) db), 0, (int) dbToY (db) - 6, 20, 12, juce::Justification::centredRight);
    }

    // --- Spectres des autres pistes (traits fins colorés) ---
    auto& bus = SharedBus::get();
    const auto nowMs = (uint32_t) juce::Time::getMillisecondCounter();
    const int myGroup = (int) std::lround (proc.apvts.getRawParameterValue (id::group)->load());
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (s == proc.getSlotIndex()) continue;
        const auto& slot = bus.slot (s);
        if (! slot.active.load() || ! slot.linked.load() || slot.group.load() != myGroup) continue;
        if ((uint32_t) (nowMs - slot.lastUpdateMs.load()) > kSlotTimeoutMs) continue;

        float lv[kNumBands];
        for (size_t b = 0; b < (size_t) kNumBands; ++b) lv[b] = slot.levelDb[b].load();
        g.setColour (juce::Colour::fromHSV ((float) (s % 12) / 12.0f, 0.55f, 0.95f, 0.55f));
        g.strokePath (buildSpectrumPath (lv, false), juce::PathStrokeType (1.2f));
    }

    // --- Spectre de cette piste (rempli) ---
    {
        float lv[kNumBands];
        for (size_t b = 0; b < (size_t) kNumBands; ++b) lv[b] = proc.getOwnLevelDisplay()[b].load();
        const auto fill = buildSpectrumPath (lv, true);
        g.setGradientFill (juce::ColourGradient (kAccent.withAlpha (0.35f), 0.0f, a.getY(),
                                                 kAccent.withAlpha (0.02f), 0.0f, a.getBottom(), false));
        g.fillPath (fill);
        g.setColour (kAccent.withAlpha (0.8f));
        g.strokePath (buildSpectrumPath (lv, false), juce::PathStrokeType (1.5f));
    }

    // --- Courbes de réponse ---
    constexpr int numPts = 300;
    std::array<BiquadCoeffs, kNumManualBands + 1> manual;
    proc.computeManualCoeffs (fs, manual);

    std::array<BiquadCoeffs, kNumBands> autoCoeffs;
    const auto& layout = BandLayout::get();
    for (int b = 0; b < kNumBands; ++b)
        autoCoeffs[(size_t) b] = biquad::peaking (fs, layout.centre[(size_t) b],
                                                  proc.getAutoCutDisplay()[(size_t) b].load(), kBankQ);

    juce::Path autoPath, manualPath, totalPath, autoFill;
    const float y0 = dbToY (0.0f);
    for (int i = 0; i < numPts; ++i)
    {
        const float x = a.getX() + a.getWidth() * (float) i / (numPts - 1);
        const double f = xToFreq (x);
        double dbAuto = 0.0, dbMan = 0.0;
        for (const auto& c : autoCoeffs) dbAuto += biquad::magnitudeDb (c, f, fs);
        for (const auto& c : manual)     dbMan  += biquad::magnitudeDb (c, f, fs);

        const float ya = dbToY ((float) dbAuto), ym = dbToY ((float) dbMan), yt = dbToY ((float) (dbAuto + dbMan));
        if (i == 0) { autoPath.startNewSubPath (x, ya); manualPath.startNewSubPath (x, ym); totalPath.startNewSubPath (x, yt); autoFill.startNewSubPath (x, y0); autoFill.lineTo (x, ya); }
        else        { autoPath.lineTo (x, ya); manualPath.lineTo (x, ym); totalPath.lineTo (x, yt); autoFill.lineTo (x, ya); }
    }
    autoFill.lineTo (a.getRight(), y0);
    autoFill.closeSubPath();

    g.setColour (kCut.withAlpha (0.28f));
    g.fillPath (autoFill);
    g.setColour (kCut);
    g.strokePath (autoPath, juce::PathStrokeType (1.5f));
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.strokePath (manualPath, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::white);
    g.strokePath (totalPath, juce::PathStrokeType (2.2f));

    // --- Poignées ---
    for (int i = 0; i <= kNumManualBands; ++i)
    {
        const auto p = handlePos (i);
        const bool active = (i == dragging || i == hover);
        g.setColour (active ? kAccent : kPanel.brighter (0.6f));
        g.fillEllipse (p.x - 7.0f, p.y - 7.0f, 14.0f, 14.0f);
        g.setColour (kBg);
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText (i == kNumManualBands ? "HP" : juce::String (i + 1),
                    juce::Rectangle<float> (p.x - 8.0f, p.y - 7.0f, 16.0f, 14.0f).toNearestInt(), juce::Justification::centred);
    }

    // --- Légende ---
    g.setFont (10.0f);
    g.setColour (kCut);        g.drawText ("Coupes auto", (int) a.getRight() - 230, (int) a.getY() + 2, 80, 12, juce::Justification::right);
    g.setColour (kAccent);     g.drawText ("Cette piste",  (int) a.getRight() - 150, (int) a.getY() + 2, 70, 12, juce::Justification::right);
    g.setColour (juce::Colours::white); g.drawText ("Courbe finale", (int) a.getRight() - 80, (int) a.getY() + 2, 76, 12, juce::Justification::right);
}

void EqDisplay::setParam (const juce::String& paramId, float plain)
{
    if (auto* p = proc.apvts.getParameter (paramId))
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

void EqDisplay::beginGesture (int h)
{
    if (h == kNumManualBands) { proc.apvts.getParameter (id::hpf)->beginChangeGesture(); return; }
    proc.apvts.getParameter (id::bandFreq (h))->beginChangeGesture();
    proc.apvts.getParameter (id::bandGain (h))->beginChangeGesture();
}
void EqDisplay::endGesture (int h)
{
    if (h == kNumManualBands) { proc.apvts.getParameter (id::hpf)->endChangeGesture(); return; }
    proc.apvts.getParameter (id::bandFreq (h))->endChangeGesture();
    proc.apvts.getParameter (id::bandGain (h))->endChangeGesture();
}

void EqDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragging = findHandle (e.position);
    if (dragging >= 0) beginGesture (dragging);
}

void EqDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging < 0) return;
    const float f = juce::jlimit (kMinF, kMaxF, xToFreq (e.position.x));
    if (dragging == kNumManualBands) { setParam (id::hpf, juce::jmin (f, 500.0f)); return; }
    setParam (id::bandFreq (dragging), f);
    setParam (id::bandGain (dragging), juce::jlimit (-18.0f, 18.0f, yToDb (e.position.y)));
}

void EqDisplay::mouseUp (const juce::MouseEvent& e)
{
    if (dragging >= 0) endGesture (dragging);
    dragging = -1;
    hover = findHandle (e.position);
}

void EqDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = findHandle (e.position);
    if (h < 0) return;
    beginGesture (h);
    setParam (h == kNumManualBands ? id::hpf : id::bandGain (h), h == kNumManualBands ? 20.0f : 0.0f);
    endGesture (h);
}

void EqDisplay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = findHandle (e.position);
    if (h < 0 || h == kNumManualBands) return;
    const float currentQ = proc.apvts.getRawParameterValue (id::bandQ (h))->load();
    const float factor = std::pow (1.25f, w.deltaY * 4.0f);
    auto* param = proc.apvts.getParameter (id::bandQ (h));
    param->beginChangeGesture();
    setParam (id::bandQ (h), juce::jlimit (0.1f, 10.0f, currentQ * factor));
    param->endChangeGesture();
}

// ============================================================================
//  Éditeur principal
// ============================================================================
void ClearMixAudioProcessorEditor::setupKnob (Knob& k, const juce::String& text, const juce::String& paramId)
{
    k.slider.setLookAndFeel (&lnf);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
}

ClearMixAudioProcessorEditor::ClearMixAudioProcessorEditor (ClearMixAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p)
{
    setLookAndFeel (&lnf);
    setSize (940, 560);
    setResizable (true, true);
    setResizeLimits (760, 460, 1600, 1000);

    title.setText ("ClearMix", juce::dontSendNotification);
    title.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, kAccent);
    addAndMakeVisible (title);

    status.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (status);

    juce::StringArray names;
    for (const auto& r : roleTable()) names.add (r.name);
    roleBox.addItemList (names, 1);
    for (int i = 1; i <= 8; ++i) groupBox.addItem ("Groupe " + juce::String (i), i);
    addAndMakeVisible (roleBox);
    addAndMakeVisible (groupBox);
    addAndMakeVisible (linkButton);
    addAndMakeVisible (autoButton);

    roleAtt  = std::make_unique<CbAtt> (proc.apvts, id::role,  roleBox);
    groupAtt = std::make_unique<CbAtt> (proc.apvts, id::group, groupBox);
    linkAtt  = std::make_unique<BtAtt> (proc.apvts, id::link,   linkButton);
    autoAtt  = std::make_unique<BtAtt> (proc.apvts, id::autoOn, autoButton);

    setupKnob (kPriority, "Priorite", id::priority);
    setupKnob (kAmount,   "Amount",   id::amount);
    setupKnob (kMaxCut,   "Max Cut",  id::maxCut);
    setupKnob (kAttack,   "Attack",   id::attack);
    setupKnob (kRelease,  "Release",  id::release);
    setupKnob (kOutput,   "Output",   id::outGain);

    addAndMakeVisible (display);
    startTimerHz (5);
    timerCallback();
}

ClearMixAudioProcessorEditor::~ClearMixAudioProcessorEditor()
{
    stopTimer();
    for (auto* k : { &kPriority, &kAmount, &kMaxCut, &kAttack, &kRelease, &kOutput })
        k->slider.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void ClearMixAudioProcessorEditor::timerCallback()
{
    // Compte les autres pistes actuellement liées à mon groupe.
    auto& bus = SharedBus::get();
    const auto nowMs = (uint32_t) juce::Time::getMillisecondCounter();
    const int group = (int) std::lround (proc.apvts.getRawParameterValue (id::group)->load());
    int count = 0;
    for (int s = 0; s < kMaxSlots; ++s)
    {
        if (s == proc.getSlotIndex()) continue;
        const auto& sl = bus.slot (s);
        if (sl.active.load() && sl.linked.load() && sl.group.load() == group
            && (uint32_t) (nowMs - sl.lastUpdateMs.load()) <= kSlotTimeoutMs)
            ++count;
    }
    status.setText (count == 0 ? "Aucune autre piste detectee (lancez la lecture)"
                               : juce::String (count) + " autre(s) piste(s) liee(s)",
                    juce::dontSendNotification);
}

void ClearMixAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (kBg); }

void ClearMixAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto header = area.removeFromTop (34);
    title.setBounds (header.removeFromLeft (120));
    roleBox.setBounds (header.removeFromLeft (210).reduced (0, 3));
    header.removeFromLeft (8);
    groupBox.setBounds (header.removeFromLeft (110).reduced (0, 3));
    header.removeFromLeft (8);
    linkButton.setBounds (header.removeFromLeft (70));
    autoButton.setBounds (header.removeFromLeft (90));
    status.setBounds (header);

    area.removeFromTop (8);
    auto knobs = area.removeFromBottom (110);
    area.removeFromBottom (8);
    display.setBounds (area);

    const int w = knobs.getWidth() / 6;
    for (auto* k : { &kPriority, &kAmount, &kMaxCut, &kAttack, &kRelease, &kOutput })
    {
        auto cell = knobs.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell);
    }
}
