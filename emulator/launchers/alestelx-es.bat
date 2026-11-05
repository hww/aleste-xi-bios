@echo off
rem Aleste LX: LX-DOS en la unidad A, teclado espanol
cd /d "%~dp0"
alestelx.exe -a LXDOS.DSK --keyboard es boot.rom %*
