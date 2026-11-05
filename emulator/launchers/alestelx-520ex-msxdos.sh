#!/bin/sh
# Aleste 520EX: MSX-DOS de alestedos-dev.dsk. Escribe 520ex en el monitor del LX. Arrastra otro .dsk a la ventana para cambiar el disco (Mayus = unidad B), o Ctrl+F1 / Ctrl+F2
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux -a alestedos-dev.dsk boot.rom "$@"
