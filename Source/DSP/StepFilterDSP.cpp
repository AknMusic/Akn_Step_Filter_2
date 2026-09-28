#include "StepFilterDSP.h"
#include <cmath>

namespace
{
constexpr double twoPi = juce::MathConstants<double>::twoPi;

// Slow -> fast. This deliberately mirrors the 22-position rack control.
constexpr std::array<double, 22> syncBeats
{
    32.0, 16.0, 8.0, 4.0,
    3.0, 2.0, 4.0 / 3.0,
    1.5, 1.0, 2.0 / 3.0,
    0.75, 0.5, 1.0 / 3.0,
    0.375, 0.25, 1.0 / 6.0,
    0.1875, 0.125, 1.0 / 12.0,
    0.09375, 0.0625, 1.0 / 24.0
};

const std::array<juce::String, 22> syncLabels
{
    "8 bars", "4 bars", "2 bars", "1 bar",
    "1/2.", "1/2", "1/2T",
    "1/4.", "1/4", "1/4T",
    "1/8.", "1/8", "1/8T",
    "1/16.", "1/16", "1/16T",
    "1/32.", "1/32", "1/32T",
    "1/64.", "1/64", "1/64T"
};

inline double wrap01 (double x) noexcept
{
    x -= std::floor (x);
    return x < 0.0 ? x + 1.0 : x;
}
}

void StepFilterDSP::TptSvf::reset() noexcept
{
    ic1eq = 0.0f;
    ic2eq = 0.0f;
}

void StepFilterDSP::TptSvf::set (float cutoffHz, float q, double sr) noexcept
{
    const auto safeCutoff = juce::jlimit (10.0f, (float) (sr * 0.45), cutoffHz);
    const auto safeQ = juce::jlimit (0.35f, 24.0f, q);
    const float g = std::tan (juce::MathConstants<float>::pi * safeCutoff / (float) sr);
    k = 1.0f / safeQ;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

StepFilterDSP::Outputs StepFilterDSP::TptSvf::process (float x) noexcept
{
    const float v3 = x - ic2eq;
    const float v1 = a1 * ic1eq + a2 * v3;
    const float v2 = ic2eq + a2 * ic1eq + a3 * v3;

    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;

    Outputs o;
    o.low = v2;
    o.band = v1;
    o.high = x - k * v1 - v2;
    o.notch = o.low + o.high;
    return o;
}

void StepFilterDSP::prepare (double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = newSampleRate;
    preparedChannels = juce::jlimit (1, 2, numChannels);
    dryBuffer.setSize (preparedChannels, maxBlockSize, false, false, true);

    for (auto& ch : channelStates)
    {
        ch.stage1.reset();
        ch.stage2.reset();
    }

    const auto initialCutoff = frequencyStepsHz[(size_t) juce::jlimit (0, 9, parameters.frequencyIndex)];
    dryWetSmooth.reset (sampleRate, 0.015);
    resonanceSmooth.reset (sampleRate, 0.020);
    driveSmooth.reset (sampleRate, 0.020);
    cutoffSmooth.reset (sampleRate, 0.025);
    morphSmooth.reset (sampleRate, 0.020);
    amountSmooth.reset (sampleRate, 0.020);
    outputGainSmooth.reset (sampleRate, 0.020);

    dryWetSmooth.setCurrentAndTargetValue (parameters.dryWet);
    resonanceSmooth.setCurrentAndTargetValue (parameters.resonance);
    driveSmooth.setCurrentAndTargetValue (parameters.driveDb);
    cutoffSmooth.setCurrentAndTargetValue (initialCutoff);
    morphSmooth.setCurrentAndTargetValue (parameters.morph);
    amountSmooth.setCurrentAndTargetValue (parameters.lfoAmountSemitones);
    outputGainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (parameters.outputDb, -70.0f));

    lastFrequencyIndex = parameters.frequencyIndex;
    freeCyclePosition = 0.0;
}

