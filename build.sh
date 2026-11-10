#!/bin/sh
set -eu

if [ "$(uname -s)" != "Linux" ]; then
    echo "Build and tests must run in WSL/Linux." >&2
    exit 1
fi

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"
make clean
make test
build/GnuChanWelcome --version
build/GnuChanWelcome --check-config --config examples/welcome.conf
sh -n install.sh uninstall.sh tests/test_install.sh
echo "C/X11 build, configuration, and installer checks passed."
