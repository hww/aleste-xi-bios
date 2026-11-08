# ROMs for alestelx

| File | Size | CRC32 | What it is |
|---|---|---|---|
| `AL512.BIN` | 64 KB | E8C2A9A1 | Aleste 520EX firmware ROM (Patisonic, 1993), as in MAME's `al520ex` set |

The Aleste LX boot ROM is `boot.rom` = `LX.ROM` (the LX BIOS, built in the
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer) repository, `sd/LX.ROM`)
followed by `AL512.BIN`, exactly as `games/AlesteLX/boot.rom` on the MiSTer. The
GitHub workflow builds it that way for the packages. Without `AL512.BIN` the LX
native mode works, but the `520ex` command can't start the Aleste 520EX mode.

The 520EX ROM contains the Amstrad CPC firmware and Locomotive BASIC, whose
copyright belongs to Amstrad and Locomotive Software; Amstrad has allowed the
CPC ROMs to be distributed for use with emulators, keeping their copyright
messages intact. The rest is © Patisonic (Omsk), 1993. It is included here so
that the emulator packages work out of the box.
