@echo off
rem Aleste 520EX: MSX-DOS de alestedos-dev.dsk. Escribe 520ex en el monitor del LX. Arrastra otro .dsk a la ventana para cambiar el disco (Mayus = unidad B), o Ctrl+F1 / Ctrl+F2
cd /d "%~dp0"
alestelx.exe -a alestedos-dev.dsk boot.rom %*
