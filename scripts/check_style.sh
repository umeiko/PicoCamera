#!/bin/bash
# Format src/ and examples/ with astyle.
#
# CI installs astyle from Ubuntu apt (currently 3.1); the scripts are also
# verified against astyle 3.6 (Homebrew), which produces identical output
# for this tree.
#
# The option files are copies of arduino-pico's tests/astyle_core.conf and
# tests/astyle_examples.conf, minus remove-comment-prefix so Doxygen blocks
# and aligned trailing comments stay intact.
#
# Sensor register/settings tables (src/sensors/*_regs.h, *_settings.h) are
# verbatim ports from esp32-camera and are excluded to keep future syncs
# diffable.
#
# Usage:
#   scripts/check_style.sh           format in place
#   scripts/check_style.sh --check   format, then fail if git diff is non-empty
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

find src -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" \) \
    ! -name "*_regs.h" ! -name "*_settings.h" \
    -exec astyle --suffix=none --options=scripts/astyle_core.conf {} \;

find examples -type f -name "*.ino" \
    -exec astyle --suffix=none --options=scripts/astyle_examples.conf {} \;

if [ "${1:-}" = "--check" ]; then
    git diff --exit-code
fi
