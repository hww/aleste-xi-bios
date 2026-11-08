# Disk and tape images for alestelx

| File | Mode | What it is | Source |
|---|---|---|---|
| `alestedos-dev.dsk` | 520EX | MSX-DOS 1.04 (FOTON BIOS, COMMAND 2.2A) with development tools (M80/L80, PW...) | [hww/Aleste520EX](https://github.com/hww/Aleste520EX), `Soft/Disks` |
| `mag-player.dsk` | 520EX | MAG, the MagicSound player, with STM songs (`mag axelf`, `sll1`, `chrush`, `headache`, `techno1`) | [hww/Aleste520EX](https://github.com/hww/Aleste520EX), `Soft/Disks` |
| `TAPEOK.CDT` | 520EX / CPC | small test tape: in CPC BASIC, `\|TAPE`, `RUN"` and a key | `tests/tapeok.asm` + `tools/make_cdt.py` |

The LX-DOS boot disk (`LXDOS.DSK`) is not here: the workflow takes it from the
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer) repository
(`sd/LXDOS.DSK`), together with `LX.ROM`, so that both come from the same build.
