#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>

class StepFilterDSP
{
public:
    struct Parameters
    {
        float dryWet = 90.0f / 127.0f;
        float resonance = (59.0f / 127.0f) * 1.25f;
        float driveDb = (54.0f / 127.0f) * 10.0f;
        int frequencyIndex = 6;
        float outputDb = 6.0f;
        int filterType = 3; // HP, BP/Bell, Notch, Morph
        float morph = 1.0f;
        float lfoAmountSemitones = (73.0f / 127.0f) * 30.0f;
        float lfoRateNormalized = 75.0f / 127.0f;
        bool lfoSync = false;
        int lfoWave = 0;
        float lfoPhaseDegrees = 0.0f;
    };

    void prepare (double newSampleRate, int maxBlockSize, int numChannels);
    void reset();
    void setParameters (const Parameters& newParameters);
    void process (juce::AudioBuffer<float>& buffer, juce::AudioPlayHead* playHead);

    static constexpr std::array<float, 10> frequencyStepsHz
    {
        70.0f, 100.0f, 150.0f, 250.0f, 500.0f,
        1000.0f, 2000.0f, 3000.0f, 5000.0f, 7500.0f
    };

    static juce::String getSyncRateLabel (int index);
    static float getFreeRateHz (float normalized);

private:
    struct Outputs
    {
        float low = 0.0f;
        float band = 0.0f;
        float high = 0.0f;
        float notch = 0.0f;
    };

    class TptSvf
    {
    public:
        void reset() noexcept;
        void set (float cutoffHz, float q, double sampleRate) noexcept;
        Outputs process (float x) noexcept;

    private:
        float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, k = 1.0f;
        float ic1eq = 0.0f, ic2eq = 0.0f;
    };

    struct ChannelState
    {
        TptSvf stage1, stage2;
    };

    static float selectOutput (const Outputs& o, int filterType, float morph) noexcept;
    static float softDrive (float x, float driveDb) noexcept;
    static float lfoValue (int waveform, double cycles, int channel) noexcept;
    static float pseudoRandomBipolar (std::int64_t cycle, int channel) noexcept;
    static double getSyncBeatsPerCycle (int index) noexcept;

    double sampleRate = 44100.0;
    int preparedChannels = 2;
    Parameters parameters;

    std::array<ChannelState, 2> channelStates;
    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryWetSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> resonanceSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> cutoffSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> morphSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmooth;

    double freeCyclePosition = 0.0;
    int lastFrequencyIndex = -1;
};