void StepFilterDSP::reset()
{
    for (auto& ch : channelStates)
    {
        ch.stage1.reset();
        ch.stage2.reset();
    }
    freeCyclePosition = 0.0;
}

void StepFilterDSP::setParameters (const Parameters& p)
{
    parameters = p;
    dryWetSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, p.dryWet));
    resonanceSmooth.setTargetValue (juce::jlimit (0.0f, 1.25f, p.resonance));
    driveSmooth.setTargetValue (juce::jlimit (0.0f, 10.0f, p.driveDb));
    morphSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, p.morph));
    amountSmooth.setTargetValue (juce::jlimit (0.0f, 30.0f, p.lfoAmountSemitones));
    outputGainSmooth.setTargetValue (juce::Decibels::decibelsToGain (p.outputDb, -70.0f));

    const auto idx = juce::jlimit (0, 9, p.frequencyIndex);
    if (idx != lastFrequencyIndex)
    {
        cutoffSmooth.setTargetValue (frequencyStepsHz[(size_t) idx]);
        lastFrequencyIndex = idx;
    }
}

float StepFilterDSP::selectOutput (const Outputs& o, int type, float morph) noexcept
{
    switch (juce::jlimit (0, 3, type))
    {
        case 0: return o.high;
        case 1: return o.band * 1.35f; // "Bell" / band-pass presentation from the rack
        case 2: return o.notch;
        default: break;
    }

    // Ableton's classic Morph path: LP -> BP -> HP -> Notch -> LP.
    const float p = juce::jlimit (0.0f, 1.0f, morph) * 4.0f;
    const int segment = juce::jlimit (0, 3, (int) std::floor (std::min (p, 3.999999f)));
    const float t = p - (float) segment;

    const float modes[5] { o.low, o.band * 1.35f, o.high, o.notch, o.low };
    const float a = std::cos (t * juce::MathConstants<float>::halfPi);
    const float b = std::sin (t * juce::MathConstants<float>::halfPi);
    return modes[segment] * a + modes[segment + 1] * b;
}

float StepFilterDSP::softDrive (float x, float driveDb) noexcept
{
    if (driveDb <= 0.001f)
        return x;

    const float gain = juce::Decibels::decibelsToGain (driveDb);
    const float norm = std::tanh (gain);
    return norm > 0.0001f ? std::tanh (x * gain) / norm : x;
}

float StepFilterDSP::pseudoRandomBipolar (std::int64_t cycle, int channel) noexcept
{
    std::uint64_t x = (std::uint64_t) cycle;
    x ^= 0x9e3779b97f4a7c15ULL + (std::uint64_t) (channel + 1) * 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    const double unit = (double) (x & 0xFFFFFFu) / (double) 0xFFFFFFu;
    return (float) (unit * 2.0 - 1.0);
}

float StepFilterDSP::lfoValue (int waveform, double cycles, int channel) noexcept
{
    const auto whole = (std::int64_t) std::floor (cycles);
    const float p = (float) wrap01 (cycles);

    switch (juce::jlimit (0, 6, waveform))
    {
        case 0: return std::sin ((float) (twoPi * p));
        case 1: return p < 0.5f ? 1.0f : -1.0f;
        case 2: return 1.0f - 4.0f * std::abs (p - 0.5f);
        case 3: return 2.0f * p - 1.0f;
        case 4: return 1.0f - 2.0f * p;
        case 5: // stereo S&H, independent L/R
            return pseudoRandomBipolar (whole, channel);
        case 6: // mono S&H
            return pseudoRandomBipolar (whole, 0);
        default: return 0.0f;
    }
}

double StepFilterDSP::getSyncBeatsPerCycle (int index) noexcept
{
    return syncBeats[(size_t) juce::jlimit (0, 21, index)];
}

juce::String StepFilterDSP::getSyncRateLabel (int index)
{
    return syncLabels[(size_t) juce::jlimit (0, 21, index)];
}

