# Build fix 1.0.2

- Keeps `juce_generate_juce_header(AKNStepFilter)` from 1.0.1.
- Loads embedded PNG assets by their original filenames instead of assuming JUCE-generated C++ symbol names.
- Fixes JUCE 8 `AudioPlayHead::PositionInfo::getIsPlaying()` usage (`bool`, not optional).
