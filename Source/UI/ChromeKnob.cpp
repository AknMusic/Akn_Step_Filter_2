#include "ChromeKnob.h"
#include <BinaryData.h>

namespace
{
juce::Image loadEmbeddedImageByOriginalFilename (const juce::String& filename)
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String original (BinaryData::originalFilenames[i]);
        if (original == filename || juce::File (original).getFileName() == filename)
        {
            int size = 0;
            if (const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                return juce::ImageFileFormat::loadFrom (data, (size_t) size);
        }
    }

    return {};
}
}

ChromeKnobLookAndFeel::ChromeKnobLookAndFeel()
{
    knobImage = loadEmbeddedImageByOriginalFilename ("aksf-knob.png");
}

void ChromeKnobLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                               int x, int y, int width, int height,
                                               float sliderPos,
                                               float rotaryStartAngle,
                                               float rotaryEndAngle,
                                               juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                      .reduced (2.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto knobBounds = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());

    if (knobImage.isValid())
        g.drawImage (knobImage, knobBounds);
    else
    {
        g.setColour (juce::Colour (0xffb8b8b8));
        g.fillEllipse (knobBounds);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (knobBounds, 2.0f);
    }

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto centre = knobBounds.getCentre();
    const float radius = knobBounds.getWidth() * 0.37f;
    const auto p1 = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius * 0.52f);
    const auto p2 = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * radius;

    g.setColour (juce::Colour (0xfff5f4e9));
    g.drawLine ({ p1, p2 }, size > 80.0f ? 4.0f : 2.7f);
}

LabelledKnob::LabelledKnob (const juce::String& labelText, bool large)
    : label (labelText), isLarge (large)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.15f,
                              juce::MathConstants<float>::pi * 2.85f,
                              true);
    knob.setLookAndFeel (&look);
    knob.setMouseDragSensitivity (220);
    addAndMakeVisible (knob);

    readout.setJustificationType (juce::Justification::centred);
    readout.setColour (juce::Label::textColourId, juce::Colour (0xffaebdb3));
    readout.setFont (juce::FontOptions (isLarge ? 13.0f : 10.5f));
    addAndMakeVisible (readout);

    knob.onValueChange = [this]
    {
        if (readoutFunction)
            readout.setText (readoutFunction (knob.getValue()), juce::dontSendNotification);
    };
}

void LabelledKnob::setReadoutFunction (std::function<juce::String(double)> fn)
{
    readoutFunction = std::move (fn);
    if (readoutFunction)
        readout.setText (readoutFunction (knob.getValue()), juce::dontSendNotification);
}

void LabelledKnob::resized()
{
    const int labelH = isLarge ? 26 : 22;
    const int readoutH = isLarge ? 22 : 18;
    const int knobSize = juce::jmin (getWidth(), getHeight() - labelH - readoutH);
    const int knobX = (getWidth() - knobSize) / 2;
    knob.setBounds (knobX, 0, knobSize, knobSize);
    readout.setBounds (0, knobSize + labelH, getWidth(), readoutH);
}

void LabelledKnob::paint (juce::Graphics& g)
{
    const int labelH = isLarge ? 26 : 22;
    const int readoutH = isLarge ? 22 : 18;
    const int knobSize = juce::jmin (getWidth(), getHeight() - labelH - readoutH);
    g.setColour (juce::Colour (0xffefeee4));
    g.setFont (juce::FontOptions (isLarge ? 16.0f : 13.0f));
    g.drawText (label, 0, knobSize, getWidth(), labelH, juce::Justification::centred);
}
