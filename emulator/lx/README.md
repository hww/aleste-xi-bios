# alestelx — Aleste LX emulator

User guide: `USAGE.md` (Spanish: `USAGE.es.md`). GDB / DeZog: `GDB.md` (`GDB.es.md`).

Emulator of the Aleste LX as implemented in the FPGA core
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer), for Windows and
Linux (SDL2). Every device follows the core's RTL register by register, so
software that works here works on the FPGA and the other way round.

Native video follows mc6845mod.sv: 1, 2, 4 and 8 bits per pixel, CPC-style
or linear addressing (R20) and 2 to 16 bytes per character (R21); the
320x200x256 mode (8 bpp linear) matches the core pixel for pixel.

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

### 520EX / CPC (legacy) mode

When software switches the MMU to legacy mode (the `520EX` command of the ROM
monitor, with AL512.BIN behind LX.ROM in `boot.rom`), the emulator becomes an
Aleste 520EX, following the core's RTL: legacy MMU (CPC RAM configurations,
lower/upper ROM, the PROM D62 upper ROM mapping and the MSX mapper with
MAPMOD), UM6845 CRTC (type 0) emulated character by character, Gate Array
(inks, modes changing at the output HSYNC, GA40010 output syncs and the
300 Hz interrupt with its 52-line counter), CPC modes 0-3 and the 520EX high
resolution modes, PROM D62 colours or MAPMOD direct RGB, PPI 8255, AY
registers with the keyboard matrix on its port A, the FABF extension port,
the idle 8251 and the floppy controller at FB7E/FB7F (DISKINT on PB1). The
CPU runs at 4 MHz. The picture is the monitor's visible window (768 x 272,
48 us x 272 lines after the syncs), as in the core.

`--boot amstrad` holds SHIFT for 2.5 s after the switch (until a key is
pressed), like the core's boot option: the 520EX SETUP then starts in
AMSTRAD mode (CPC BASIC 1.1).

Also emulated: the AY-3-8910 (YM2149 volume table, ABC stereo like the
core) and the MC146818 clock shared with the AY on the PPI port A (FABF D5,
PC2..PC0 commands, UF update flag, years since 1980), which the 520EX needs
to find its floppy drives.

And the 520EX MagicSound (Patisonic's board, as in the core's
`magic_sound.sv`): 8237 DMA, two 8254s at 4 MHz, a DAC and a volume byte per
channel, the inverted DMA page mapper and the end-of-block NMI the MAG player
uses; channels A and D on the left, B and C on the right.

Mice, as in the core: the 520EX serial mouse on the 8251 (Mouse Systems
packets at 1200 baud, only while the receiver is enabled; the MSX-DOS BIOS
uses it), and the CPC mice Kempston (FBEE/FBEF/FAEF), SYMBiFACE II (FD10),
Multiplay (F990-F997) with `--cpc-mouse`, and AMX on the joystick row with
`--amx-mouse`. Click in the window (or Ctrl+F5) to capture the PC mouse;
Ctrl+F5 releases it.

Tape: TZX/CDT files with the same blocks as the core's player (10-15, 20-25,
2A, 2B, 30-35, 5A), decoded to pulses and played while the motor (PPI PC4)
is on, into PB7. `--tape file`, drop a .cdt/.tzx into the window or Ctrl+F3;
Ctrl+F4 rewinds, Ctrl+Shift+F3 removes it; the title shows the position.
`tools/make_cdt.py` turns a binary into a CPC firmware CDT (1000 baud).

Done: 520EX boot screen, CPC BASIC with keyboard and sound, MSX-DOS 1.04
booting from `alestedos-dev.dsk` (FOTON-127 BIOS, COMMAND 2.2A, DIR, disk
change), MAG.COM playing the STM songs of `mag-player.dsk`, serial mouse and
tape loading.

Not yet in native mode: IPC and the graphics accelerator.

## Automatic builds

