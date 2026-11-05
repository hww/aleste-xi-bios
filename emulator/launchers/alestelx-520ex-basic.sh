#!/bin/sh
# Aleste 520EX en modo AMSTRAD: escribe 520ex en el monitor del LX (sin disco) y llegas al BASIC del CPC
# Necesita SDL2 (Debian/Ubuntu: sudo apt install libsdl2-2.0-0)
cd "$(dirname "$0")" || exit 1
exec ./alestelx-linux --boot amstrad boot.rom "$@"