float StepFilterDSP::getFreeRateHz (float normalized)
{
    const float n = juce::jlimit (0.0f, 1.0f, normalized);
    return 0.01f * std::pow (1000.0f, n); // exactly 0.01 -> 10 Hz
}

void StepFilterDSP::process (juce::AudioBuffer<float>& buffer, juce::AudioPlayHead* playHead)
{
    const int channels = juce::jmin (preparedChannels, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    for (int ch = 0; ch < channels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, samples);

    double bpm = 120.0;
    double ppq = 0.0;
    bool transportPlaying = false;

    if (playHead != nullptr)
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto hostBpm = pos->getBpm()) bpm = *hostBpm;
            if (auto hostPpq = pos->getPpqPosition()) ppq = *hostPpq;
            transportPlaying = pos->getIsPlaying();
        }
    }

    const int syncIndex = juce::jlimit (0, 21, (int) std::lround (parameters.lfoRateNormalized * 21.0f));
    const double beatsPerCycle = getSyncBeatsPerCycle (syncIndex);
    const double syncCyclesPerSample = (bpm / 60.0) / (sampleRate * beatsPerCycle);
    const double freeCyclesPerSample = (double) getFreeRateHz (parameters.lfoRateNormalized) / sampleRate;
    const double startSyncedCycles = ppq / beatsPerCycle;
    const double phaseOffset = juce::jlimit (0.0f, 360.0f, parameters.lfoPhaseDegrees) / 360.0;

    for (int i = 0; i < samples; ++i)
    {
        const float wet = dryWetSmooth.getNextValue();
        const float resonance = resonanceSmooth.getNextValue();
        const float driveDb = driveSmooth.getNextValue();
        const float baseCutoff = cutoffSmooth.getNextValue();
        const float morph = morphSmooth.getNextValue();
        const float amount = amountSmooth.getNextValue();
        const float outGain = outputGainSmooth.getNextValue();

        // Resonance parameter in the rack is 0..1.25. Map it to a musical SVF Q curve.
        const float qNorm = resonance / 1.25f;
        const float q = 0.5f * std::pow (40.0f, juce::jlimit (0.0f, 1.0f, qNorm));

        const double baseCycles = parameters.lfoSync && transportPlaying
                                ? startSyncedCycles + syncCyclesPerSample * (double) i
                                : freeCyclePosition;

        for (int ch = 0; ch < channels; ++ch)
        {
            const double channelCycles = baseCycles + (ch == 1 ? phaseOffset : 0.0);
            const float lfo = lfoValue (parameters.lfoWave, channelCycles, ch);
            const float semitoneOffset = lfo * amount;
            const float cutoff = juce::jlimit (20.0f,
                                               (float) (sampleRate * 0.45),
                                               baseCutoff * std::pow (2.0f, semitoneOffset / 12.0f));

            auto& state = channelStates[(size_t) ch];
            state.stage1.set (cutoff, q, sampleRate);
            state.stage2.set (cutoff, q, sampleRate);

            float x = buffer.getSample (ch, i);
            x = softDrive (x, driveDb);

            const auto first = state.stage1.process (x);
            const float y1 = selectOutput (first, parameters.filterType, morph);
            const auto second = state.stage2.process (y1);
            const float filtered = selectOutput (second, parameters.filterType, morph);

            // Equal-power rack-style dry/wet crossfade.
            const float dryGain = std::cos (wet * juce::MathConstants<float>::halfPi);
            const float wetGain = std::sin (wet * juce::MathConstants<float>::halfPi);
            const float dry = dryBuffer.getSample (ch, i);
            buffer.setSample (ch, i, (dry * dryGain + filtered * wetGain) * outGain);
        }

        if (! (parameters.lfoSync && transportPlaying))
        {
            freeCyclePosition += parameters.lfoSync ? syncCyclesPerSample : freeCyclesPerSample;
            if (freeCyclePosition > 1000000.0)
                freeCyclePosition -= std::floor (freeCyclePosition);
        }
    }
}
