# alestelx — user guide

*(Versión en español: `USAGE.es.md`.)*

`alestelx` emulates the **Aleste LX** as implemented in the FPGA core
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer), on Windows and
Linux. It runs the LX BIOS and LX-DOS, LX software (graphics, Magic Sound 2
music) and also the **Aleste 520EX / Amstrad CPC** compatible mode (AlesteDOS,
MSX-DOS, CPC BASIC, the MAG player, tapes). Every device follows the core's
hardware description, so what works in the emulator works on the MiSTer.

## 1. What is in the package

| File | |
|---|---|
| `alestelx.exe` + `SDL2.dll` | the emulator for Windows |
| `alestelx-linux` | the emulator for Linux |
| `boot.rom` | the ROM: LX BIOS followed by the 520EX ROM (the same file as `games/AlesteLX/boot.rom` on the MiSTer) |
| `LXDOS.DSK` | LX-DOS boot disk, with test programs, the image viewer, the demos and sample images |
| `alestedos-dev.dsk`, `mag-player.dsk` | 520EX disks: MSX-DOS with development tools, and the MAG player with its songs |
| `TAPEOK.CDT` | a small test tape for the CPC mode |
| `*.bat`, `*.sh` | launchers for each use (Windows / Linux) |
| `bios.map`, `lxdos.map` | symbols for the debugger |
| `img2lx.py` | converts pictures to the LX 256-colour format |
| `gdb_rsp_test.py` | test client for the GDB server |
| `README.md`, `GDB.md`, `USAGE.md` | documentation |

**Requirements.** Windows: nothing else (keep `SDL2.dll` next to the
`.exe`). Linux: SDL2 (`sudo apt install libsdl2-2.0-0` on Debian/Ubuntu).
`img2lx.py` needs Python 3 with Pillow (`pip install pillow`).

**The 520EX ROM.** The Aleste 520EX mode needs the original 520EX ROM,
`AL512.BIN` (64 KB, MAME set `al520ex`). The packages built on GitHub
already include it inside `boot.rom` (LX.ROM followed by AL512.BIN, as on
the MiSTer; see `ROMS.md`). With a `boot.rom` that only has LX.ROM, put
`AL512.BIN` in the same folder and the emulator loads it by itself (or
`--rom520 file`). Without it, the `520ex` command says that the ROM is
missing.

Keep `boot.rom` and `LXDOS.DSK` from the same package: the ROM checks that
LX-DOS was built with it and refuses to boot a different one (it says so on
screen).

## 2. Launchers

Double-click the `.bat` (Windows) or run the `.sh` (Linux). Any extra
argument is passed to the emulator (e.g. `alestelx.bat --scale 4`).

| Launcher | What it does |
|---|---|
| `alestelx` | LX-DOS from `LXDOS.DSK`, US keyboard |
| `alestelx-es` | the same with the Spanish keyboard |
| `alestelx-debug` | starts paused with the debugger in the terminal (type `help` there) |
| `alestelx-gdb` | GDB server on port 2159 for GDB or DeZog (see `GDB.md`) |
| `alestelx-520ex-basic` | 520EX in AMSTRAD mode: type `520ex` at the LX monitor and you get CPC BASIC |
| `alestelx-520ex-msxdos` | 520EX booting MSX-DOS from `alestedos-dev.dsk` (type `520ex` at the LX monitor) |
| `alestelx-520ex-mag` | 520EX with `mag-player.dsk`: then `mag axelf` (also sll1, chrush, headache, techno1) |
| `alestelx-520ex-tape` | CPC BASIC with `TAPEOK.CDT`: `\|TAPE`, then `RUN"` and a key |

## 3. Keys of the emulator window

The PC keyboard goes to the emulated machine, except these:

| Key | |
|---|---|
| F12 | reset |
| Shift+F12 | switch the keyboard layout US / Spanish (applies at the next reset, like the MiSTer OSD option) |
| F11 | full screen |
| Ctrl+F1 / Ctrl+F2 | choose a disk for drive A / B (file dialog) |
| Ctrl+Shift+F1 / F2 | eject the disk of drive A / B |
| Ctrl+F3 | choose a tape (520EX/CPC mode); Ctrl+Shift+F3 removes it |
| Ctrl+F4 | rewind the tape |
| Ctrl+F5 | capture / release the mouse (a click in the window also captures it) |
| Pause or Scroll Lock | stop / resume (debugger) |

