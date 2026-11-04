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

The PC keyboard feeds both the PS/2 event queue the BIOS reads and the
520EX/CPC key matrix that games read (ports 10h-1Ah of MMIO page 8). The
matrix table is generated from the core's `kbd_520ex.sv` by
`tools/gen_kbdmatrix.py`, so both always agree.

The first game controller (or joystick) is row 9 of the matrix, like the
core's joystick: D-pad or left stick, A/Cross fire 1, B/Circle fire 2.

Not yet: the 520EX/CPC legacy mode (the emulator stops when software switches
to it), IPC and the graphics accelerator.

## Build

    cmake -S . -B build && cmake --build build

Needs SDL2 (Linux: `libsdl2-dev`; Windows: vcpkg or the SDL2 development
libraries, with `SDL2_DIR` pointing to their cmake folder). z80ex is built
from `../deps/z80ex`.

## Use

    alestelx [-a disk.dsk] [-b disk.dsk] [--keyboard us|es] [--scale N]
             [--debug] [--sym file.map] [--break addr] boot.rom

`boot.rom` is the same file as on the MiSTer SD (`games/AlesteLX/boot.rom`:
LX.ROM, optionally followed by AL512.BIN). `--keyboard` is the OSD Keyboard
option (US by default); the BIOS reads it at boot, and `KEYB US|ES` changes it
in LX-DOS.

Keys in the window: the PC keyboard goes to the machine. F12 resets,
Shift+F12 switches the keyboard layout (applies at the next reset, like the
OSD option), F11 toggles full screen, Pause or Scroll Lock stops and resumes
(debugger).

## Debugger

A console in the terminal the emulator was started from, so it never fights
with the machine's keyboard. `--debug` starts paused; Pause/Scroll Lock in
the window stops and resumes at any time. Symbols come from z88dk map files
(`--sym lx/build/src/build/aleste.map` for the ROM, the LX-DOS map for the
DOS, etc.).

| Command | |
|---|---|
| `c` / `g [addr]` | continue (optionally until addr) |
| `p` | pause |
| `s [n]` | step n instructions |
| `n` | step over CALL, RST, DJNZ, LDIR and the other block instructions |
| `t [n]` | trace n instructions |
| `r` | registers, MMU (supervisor/user, control, MMIO page, slots, banks) and PIC |
| `d [addr] [n]` | disassemble, with symbols |
| `m [addr] [n]` / `pm phys [n]` | memory as the CPU sees it / physical memory |
| `b addr\|symbol\|p:phys` | breakpoint (logical, symbol or physical address) |
| `bl`, `bc n\|all` | list and clear breakpoints |
| `sym file.map` | load symbols |
| `q` | quit |

Addresses are hexadecimal (`C000`, `0xC000`, `$C000`) or symbol names.

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
| ZEXDOC.COM (whole run) | all 67 tests OK |
| keyboard matrix (`ctest`) | the 92 keys of the RTL table, each in its row and bit |
