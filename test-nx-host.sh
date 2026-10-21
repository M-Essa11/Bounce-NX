#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build-nx
gcc -DBOUNCE_NX -std=c11 -Wall -Wextra -Isrc \
    tests/physics_slope_test.c src/physics.c -o build-nx/physics_test.exe
./build-nx/physics_test.exe
echo "Ramp and reverse-gravity regression tests passed."
gcc -DBOUNCE_NX -std=c11 -Wall -Wextra -Isrc \
    tests/nx_resources_test.c src/local.c src/cbmf.c -o build-nx/nx_resources_test.exe
./build-nx/nx_resources_test.exe "$PWD/romfs-nx"