The window title shows the disks in each drive, the tape and its position,
the keyboard layout, and whether the mouse is captured or the machine paused.

## 4. First steps with LX-DOS

`alestelx` (or `alestelx-es`) boots LX-DOS from `LXDOS.DSK`: the ROM finds
`LXDOS.SYS` on drive A and starts it. LX-DOS runs CP/M 2.2 and MSX-DOS 1
programs and has its own graphics and music functions.

| Command | |
|---|---|
| `A:`, `B:` | change the current drive |
| `DIR` | list the files |
| `TYPE file` | show a text file (Esc stops) |
| `COPY src dst` | copy a file (`COPY X.TXT B:` copies to the other drive) |
| `DEL file`, `REN old new` | delete (wildcards `*` and `?`), rename |
| `KEYB US` / `KEYB ES` | keyboard layout |
| `CLS`, `VER`, `HELP` | clear the screen, version, help |
| `MONITOR` | back to the ROM monitor |
| `name [args]` | run `NAME.COM` |

`AUTOEXEC.BAT`, if present, runs at boot.

Programs on `LXDOS.DSK`:

| Program | |
|---|---|
| `VIEW` | image viewer: shows every `.LXI` (any key: next, Esc: exit); `VIEW B:*.LXI` |
| `TUNNEL` | 256-colour demo with a rotating palette (a key exits) |
| `MODPLAY LXDEMO.MOD` | plays a MOD or STM in the background (`MODPLAY /S` stops it) |
| `HELLO`, `FILETEST`, `MSXTEST`, `GFX8TEST` | test programs (console, files, MSX-DOS functions, 8 bpp mode) |
| `ZEXDOC`, `ZEXALL` | Z80 instruction exercisers (they take a while) |

### Your own pictures

```
python3 img2lx.py photo.jpg PHOTO.LXI           # whole picture, with black bands
python3 img2lx.py photo.jpg PHOTO.LXI --fill    # crop it to fill the screen
```

The result is 320×200 with 256 colours from the LX 4096-colour palette,
with dithering. To put it on a disk, use the MSX-DOS disk tool of the
AlesteLX_MiSTer repository (`tools/make_msxdisk.py`), or copy it from
another disk with `COPY` inside the emulator.

### The ROM monitor

Without a system disk in drive A, or after `MONITOR`, you get the LX ROM
monitor (`LX>` prompt): `HELP`, `DIR`, `MOD file`, `PLAY`, `STOP`, `DEMO`,
`KEYS` (keyboard matrix), `DUMP addr`, `TIME`, `BOOT` (boot LX-DOS again) and
`520EX` (switch to the 520EX mode).

## 5. Floppy disks

- **Drag a `.dsk` file into the window** to insert it in drive A; hold
  **Shift** while dropping for drive B.
- Or **Ctrl+F1 / Ctrl+F2** for a file dialog (Windows; on Linux it uses
  `zenity` if installed), and Ctrl+Shift+F1 / F2 to eject.
- From the command line: `-a disk.dsk`, `-b disk.dsk`.

Images are 720 KB MSX-DOS disks (80 tracks, 2 sides, 9 sectors), like
those of the core and the 520EX. **Whatever the machine writes is saved back
into the image file**: when the disk is changed or ejected, on exit, and
every few seconds — as the MiSTer does with the SD card. Keep a copy of disks
you do not want modified.

## 6. Aleste 520EX / Amstrad CPC mode

The LX also contains the whole 520EX. To enter it: go to the ROM monitor
(no system disk, or `MONITOR` in LX-DOS) and type **`520ex`**. F12 (reset)
goes back to the LX.

- With **`alestedos-dev.dsk`** in drive A, the 520EX boots **MSX-DOS 1.04**
  (FOTON BIOS, COMMAND 2.2A): `dir`, M80/L80, PW and the other tools.
- With **`mag-player.dsk`**: `mag axelf` plays Scream Tracker songs through
  the 520EX MagicSound (also `sll1`, `chrush`, `headache`, `techno1`).
- With **`--boot amstrad`** (the `alestelx-520ex-basic` launcher), Shift is
  held for 2.5 s after the switch and the 520EX SETUP starts in AMSTRAD
  mode: **CPC BASIC 1.1**.
