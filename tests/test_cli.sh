#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PROGRAM="$ROOT/build/GnuChanWelcome"
VALID=$(mktemp)
INVALID=$(mktemp)
trap 'rm -f "$VALID" "$INVALID"' EXIT HUP INT TERM

printf '# first run state\nFirstRunComplete = true\n' > "$VALID"
printf 'not a setting\n' > "$INVALID"

"$PROGRAM" --version | grep -Fx 'GnuChanWelcome 0.2.0'
"$PROGRAM" --check-config --config "$VALID" >/dev/null
if "$PROGRAM" --check-config --config "$INVALID" >/dev/null 2>&1; then
    echo "malformed config was accepted" >&2
    exit 1
fi

env -u DISPLAY "$PROGRAM" --first-run --config "$VALID"
if env -u DISPLAY "$PROGRAM" --first-run --config "$INVALID" >/dev/null 2>&1; then
    echo "malformed first-run config was accepted" >&2
    exit 1
fi
if command -v xvfb-run >/dev/null 2>&1 && command -v timeout >/dev/null 2>&1; then
    if timeout 2 xvfb-run -a "$PROGRAM" --config "$VALID" >/dev/null 2>&1; then
        :
    else
        status=$?
        [ "$status" -eq 124 ] || {
            echo "X11 window smoke test failed with status $status" >&2
            exit 1
        }
    fi
fi
echo "C/X11 command and config checks passed"
