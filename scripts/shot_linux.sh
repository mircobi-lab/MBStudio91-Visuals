#!/bin/bash
# Avvia lo Standalone sotto Xvfb con il segnale di prova e salva uno screenshot (solo per i test su Linux).
# uso: shot_linux.sh OUT.png [argomenti del plugin: --preset-a N --preset-b N --palette N --pixel N --xf V ...]
OUT="$1"; shift
BIN="$(dirname "$0")/../build/MBStudio91Visuals_artefacts/Release/Standalone/MBStudio91 Visuals"
export LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=3.2 MESA_GLSL_VERSION_OVERRIDE=150
xvfb-run -a -s "-screen 0 1400x900x24" bash -c "\"$BIN\" --test-tone 1 $* & P=\$!; sleep ${SHOT_WAIT:-14}; import -window root \"$OUT\"; kill \$P; wait \$P 2>/dev/null" 2>&1 | grep -v "ALSA lib"
