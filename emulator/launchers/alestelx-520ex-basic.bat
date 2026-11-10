@echo off
rem Aleste 520EX en modo AMSTRAD: escribe 520ex en el monitor del LX (sin disco) y llegas al BASIC del CPC
cd /d "%~dp0"
alestelx.exe --boot amstrad boot.rom %*
