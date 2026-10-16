#!/bin/bash
# Decode regression check: golden frame files must decode to PCM with
# the committed sha256 (no audio kept in-tree).
set -e
cd "$(dirname "$0")/.."
B=build/imbebench
[ -x "$B" ] || { echo "build first (cmake -B build && cmake --build build)"; exit 1; }
check() { # frames-file expected-sha
    "$B" --decode "$1" /tmp/golden.$$ >/dev/null 2>&1
    got=$(sha256sum /tmp/golden.$$ | cut -d' ' -f1)
    rm -f /tmp/golden.$$
    if [ "$got" != "$2" ]; then echo "$1: decode hash mismatch: $got"; exit 1; fi
    echo "$1: decode hash OK"
}
check testdata/golden_short.frames    3ef8089a18e23423e0a0ae1c1b3f11437bf5f022bce35197ae2e555b7a284bbf
check testdata/golden_long.frames 04363a74cd3f5d4509d1c41655ce4709f02d103e7fa7026989d0af14825ddd55
echo "golden decode: ALL OK"
