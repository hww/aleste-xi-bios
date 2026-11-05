#!/bin/sh
# Aleste LX: LX-DOS en la unidad A, teclado de EE. UU.
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux -a LXDOS.DSK --keyboard us boot.rom "$@"
