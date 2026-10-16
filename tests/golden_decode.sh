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
check testdata/golden_short.frames    36db9b10dd894f3b3730b471b4fc95953fda0dc3e578c9e3426048aa0e924ff0
check testdata/golden_long.frames 66ca21ede2508d45a91b4851e105551de03424cdc692296f89e5983a77dbff1f
echo "golden decode: ALL OK"
