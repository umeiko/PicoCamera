#!/bin/bash
# Compile every example for RP2040, RP2350 and RP2350-RISCV with the warning
# flags the arduino-pico core CI uses (warnings are errors there).
#
# Needs: pio (PlatformIO Core) on PATH, or set PIO=/path/to/pio.
set -u
PIO="${PIO:-pio}"
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$HERE")"
PROJ="$HERE/ci"
ENVS="rp2040 rp2350 rp2350-riscv"
EXAMPLES="CameraCaptureJPEG CameraCaptureRGB565 CameraSerialInfo push_image_to_python"

fail=0
for ex in $EXAMPLES; do
    rm -rf "$PROJ/src"
    mkdir -p "$PROJ/src"
    cp "$ROOT/examples/$ex/$ex.ino" "$PROJ/src/$ex.ino"
    for env in $ENVS; do
        echo "=== $ex [$env] ==="
        if ! $PIO run --project-dir "$PROJ" -e "$env" -s; then
            echo "FAIL: $ex [$env]"
            fail=1
        fi
    done
done

# camera_render_to_tft is .test.skip'd in arduino-pico CI (needs TFT_eSPI);
# here just verify it compiles, without the strict -Werror flags.
ex=camera_render_to_tft
rm -rf "$PROJ/src"
mkdir -p "$PROJ/src"
cp "$ROOT/examples/$ex/$ex.ino" "$PROJ/src/$ex.ino"
echo "=== $ex [tft-rp2040] ==="
if ! $PIO run --project-dir "$PROJ" -e tft-rp2040 -s; then
    echo "FAIL: $ex [tft-rp2040]"
    fail=1
fi
rm -rf "$PROJ/src"

if [ $fail -eq 0 ]; then
    echo "ALL BUILDS PASSED"
else
    echo "SOME BUILDS FAILED"
fi
exit $fail
