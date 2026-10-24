#!/usr/bin/env bash
# Records docs/media/*-3d.{png,gif} with the Strata frontend, headless.
# Needs the strata/ build (docs/STRATA.md), an EGL + GL 4.5 driver (Mesa
# llvmpipe works: LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe) and
# ImageMagick's `convert`.
# Usage (from the repository root): bash docs/capture-3d.sh [build-strata]
set -euo pipefail
BUILD="${1:-build-strata}"
DEMO="$BUILD/envlib_strata_demo"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/w" "$TMP/m" docs/media
MEADOW_VIEW=(--zoom 1.15 --yaw 20 --pitch 48)
"$DEMO" --world walkers --headless --ticks 5 --width 960 --height 600 --pick 3,6 --screenshot docs/media/walkers-3d.png
"$DEMO" --world meadow --headless --ticks 40 --width 960 --height 600 "${MEADOW_VIEW[@]}" --screenshot docs/media/meadow-3d.png
"$DEMO" --world walkers --headless --ticks 20 --frames-per-tick 6 --frames "$TMP/w" --width 640 --height 400
"$DEMO" --world meadow --headless --ticks 60 --frames-per-tick 3 --frames "$TMP/m" --width 640 --height 400 "${MEADOW_VIEW[@]}"
convert -delay 8 -loop 0 "$TMP"/w/frame-*.png -layers Optimize docs/media/walkers-3d.gif
convert -delay 8 -loop 0 "$TMP"/m/frame-*.png -layers Optimize docs/media/meadow-3d.gif
ls -l docs/media
