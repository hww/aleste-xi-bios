@echo off
rem Aleste 520EX en modo AMSTRAD con una cinta: escribe 520ex; en el BASIC, |TAPE, luego RUN' y una tecla
cd /d "%~dp0"
alestelx.exe --boot amstrad --tape TAPEOK.CDT boot.rom %*
