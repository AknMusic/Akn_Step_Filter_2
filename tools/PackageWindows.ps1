param(
    [string]$BuildDir = "build",
    [string]$OutDir = "dist"
)
$ErrorActionPreference = "Stop"
$bundle = Join-Path $BuildDir "AKNStepFilter_artefacts/Release/VST3/AKN Step Filter.vst3"
if (!(Test-Path $bundle)) { throw "Missing VST3 bundle: $bundle" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$archive = Join-Path $OutDir "AKN-Step-Filter-Windows-x64.zip"
if (Test-Path $archive) { Remove-Item $archive -Force }
Compress-Archive -Path $bundle -DestinationPath $archive -CompressionLevel Optimal
Write-Host $archive
