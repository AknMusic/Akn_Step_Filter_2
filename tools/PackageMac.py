#!/usr/bin/env python3
from pathlib import Path
import argparse, subprocess, shutil

p = argparse.ArgumentParser()
p.add_argument('--build-dir', default='build-macos')
p.add_argument('--out-dir', default='dist')
a = p.parse_args()

build = Path(a.build_dir)
out = Path(a.out_dir)
out.mkdir(parents=True, exist_ok=True)

bundle = build / 'AKNStepFilter_artefacts' / 'Release' / 'VST3' / 'AKN Step Filter.vst3'
binary = bundle / 'Contents' / 'MacOS' / 'AKN Step Filter'
if not binary.exists():
    raise SystemExit(f'Missing binary: {binary}')

archs = subprocess.check_output(['xcrun', 'lipo', '-archs', str(binary)], text=True).strip().split()
required = {'arm64', 'x86_64'}
if not required.issubset(set(archs)):
    raise SystemExit(f'Expected universal binary {required}, got {archs}')

subprocess.check_call(['codesign', '--force', '--deep', '--sign', '-', str(bundle)])
archive = out / 'AKN-Step-Filter-macOS-Universal.zip'
if archive.exists(): archive.unlink()
subprocess.check_call(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(bundle), str(archive)])
print(archive)
