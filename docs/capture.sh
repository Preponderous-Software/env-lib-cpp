#!/usr/bin/env bash
# Builds docs/demo.cpp against the library sources and renders its console
# output, frame by frame, into docs/images/random-walk.gif.
# Requires g++ and Python 3 with Pillow and the DejaVu Sans Mono font.
# Usage (from the repository root): bash docs/capture.sh [python]
set -euo pipefail
PY="${1:-python3}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
g++ -std=c++17 docs/demo.cpp src/entity.cpp src/environment.cpp src/grid.cpp src/location.cpp -o "$TMP/demo"
"$TMP/demo" > "$TMP/out.txt"
"$PY" - "$TMP/out.txt" docs/images/random-walk.gif <<'PYEOF'
import sys
from PIL import Image, ImageDraw, ImageFont

text = open(sys.argv[1]).read()
blocks = [b for b in text.split("step ")[1:]]
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20)
bg, fg, dim, hit = (13, 17, 23), (201, 209, 217), (110, 118, 129), (126, 231, 135)
frames = []
for block in blocks:
    lines = block.rstrip("\n").split("\n")
    header, rows = "step " + lines[0], [l for l in lines[1:] if l.strip()]
    w = 24 + 12 * max(len(r) for r in rows) + 24
    img = Image.new("RGB", (w, 24 + 30 * (len(rows) + 2) + 6), bg)
    d = ImageDraw.Draw(img)
    d.text((24, 16), "$ ./demo   # " + header, font=font, fill=dim)
    for i, row in enumerate(rows):
        x = 24
        for ch in row:
            d.text((x, 16 + 30 * (i + 2)), ch, font=font, fill=hit if ch == "x" else fg)
            x += 12
    frames.append(img)
frames[0].save(sys.argv[2], save_all=True, append_images=frames[1:], duration=500, loop=0)
print(f"wrote {sys.argv[2]} ({len(frames)} frames)")
PYEOF
