@echo off
rem Aleste LX con el servidor GDB en el puerto 2159: gdb (target remote :2159) o DeZog (conexion MAME, puerto 2159)
cd /d "%~dp0"
alestelx.exe -a LXDOS.DSK --gdb 2159 boot.rom %*
