#!/bin/sh
# Aleste LX con el servidor GDB en el puerto 2159: gdb (target remote :2159) o DeZog (conexion MAME, puerto 2159)
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux -a LXDOS.DSK --gdb 2159 boot.rom "$@"
