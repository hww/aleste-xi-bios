# Debugging the Aleste LX with GDB or DeZog (alestelx GDB server)

*(Versión en español: `GDB.es.md`.)*

`alestelx` includes a server for the **GDB remote protocol** (RSP, *Remote
Serial Protocol*): the same one used by `gdbserver`, OpenOCD or MAME's
*gdbstub*. With it, an external debugger controls the emulated machine: it
stops it, steps it one instruction at a time, sets breakpoints and reads or
writes registers and memory, while the emulator keeps showing the screen and
playing sound as usual.

Possible clients:

- **DeZog** (Z80 debugging extension for VS Code): visual debugging with the
  source code, registers, memory, stack and disassembly. It connects as it
  does to MAME.
- **GDB** built with Z80 support, from the command line.
- Any tool that speaks RSP (IDA, your own scripts...). `tests/gdb_rsp_test.py`
  is a minimal Python client that works both as an example and as a test.

## Starting the emulator with the server

Windows: `alestelx-gdb.bat`. Linux: `./alestelx-gdb.sh`. Both run:

    alestelx -a LXDOS.DSK --gdb 2159 boot.rom

- `--gdb PORT` opens the server on `localhost:PORT` (2159 in the launchers).
  It only listens on the local machine (127.0.0.1): it is not reachable from
  the network.
- The machine **starts stopped**, at reset (PC = 0000h), waiting for the
  debugger to connect and tell it to go on, like MAME with `-debug -debugger
  gdbstub`.
- When a debugger connects, the machine stops; when it disconnects, the
  machine runs on its own. You can connect again.
- It combines with the other options: disks, `--keyboard`, `--sym`,
  `--debug` (the built-in debugger console in the terminal, at the same
  time)...

Without a window, for automated tests:

    alestelx --headless --gdb 2159 -a LXDOS.DSK boot.rom

In this mode the emulator exits when the debugger disconnects or kills it
(`kill`).

## What the server provides

### Registers

Twelve 16-bit registers, in the order of MAME's *gdbstub* (the one DeZog
expects), also described in the `target.xml` the server sends, so GDB
understands them with no configuration:

| n | register | n | register | n | register |
|---|---|---|---|---|---|
| 0 | AF | 4 | AF' | 8 | IX |
| 1 | BC | 5 | BC' | 9 | IY |
| 2 | DE | 6 | DE' | 10 | SP |
| 3 | HL | 7 | HL' | 11 | PC |

I, R and the interrupt mode are not in this list: `monitor r` shows them
(see below).

### Memory: logical and physical

The LX has 16 MB of physical memory and an MMU that maps 16 KB banks into
the four windows the CPU sees. The server gives access to both:

| Address in the debugger | What it is |
|---|---|
| `0000`-`FFFF` | what the CPU sees now, through the MMU (in 520EX mode, through its MMU) |
| `10000` and up | **physical** memory: address − 10000h |

Physical memory examples:

| In the debugger | Physical | What is there |
|---|---|---|
| `0xC10000` | C00000 | the LX ROM (BIOS) |
| `0xC50000` | C40000 | the text mode VRAM |
| `0xC60000` | C50000 | the 320×200×256 VRAM |
| `0xC90000` | C80000 | LXDOS.SYS |
| `0xCA0000` | C90000 | the running .COM program (its 0000h) |

Reading and writing work in both areas (reads of up to 2 KB per request),
for clients that send the full address, such as the test one. **GDB for Z80
works with 16-bit addresses** and may truncate the physical ones: with GDB,
look at physical memory with `monitor pm` (see below).

### Breakpoints