- **Tapes** (`.cdt`, `.tzx`): `--tape file`, drag the file into the window
  or Ctrl+F3. In BASIC, type `|TAPE` first (the default is disk), then
  `RUN"` or `LOAD"`. The tape plays while the motor is on; Ctrl+F4 rewinds.

Things to know:

- **The 520EX keyboard is positional**, as on the core: keys produce what
  is printed on the 520EX keyboard in that position. For example `"` is
  Shift+2, `|` is Shift + the key right of P, and the PC `=` key gives `↑`.
- The **clock RAM** (the 520EX SETUP configuration) is kept in `aleste.cmo`
  next to `boot.rom` (`--cmos file` to use another one), like the core's
  ALESTE.CMO. The time is the PC's.
- **Mice:** the 520EX serial mouse (MSX-DOS) is always there; CPC mice with
  `--cpc-mouse` (Kempston, SYMBiFACE II, Multiplay) and `--amx-mouse` (AMX
  on the joystick port), like the core's OSD options. Click in the window
  or Ctrl+F5 to capture the PC mouse.

## 7. Joystick

The first game controller or joystick is the machine's joystick (row 9 of
the keyboard matrix, as on the core): D-pad or left stick to move, A/Cross
fire 1, B/Circle fire 2. It can be plugged in or out while running.

## 8. Debugging

- **Built-in debugger** (`alestelx-debug`, or Pause / Scroll Lock at any
  time): a console in the terminal with breakpoints (logical, physical and
  by physical page), steps, trace, history, disassembly with symbols,
  memory, registers with the MMU state, disk change... Type `help`. Details
  in `README.md`.
- **GDB / DeZog** (`alestelx-gdb`): see `GDB.md`.

## 9. Command-line options

    alestelx [options] boot.rom

| Option | |
|---|---|
| `-a file`, `-b file` | disk in drive A / B |
| `--keyboard us\|es` | keyboard layout (US by default; `--es` = `--keyboard es`) |
| `--scale N` | window of 320N × 240N (3 by default) |
| `--boot amstrad` | 520EX: hold Shift at the switch, AMSTRAD mode (CPC BASIC) |
| `--tape file` | tape (CDT / TZX) for the 520EX/CPC mode |
| `--rom520 file` | the 520EX ROM (AL512.BIN) when boot.rom doesn't contain it (by default, an AL512.BIN next to boot.rom) |
| `--cmos file` | clock RAM file (default `aleste.cmo` next to `boot.rom`) |
| `--cpc-mouse`, `--amx-mouse` | CPC mice / AMX mouse |
| `--debug` | start paused with the debugger console |
| `--sym file.map` | load debugger symbols (repeatable) |
| `--break addr` | breakpoint (address, symbol, `p:physical`, `pg:page`) |
| `--gdb port` | GDB server (see `GDB.md`) |

For automated tests (with or without `--headless`, the window-less mode):
`--ms N` (run N ms of machine time and exit), `--type MS:text` (type text at
MS ms; `|` is Enter, `~` Esc), `--shot file.ppm` (screen at the end),
`--pcm file` (audio, stereo 16-bit 48 kHz), `--save-a file` (drive A image
at the end), `--cmd MS:command` (debugger command), `--mouse MS:dx,dy,buttons`.

## 10. Problems

- **"LXDOS.SYS was built for another ROM":** `boot.rom` and `LXDOS.DSK` come
  from different builds. Use both from the same package.
- **"Aleste 520EX ROM not found" after `520ex`:** put `AL512.BIN` next to
  `boot.rom` (section 1).
- **Black screen or garbage in the LX graphics modes:** make sure the
  emulator is from the same package (an old `.exe` does not know the 256
  colour mode).
- **No sound:** check the system volume and that no other program holds the
  audio device exclusively.
- **Linux: "error while loading shared libraries: libSDL2":** install SDL2
  (`sudo apt install libsdl2-2.0-0`).
- **Characters come out wrong in the 520EX:** its keyboard is positional
  (section 6). In LX mode, choose the layout that matches your keyboard
  (`--keyboard es`, `KEYB ES`).

## 11. Building from source

    cmake -S . -B build && cmake --build build

in the `emulator` folder of the repository (needs SDL2 development files;
on Windows, vcpkg or the SDL2 development libraries with `SDL2_DIR`). See
`README.md`.
