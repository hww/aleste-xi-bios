@echo off
rem Aleste LX con el depurador: escribe help en esta terminal (Pausa en la ventana para y sigue)
cd /d "%~dp0"
alestelx.exe -a LXDOS.DSK --debug --sym bios.map boot.rom %*