- **Execution** breakpoints, by CPU address (GDB's normal `break` and
  DeZog's). They trigger whatever bank is mapped in that window: a
  breakpoint at 4000h stops both in ROM code and in LX-DOS code if both go
  through there.
- To stop only in a specific bank, use the built-in debugger commands
  through `monitor`:
  - `monitor b p:C86CED` — at that exact **physical** address;
  - `monitor b pg:322` — when anything in physical page 322h runs (16 KB:
    physical address / 4000h; here C88000-C8BFFF, bank 22h of slot 3, the
    third LX-DOS window).
  - When they trigger, the external debugger gets the stop like any other.
- Memory watchpoints: not yet. If you ask GDB for one, it emulates it by
  single-stepping (slow, but it works).

### Control

| | |
|---|---|
| continue | `continue` (also from a given address) |
| one step | `stepi` (one instruction, with its prefixes) |
| stop | Ctrl-C in GDB, or DeZog's pause button |
| quit | `detach` (the machine keeps running) or `kill` (the emulator exits) |

### `monitor`: the alestelx debugger commands

`monitor COMMAND` runs any command of the built-in debugger. **Its output
goes to the emulator's terminal**, not to the debugger's. The most useful:

| | |
|---|---|
| `monitor r` | registers, I, R, IM, IFF, the MMU (supervisor, slots, banks) and the PIC |
| `monitor hist 40` | the last 40 executed instructions, disassembled from where they ran |
| `monitor d 4000 20` | disassemble with symbols |
| `monitor pm C50000 64` | physical memory |
| `monitor sym lxdos.map` | load symbols from a z88dk map file |
| `monitor b p:C86CED`, `monitor b pg:322` | physical or physical-page breakpoints |
| `monitor bl`, `monitor bc all` | list and clear them |
| `monitor disk a other.dsk` | change the floppy |

See `README.md` for the full list.

## With DeZog (VS Code)

DeZog connects to the emulator as it does to MAME: connection type `mame`,
on `localhost` and the emulator's port. A starting `launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "dezog",
            "request": "launch",
            "name": "Aleste LX (alestelx)",
            "remoteType": "mame",
            "mame": { "hostname": "localhost", "port": 2159 },
            "rootFolder": "${workspaceFolder}",
            "startAutomatically": false
        }
    ]
}
```

1. Start the emulator with `alestelx-gdb.bat` / `alestelx-gdb.sh`.
2. In VS Code, launch the "Aleste LX (alestelx)" configuration.
3. Registers, memory, disassembly, breakpoints (click in the margin) and
   steps work from DeZog's interface.

For source-level debugging DeZog reads z88dk list files: add to the
configuration the z88dk list file section described in the DeZog
documentation for your version, pointing to the `.lis` files and the `.map`
of the build (`lx/build/src/build/aleste.map` for the ROM,
`lx/build/dos/lxdos.map` for LX-DOS). Remember that the ROM and LX-DOS use
the same logical addresses in different banks: load the lists of the one you
are debugging.

> The server has been tested with its own RSP client
> (`tests/gdb_rsp_test.py`), which follows the same steps as GDB and DeZog;
> not yet with DeZog itself. If something does not fit, the emulator's
> terminal shows every connection and the `monitor` commands.

## With GDB

You need a GDB with Z80 support (available since GDB 11). Ubuntu's and
Debian's `gdb-multiarch` does **not** include it ("Undefined item: z80"). To
build it on Linux (or in WSL on Windows):

```sh
wget https://ftp.gnu.org/gnu/gdb/gdb-15.2.tar.xz
tar xf gdb-15.2.tar.xz && cd gdb-15.2
./configure --target=z80-unknown-elf --prefix=$HOME/z80gdb --disable-sim
make -j$(nproc) && make install
export PATH=$HOME/z80gdb/bin:$PATH        # installed as z80-unknown-elf-gdb
```

A session:

```
$ z80-unknown-elf-gdb
(gdb) target remote :2159
(gdb) info registers
(gdb) x/8xb 0                     # CPU memory: DI, JP 0100h after reset
(gdb) monitor pm C00000 32        # physical memory (shown in the emulator's terminal)
(gdb) break *0x4B4E               # a breakpoint (the ROM's main: grep _main bios.map)
(gdb) continue
(gdb) x/5i $pc                    # disassemble
(gdb) stepi
(gdb) monitor hist 20             # output in the emulator's terminal
(gdb) set $hl = 0x1234            # change a register
(gdb) set *(unsigned char*)0x0100 = 0     # write to CPU memory
(gdb) continue                    # Ctrl-C to stop
(gdb) detach                      # or kill
```

Function addresses come from the z88dk map files (`aleste.map`,
`lxdos.map`, shipped in the package as `bios.map` and `lxdos.map`): `grep
_main bios.map`, or `monitor sym bios.map` and then `monitor d main`. They
change with every ROM build.

## Common problems

- **It does not connect:** the port is used by another program (try another
  one with `--gdb 2160`), or the emulator was not started with `--gdb`. It
  only accepts connections from the same machine.
- **It stops "by itself" at startup:** that is expected, it waits for the
  debugger. Press continue.
- **A breakpoint triggers where you did not expect:** they are by logical
  address; use `monitor b p:...` for a specific physical address.
- **The machine hangs at C000h (`_undefined_method`):** the ROM and
  LXDOS.SYS are not from the same build; newer ROMs detect it and say so at
  boot.

## Testing the server

```sh
./alestelx-linux --headless --gdb 2159 -a LXDOS.DSK boot.rom &
python3 gdb_rsp_test.py 2159 bios.map
```

It goes through `qSupported`, `target.xml`, registers, logical and physical
memory, a breakpoint at `main` (taken from the map) with `continue`, a step,
`monitor`, Ctrl-C and `kill`.

## Packets the server understands (for client writers)

`?`, `g`, `G`, `p`, `P`, `m`, `M`, `c [addr]`, `s [addr]`, `vCont?`,
`vCont;c|C|s|S|t`, `Z0`/`Z1` and `z0`/`z1`, `D`, `k`, `vKill`, `H`, `T`,
`qSupported`, `qXfer:features:read:target.xml`, `qAttached`, `qC`,
`qfThreadInfo`, `qsThreadInfo`, `qOffsets`, `qSymbol`, `qRcmd` (monitor),
`QStartNoAckMode` and the 03h byte (Ctrl-C). Stops: `S05` (breakpoint or
step) and `S02` (interrupt).
