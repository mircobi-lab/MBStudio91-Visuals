#!/bin/bash
# Compila MBStudio91 Visuals su Mac (richiede Xcode o Command Line Tools e CMake: brew install cmake)
# e installa VST3, AU e Standalone nelle cartelle dell'utente. Uso: ./scripts/build_mac.sh [--no-install]
set -e
cd "$(dirname "$0")/.."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel "$(sysctl -n hw.ncpu)"
ART="build/MBStudio91Visuals_artefacts/Release"
[ -d "$ART" ] || ART="build/MBStudio91Visuals_artefacts"
for b in "$ART/VST3/MBStudio91 Visuals.vst3" "$ART/AU/MBStudio91 Visuals.component" "$ART/Standalone/MBStudio91 Visuals.app"; do
  codesign --force --deep --sign - "$b"
done
if [ "$1" != "--no-install" ]; then
  mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components ~/Applications
  rm -rf ~/Library/Audio/Plug-Ins/VST3/"MBStudio91 Visuals.vst3" ~/Library/Audio/Plug-Ins/Components/"MBStudio91 Visuals.component" ~/Applications/"MBStudio91 Visuals.app"
  cp -R "$ART/VST3/MBStudio91 Visuals.vst3" ~/Library/Audio/Plug-Ins/VST3/
  cp -R "$ART/AU/MBStudio91 Visuals.component" ~/Library/Audio/Plug-Ins/Components/
  cp -R "$ART/Standalone/MBStudio91 Visuals.app" ~/Applications/
  # Audio Unit: fa rileggere la cache dei componenti
  killall -9 AudioComponentRegistrar 2>/dev/null || true
  echo "Installato. Riavvia Ableton Live; se l'AU non compare: auval -a | grep -i mbst"
fi
echo "Fatto: $ART"
