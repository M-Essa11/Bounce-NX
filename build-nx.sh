#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [[ "$PWD" == *" "* ]]; then
    echo "Build from a path without spaces, for example C:/Projects/Bounce-NX." >&2
    exit 1
fi
: "${DEVKITPRO:?Run this from the devkitPro MSYS2 shell}"
for asset in icons levels sounds lang fonts; do
    mkdir -p "romfs-nx/$asset"
    cp -ru "$asset/." "romfs-nx/$asset/"
done
make -j"${JOBS:-4}" "$@"
