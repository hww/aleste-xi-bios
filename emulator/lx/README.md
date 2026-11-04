# alestelx — Aleste LX emulator (native mode)

Emulator of the Aleste LX as implemented in the FPGA core
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer), for Windows and
Linux (SDL2). Every device follows the core's RTL register by register, so
software that works here works on the FPGA and the other way round.

Emulated: 16 MB of physical memory, native MMU (slots, banks, E0h-FFh,
supervisor/user, syscall, traps), PIC and system timer, MC146818 clock (PC
time), palette and CRTC with the LX extended registers (linear 1/2/4 bpp
modes), DMA/blitter, Magic Sound 2 (4 channels, 48 kHz), PS/2 keyboard (US or
Spanish layout) and the uPD765 floppy controller with 720 KB MSX-DOS images
(read and write, two drives). CPU: z80ex at 18 MHz, 60 Hz VBlank.

Not yet: the 520EX/CPC legacy mode (the emulator stops when software switches
to it), the keyboard matrix, IPC and the graphics accelerator.

## Build

    cmake -S . -B build && cmake --build build

Needs SDL2 (Linux: `libsdl2-dev`; Windows: vcpkg or the SDL2 development
libraries, with `SDL2_DIR` pointing to their cmake folder). z80ex is built
from `../deps/z80ex`.

## Use

    alestelx [-a disk.dsk] [-b disk.dsk] [--es] [--scale N] boot.rom

`boot.rom` is the same file as on the MiSTer SD (`games/AlesteLX/boot.rom`:
LX.ROM, optionally followed by AL512.BIN). The PC keyboard goes to the
machine; F12 resets, F11 toggles full screen.

Headless mode, for automated tests (the same checks as the core testbench):

    alestelx --headless --ms 3000 -a LXDOS.DSK --type "1200:dir|" \
             --shot screen.ppm --pcm audio.raw --save-a after.dsk boot.rom

`--type MS:text` types the text at MS milliseconds of machine time (`|` is
Enter, `~` Esc), `--shot` saves the screen (PPM), `--pcm` the Magic Sound 2
output (stereo, 16 bits, 48 kHz) and `--save-a` drive A's image at the end.

## Validation (same results as the core in simulation)

| Test | Result |
|---|---|
| boot ROM + LXDOS.DSK, `dir`, `hello one two` | boots LX-DOS, lists the disk, HELLO.COM prints its checks |
| FILETEST.COM, MSXTEST.COM | all file, block, date/time and absolute-sector checks pass; the saved disk is a valid MSX-DOS image |
| `modplay lxdemo.mod` | bass C3 and melody E4/F4, as in the core |
