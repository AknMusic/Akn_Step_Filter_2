# AKN Step Filter VST3 — v1.0

Version VST3 native du rack **Akuen Step Filter** pour Windows et macOS.

## Formats produits

- Windows: VST3 x64
- macOS: VST3 Universal Binary (`arm64 + x86_64`)

Le projet utilise JUCE 8.0.15 via CMake/FetchContent. Les images du panel sont compilées dans le binaire avec `juce_add_binary_data`, donc **aucun PNG ou JS externe n'est nécessaire à l'installation du VST3**.

## Contrôles

- Dry / Wet
- Resonance: 0–125 %
- Drive: 0–10 dB
- Frequency: 70, 100, 150, 250, 500 Hz, 1, 2, 3, 5, 7.5 kHz
- Volume: -inf / -70 dB à +6 dB
- Filter Type: High Pass, Bell/Band Pass, Notch, Morph
- Morph: LP → BP → HP → Notch → LP
- LFO Amount: 0–30 demi-tons
- LFO Rate: 0.01–10 Hz ou 22 divisions synchronisées
- Free / Sync
- LFO Wave: Sine, Square, Triangle, Saw Up, Saw Down, S&H Stereo, S&H Mono
- LFO Phase: 0–360°

## Note DSP importante

Le rack Ableton d'origine utilise **Auto Filter**, dont le DSP interne est propriétaire. Le VST3 reproduit la structure et les plages du preset avec un filtre state-variable 24 dB et une saturation inspirée du comportement du circuit utilisé dans le rack. Le résultat vise la même fonction musicale et le même workflow, mais n'est pas un clone bit-perfect d'Auto Filter.

## Build local

### Windows

Prérequis: Visual Studio 2022 avec C++ Desktop + CMake.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target AKNStepFilter_VST3
```

Sortie typique:

`build/AKNStepFilter_artefacts/Release/VST3/AKN Step Filter.vst3`

Copier le bundle dans:

`C:\Program Files\Common Files\VST3\`

### macOS Universal

Prérequis: Xcode + CMake.

```bash
cmake -S . -B build-macos -G Xcode \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build-macos --config Release --target AKNStepFilter_VST3
```

Sortie typique:

`build-macos/AKNStepFilter_artefacts/Release/VST3/AKN Step Filter.vst3`

Copier dans:

`~/Library/Audio/Plug-Ins/VST3/`

Pour une distribution publique macOS, il faudra ensuite signer avec un certificat Developer ID et notariser le bundle.

## GitHub Actions

Les workflows dans `.github/workflows/` produisent automatiquement:

- `AKN-Step-Filter-Windows-x64.zip`
- `AKN-Step-Filter-macOS-Universal.zip`

Les fichiers sont disponibles dans les **Artifacts** du run GitHub Actions.
