#pragma once

#include <JuceHeader.h>

class ChromeKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChromeKnobLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

private:
    juce::Image knobImage;
};

class LabelledKnob : public juce::Component
{
public:
    LabelledKnob (const juce::String& labelText, bool large = false);

    juce::Slider& slider() noexcept { return knob; }
    void setReadoutFunction (std::function<juce::String(double)> fn);
    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    juce::String label;
    juce::Slider knob;
    juce::Label readout;
    ChromeKnobLookAndFeel look;
    bool isLarge = false;
    std::function<juce::String(double)> readoutFunction;
};
