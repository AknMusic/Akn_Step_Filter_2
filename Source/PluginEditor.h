#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ChromeKnob.h"

class AKNStepFilterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit AKNStepFilterAudioProcessorEditor (AKNStepFilterAudioProcessor&);
    ~AKNStepFilterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureControls();
    void updateDynamicReadouts();
    void drawFrequencyTicks (juce::Graphics&, juce::Point<float> centre, float radius) const;

    AKNStepFilterAudioProcessor& processor;
    juce::Image panelImage;

    LabelledKnob frequency { "FREQUENCY", true };
    LabelledKnob resonance { "RESONANCE" };
    LabelledKnob drive { "DRIVE" };
    LabelledKnob morph { "MORPH" };
    LabelledKnob filterType { "FILTER TYPE" };
    LabelledKnob lfoAmount { "LFO AMOUNT" };
    LabelledKnob lfoRate { "LFO RATE" };
    LabelledKnob lfoWave { "LFO WAVE" };
    LabelledKnob lfoPhase { "LFO PHASE" };
    LabelledKnob dryWet { "DRY / WET" };
    LabelledKnob volume { "VOLUME" };

    juce::TextButton syncButton { "FREE" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> frequencyAttachment, resonanceAttachment, driveAttachment,
        morphAttachment, filterTypeAttachment, lfoAmountAttachment, lfoRateAttachment,
        lfoWaveAttachment, lfoPhaseAttachment, dryWetAttachment, volumeAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AKNStepFilterAudioProcessorEditor)
};