`.github/workflows/alestelx.yml` builds the emulator on every push or pull
request that touches `emulator/`: Linux (Ubuntu, SDL2) and Windows (Visual
Studio 2022 + vcpkg, through `CMakePresets.json`). It runs the keyboard
matrix test, a headless LX-DOS boot and the GDB server test, and uploads
both packages as artifacts (Actions tab > run > Artifacts), with LX.ROM and
LXDOS.DSK taken from the MiSTer core repository. Pushing a tag
`alestelx-v*` (e.g. `alestelx-v0.1`) also publishes them as a release. For
the 520EX mode, append the 520EX ROM (AL512.BIN) to boot.rom.

## Launchers

`launchers/` has ready-made scripts for each use, as `.bat` (Windows) and
`.sh` (Linux): `alestelx` (US keyboard), `alestelx-es`, `alestelx-debug`,
`alestelx-gdb`, `alestelx-520ex-basic`, `alestelx-520ex-msxdos`,
`alestelx-520ex-mag` and `alestelx-520ex-tape`. They expect the emulator,
`boot.rom` and the disk images in the same folder; extra arguments are passed
on. On Linux the binary needs SDL2 (`sudo apt install libsdl2-2.0-0`).

## Build

    cmake -S . -B build && cmake --build build

Needs SDL2 (Linux: `libsdl2-dev`; Windows: vcpkg or the SDL2 development
libraries, with `SDL2_DIR` pointing to their cmake folder). z80ex is built
from `../deps/z80ex`.

## Use

    alestelx [-a disk.dsk] [-b disk.dsk] [--keyboard us|es] [--scale N]
             [--debug] [--sym file.map] [--break addr] [--boot amstrad]
             [--cmos file] [--tape file] [--cpc-mouse] [--amx-mouse] [--gdb port] boot.rom

`boot.rom` is the same file as on the MiSTer SD (`games/AlesteLX/boot.rom`:
LX.ROM, optionally followed by AL512.BIN). `--keyboard` is the OSD Keyboard
option (US by default); the BIOS reads it at boot, and `KEYB US|ES` changes it
in LX-DOS.

### Floppy disks

Drag a `.dsk` file into the window to insert it in drive A (hold Shift for
drive B), or use Ctrl+F1 / Ctrl+F2 to pick one with a file dialog (Windows;
on Linux, `zenity` if installed); Ctrl+Shift+F1 / F2 eject. The debugger
also has `disk a|b file` and `eject a|b`. The window title shows what is in
each drive. Whatever the machine writes goes back to the image file when the
disk is changed or ejected, on exit and every few seconds, as the MiSTer does
with the SD card.

The MC146818 clock RAM (the 520EX SETUP configuration) is kept in
`aleste.cmo` next to `boot.rom` (`--cmos file` to change it), like the core's
ALESTE.CMO. The time is the PC's.

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
| `b addr\|symbol\|p:phys\|pg:NN` | breakpoint (logical, symbol, physical address or any address in physical 16 KB page NN) |
| `hist [n]` | the last n executed instructions, disassembled from where they ran |
| `disk a\|b file`, `eject a\|b` | change or eject a floppy |
| `bl`, `bc n\|all` | list and clear breakpoints |
| `sym file.map` | load symbols |
| `q` | quit |

Addresses are hexadecimal (`C000`, `0xC000`, `$C000`) or symbol names.

Headless mode, for automated tests (the same checks as the core testbench):

    alestelx --headless --ms 3000 -a LXDOS.DSK --type "1200:dir|" \
             --shot screen.ppm --pcm audio.raw --save-a after.dsk boot.rom

`--type MS:text` types the text at MS milliseconds of machine time (`|` is
Enter, `~` Esc), `--mouse MS:dx,dy,buttons` moves the mouse,
`--cmd MS:command` runs a debugger command then (with a
breakpoint, the run stops there and prints the history), `--shot` saves the screen (PPM), `--pcm` the Magic Sound 2
output (stereo, 16 bits, 48 kHz) and `--save-a` drive A's image at the end.

