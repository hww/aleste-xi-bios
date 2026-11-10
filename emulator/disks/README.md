# Disk and tape images for alestelx

| File | Mode | What it is | Source |
|---|---|---|---|
| `ALESTDOS_TIME.dsk` | 520EX | MSX-DOS 1.04 (FOTON BIOS, COMMAND 2.2A) with development tools (M80/L80, PW, DBG...) and `TIME.COM`; BDOS for **two drives** | [shaeon/aleste520ex](https://github.com/shaeon/aleste520ex), `sd/dsk` |
| `mag-player.dsk` | 520EX | MAG, the MagicSound player, with STM songs (`mag axelf`, `sll1`, `chrush`, `headache`, `techno1`) | [hww/Aleste520EX](https://github.com/hww/Aleste520EX), `Soft/Disks` |
| `TAPEOK.CDT` | 520EX / CPC | small test tape: in CPC BASIC, `\|TAPE`, `RUN"` and a key | `tests/tapeok.asm` + `tools/make_cdt.py` |

**Two drives.** `ALESTDOS_TIME.dsk` has the MSX-DOS BDOS for two drives, so A:
and B: are independent (e.g. `mag-player.dsk` in B, then `b:` and `mag axelf`).
The `alestedos-dev.dsk` of hww/Aleste520EX has the one-drive BDOS: there, B: is
a "phantom" drive that asks to swap the disk in drive A ("Insert diskette for
drive B:"). `TIME.COM` shows the time from the clock (`DATE` only shows the
date).

`mag-player.dsk` boots too, but its MSX-DOS doesn't set the colours: the
prompt is printed in black on black. Typing `mag axelf` blind works; booting
`ALESTDOS_TIME.dsk` with `mag-player.dsk` in B is more comfortable.

The LX-DOS boot disk (`LXDOS.DSK`) is not here: the workflow takes it from the
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer) repository
(`sd/LXDOS.DSK`), together with `LX.ROM`, so that both come from the same build.
