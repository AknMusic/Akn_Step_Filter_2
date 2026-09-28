#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs
{
constexpr auto dryWet = "dryWet";
constexpr auto resonance = "resonance";
constexpr auto drive = "drive";
constexpr auto frequency = "frequency";
constexpr auto output = "output";
constexpr auto filterType = "filterType";
constexpr auto morph = "morph";
constexpr auto lfoAmount = "lfoAmount";
constexpr auto lfoRate = "lfoRate";
constexpr auto lfoSync = "lfoSync";
constexpr auto lfoWave = "lfoWave";
constexpr auto lfoPhase = "lfoPhase";
}

AKNStepFilterAudioProcessor::AKNStepFilterAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AKNStepFilterAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::dryWet, 1 }, "Dry / Wet",
                                      juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                      (90.0f / 127.0f) * 100.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::resonance, 1 }, "Resonance",
                                      juce::NormalisableRange<float> (0.0f, 125.0f, 0.01f),
                                      (59.0f / 127.0f) * 125.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::drive, 1 }, "Drive",
                                      juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f),
                                      (54.0f / 127.0f) * 10.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("dB")));

    juce::StringArray freqs { "70 Hz", "100 Hz", "150 Hz", "250 Hz", "500 Hz",
                              "1 kHz", "2 kHz", "3 kHz", "5 kHz", "7.5 kHz" };
    layout.add (std::make_unique<APC> (juce::ParameterID { IDs::frequency, 1 }, "Frequency", freqs, 6));

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::output, 1 }, "Volume",
                                      juce::NormalisableRange<float> (-70.0f, 6.0f, 0.01f),
                                      6.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("dB")));

    juce::StringArray types { "High Pass", "Bell / Band Pass", "Notch", "Morph" };
    layout.add (std::make_unique<APC> (juce::ParameterID { IDs::filterType, 1 }, "Filter Type", types, 3));

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::morph, 1 }, "Morph",
                                      juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                      100.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::lfoAmount, 1 }, "LFO Amount",
                                      juce::NormalisableRange<float> (0.0f, 30.0f, 0.01f),
                                      (73.0f / 127.0f) * 30.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("st")));

    // 0..1 is transformed exponentially to 0.01..10 Hz when Free, or to 22 musical steps when Sync.
    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::lfoRate, 1 }, "LFO Rate",
                                      juce::NormalisableRange<float> (0.0f, 1.0f, 0.0001f),
                                      75.0f / 127.0f));

    layout.add (std::make_unique<APB> (juce::ParameterID { IDs::lfoSync, 1 }, "LFO Sync / Free", false));

    juce::StringArray waves { "Sine", "Square", "Triangle", "Saw Up", "Saw Down", "S&H Stereo", "S&H Mono" };
    layout.add (std::make_unique<APC> (juce::ParameterID { IDs::lfoWave, 1 }, "LFO Wave", waves, 0));

    layout.add (std::make_unique<APF> (juce::ParameterID { IDs::lfoPhase, 1 }, "LFO Phase",
                                      juce::NormalisableRange<float> (0.0f, 360.0f, 0.1f),
                                      0.0f,
                                      juce::AudioParameterFloatAttributes().withLabel ("deg")));

    return layout;
}

void AKNStepFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp.setParameters (getCurrentDspParameters());
    dsp.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void AKNStepFilterAudioProcessor::releaseResources() {}

bool AKNStepFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out)
        return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

StepFilterDSP::Parameters AKNStepFilterAudioProcessor::getCurrentDspParameters() const
{
    StepFilterDSP::Parameters p;
    p.dryWet = apvts.getRawParameterValue (IDs::dryWet)->load() / 100.0f;
    p.resonance = apvts.getRawParameterValue (IDs::resonance)->load() / 100.0f;
    p.driveDb = apvts.getRawParameterValue (IDs::drive)->load();
    p.frequencyIndex = (int) std::lround (apvts.getRawParameterValue (IDs::frequency)->load());
    p.outputDb = apvts.getRawParameterValue (IDs::output)->load();
    p.filterType = (int) std::lround (apvts.getRawParameterValue (IDs::filterType)->load());
    p.morph = apvts.getRawParameterValue (IDs::morph)->load() / 100.0f;
    p.lfoAmountSemitones = apvts.getRawParameterValue (IDs::lfoAmount)->load();
    p.lfoRateNormalized = apvts.getRawParameterValue (IDs::lfoRate)->load();
    p.lfoSync = apvts.getRawParameterValue (IDs::lfoSync)->load() >= 0.5f;
    p.lfoWave = (int) std::lround (apvts.getRawParameterValue (IDs::lfoWave)->load());
    p.lfoPhaseDegrees = apvts.getRawParameterValue (IDs::lfoPhase)->load();
    return p;
}

void AKNStepFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    dsp.setParameters (getCurrentDspParameters());
    dsp.process (buffer, getPlayHead());
}

juce::AudioProcessorEditor* AKNStepFilterAudioProcessor::createEditor()
{
    return new AKNStepFilterAudioProcessorEditor (*this);
}

void AKNStepFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AKNStepFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AKNStepFilterAudioProcessor();
}
