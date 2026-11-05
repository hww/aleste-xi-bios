@echo off
rem Aleste 520EX y el reproductor MAG: escribe 520ex en el monitor del LX; con MSX-DOS, mag axelf (o sll1, chrush, headache, techno1)
cd /d "%~dp0"
alestelx.exe -a mag-player.dsk boot.rom %*
