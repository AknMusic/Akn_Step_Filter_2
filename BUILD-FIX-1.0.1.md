# Build fix 1.0.1

Fixes the CI compile error:

`fatal error C1083: Cannot open include file: 'JuceHeader.h'`

The CMake project now calls:

```cmake
juce_generate_juce_header(AKNStepFilter)
```

immediately after `juce_add_plugin(...)`.

The workflows also use the pinned JUCE 8.0.15 checkout and Node-24-compatible GitHub actions.
