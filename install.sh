#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PREFIX=${PREFIX:-/usr/local}
SYSCONFDIR=${SYSCONFDIR:-/etc}
DESTDIR=${DESTDIR:-}

make -C "$ROOT" install PREFIX="$PREFIX" SYSCONFDIR="$SYSCONFDIR" \
    DESTDIR="$DESTDIR"
