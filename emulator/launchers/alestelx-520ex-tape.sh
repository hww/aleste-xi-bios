#!/bin/sh
# Aleste 520EX en modo AMSTRAD con una cinta: escribe 520ex; en el BASIC, |TAPE, luego RUN" y una tecla
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux --boot amstrad --tape TAPEOK.CDT boot.rom "$@"
