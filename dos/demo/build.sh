#!/bin/sh
# build.sh - GPUDEMO.COM: microprogramas (pk_*.h) y programa (z88dk, CP/M).
# Uso (desde aleste-xi-bios): sh dos/demo/build.sh [ensamblador de la GPU]
# Shaeon (Carlos Palmero).
set -e
cd "$(dirname "$0")"
ASM=${1:-../../deps/lx-gpu-rtl/build/asm}
for f in gpu/*.asm; do
    n=$(basename "$f" .asm)
    python3 mkpkth.py "$f" "pk_$n.h" "pk_$n" "$ASM"
done