## GDB remote protocol (gdb-multiarch, DeZog)

`--gdb PORT` starts a GDB RSP server on localhost (the machine stays paused
until the debugger continues, like MAME's `-debug -debugger gdbstub`). It
works with the window and headless (`--headless --gdb 2159`: runs until the
debugger kills it or disconnects).

- Registers in MAME's order (AF BC DE HL AF' BC' DE' HL' IX IY SP PC), also
  described in the `target.xml` the server sends, so DeZog (VS Code) can use
  its MAME connection and a Z80-enabled GDB reads the layout from the XML.
- Memory: 0000-FFFF is what the CPU sees through the MMU; from 10000h on it
  is physical memory (address - 10000h), e.g. `0xC60000` = physical C50000,
  the 8 bpp VRAM.
- Execution breakpoints (Z0/Z1), continue, step, Ctrl-C, kill, detach;
  `monitor <command>` runs any alestelx debugger command (`hist`, `pm`, `b
  pg:NN`, ...) with its output in the emulator's terminal.

A full guide, with DeZog and GDB examples: `lx/GDB.md` (Spanish: `lx/GDB.es.md`).
`tests/gdb_rsp_test.py` talks the protocol like GDB does (registers, logical
and physical memory, breakpoint, continue, step, monitor, Ctrl-C, kill).

## Validation (same results as the core in simulation)

| Test | Result |
|---|---|
| boot ROM + LXDOS.DSK, `dir`, `hello one two` | boots LX-DOS, lists the disk, HELLO.COM prints its checks |
| FILETEST.COM, MSXTEST.COM | all file, block, date/time and absolute-sector checks pass; the saved disk is a valid MSX-DOS image |
| `modplay lxdemo.mod` | bass C3 and melody E4/F4, as in the core |
| ZEXDOC.COM (whole run) | all 67 tests OK |
| GFX8TEST.COM (`tests/gfx8test.asm`): 320x200, 256 colours, 8 bpp linear | identical, pixel by pixel, to the core in simulation |
| SYSTEST.COM (`tests/systest.asm`): RST 6 syscalls from user mode, keys pressed during its 1 s wait | same result as the core in simulation: 1011 calls, 8 interrupts taken in user mode, private and shared pages OK; 1511 cycles per null call |
| VIEW.COM and TUNNEL.COM on LXDOS.DSK (LX-DOS graphics, AlesteLX_MiSTer) | the four sample .LXI images; the tunnel with its rotating palette |
| GDB server (`tests/gdb_rsp_test.py`) | target.xml, registers, memory (logical and physical), breakpoint at `_main`, continue, step, monitor, Ctrl-C and kill |
| keyboard matrix (`ctest`) | the 92 keys of the RTL table, each in its row and bit |
| `520ex` from the ROM monitor | 520EX boot screen (high resolution, 16 colours, MAPMOD) |
| `520ex` with `--boot amstrad` | CPC BASIC 1.1 ("Aleste 520EX Personal computer"), typing and running a program; `sound 1,284,100` gives 220.0 Hz on the left channel |
| `520ex` with `alestedos-dev.dsk` in A | MSX-DOS 1.04 boots (FOTON-127 BIOS, COMMAND 2.2A, today's date), `dir` lists the 25 files |
| disk change while MSX-DOS runs | `dir` shows the new disk; writes reach the image file |
| `mag axelf` from `mag-player.dsk` | MAG plays AXELF.STM in stereo through the MagicSound, VU meters moving |
| `mouse1` from `alestedos-dev.dsk`, mouse moved 252 right / 126 down | shows 84 and 42 (the BIOS scales by 1/3): serial mouse packets through the 8251 |
| CPC BASIC: `\|tape`, `run"` with a CDT from `tools/make_cdt.py` (`tests/tapeok.asm`) | the firmware loads it at 1000 baud (header and data with CRCs) and runs it |
