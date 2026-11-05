#!/bin/sh
# Aleste 520EX y el reproductor MAG: escribe 520ex en el monitor del LX; con MSX-DOS, mag axelf (o sll1, chrush, headache, techno1)
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux -a mag-player.dsk boot.rom "$@"
