cmake_minimum_required(VERSION 3.22)

project(AKN_Step_Filter VERSION 1.0.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# Pinned so Windows and macOS builds use the same JUCE revision.
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(AKNStepFilterAssets
    SOURCES
        Assets/aksf-panel.png
        Assets/aksf-knob.png
)

juce_add_plugin(AKNStepFilter
    COMPANY_NAME "Akuen"
    BUNDLE_ID "com.akuen.stepfilter"
    PLUGIN_MANUFACTURER_CODE Akun
    PLUGIN_CODE Aksf
    FORMATS VST3
    PRODUCT_NAME "AKN Step Filter"
    PLUGIN_NAME "AKN Step Filter"
    DESCRIPTION "Akuen Step Filter"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Filter
)

# Generate JuceHeader.h for source files that include <JuceHeader.h>.
# juce_add_plugin() does not generate this umbrella header unless explicitly requested.
juce_generate_juce_header(AKNStepFilter)

target_sources(AKNStepFilter PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/StepFilterDSP.cpp
    Source/DSP/StepFilterDSP.h
    Source/UI/ChromeKnob.cpp
    Source/UI/ChromeKnob.h
)

target_compile_definitions(AKNStepFilter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AKNStepFilter PRIVATE
    AKNStepFilterAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(APPLE)
    set_target_properties(AKNStepFilter PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
endif()
