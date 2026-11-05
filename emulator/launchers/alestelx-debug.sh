#!/bin/sh
# Aleste LX con el depurador: escribe help en esta terminal (Pausa en la ventana para y sigue)
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux -a LXDOS.DSK --debug --sym bios.map boot.rom "$@"
