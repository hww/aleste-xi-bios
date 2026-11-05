@echo off
rem Aleste LX: LX-DOS en la unidad A, teclado de EE. UU.
cd /d "%~dp0"
alestelx.exe -a LXDOS.DSK --keyboard us boot.rom %*
