# GPUDEMO: LX-GPU demo and benchmark

`GPUDEMO.COM` runs the scenario written by hww (H2W Lab), "GPU Demo and
Benchmark Scenario". It covers ten scenes that go from the cheapest operations
to the most expensive ones. Each one has a visual run and a dry `--bench` run,
and both use the same microprograms.

```
GPUDEMO [--m 8|4] [--s 64|32] [--b N] [--t N] [--bench] [--only N]
        [--frames F] [--n N]
```

| Option | Meaning |
|---|---|
| `--m 8\|4` | colour mode. Without it, each scene runs in 8 bpp, then in 4 bpp. |
| `--s 64\|32` | buffer size in KB. Without it, 64 KB, then 32 KB (see the modes below). |
| `--b N` | budget in video frames (default 1). |
| `--t N` | seconds per step (default 1.2 s; 3 s for scenes 1 and 10). |
| `--bench` | 60 frames per step, no pauses for the results. |
| `--only N` | only scene N. |
| `--frames F`, `--n N` | for the core's simulation: F frames per step, and a fixed N with no search for the maximum. |

At the end it prints the summary table, and it also writes it to `BENCH.TXT`.
The table has these columns: scene, bpp, KB, budget, result, FPS, ms per frame, CPU % and GPU %.

**The figures only mean something on the MiSTer and in the core's
simulation.** The GPU in alestelx is functional, not cycle-accurate: the demo
looks right there, but its timings don't.

## Modes

| `--m` | `--s` | Screen |
|---|---|---|
| 8 | 64 | 320×200, 256 colours (the main gaming mode) |
| 8 | 32 | 320×96 window |
| 4 | 32 | 320×200, 16 colours |
| 4 | 64 | 640×200, 16 colours |

The core's linear CRTC addressing has no stride: each line starts where the
previous one ended. So the 8 bpp 32 KB case is a smaller window rather than a
viewport inside a wider buffer. The microprograms still take the line pitch as
a parameter, apart from the screen width.

**Double buffering.** The buffers are at C50000h and C60000h. The program draws
into the back one, waits for VBlank (PIC line 2, polled while masked) and
swaps them by writing R19. R12/R13 are latched at the start of the frame, but
R19 takes effect at once, so it is written in the VBlank.

## How a frame works

The Z80 only organises the work. It sets the parameters (a block of 16 words),
starts the scene's microprogram and then the one for the status line, and
waits for VBlank. The GPU itself creates and moves everything: the objects, the
sprites and the tiles. So the Z80's share stays close to 0.

**Measuring.** The 1 MHz timer (F4h–F7h) measures each frame:

| Figure | What it covers |
|---|---|
| render | Z80 setup, the scene and the status line |
| GPU % | the GPU's time against the time on screen |
| CPU % | render − GPU, against the time on screen |
| FPS | frames per second actually shown |

**The budget rule.** A step fits if the average frame leaves 5% of the budget
free and the FPS of the budget hold (60/b on the MiSTer). In the scenes that
grow, N doubles while it fits. Then a binary search runs between N and 2N, and
the table shows both: the refined maximum, and the last power of two that fit.

## Scenes

| # | Scene | What it measures |
|---|---|---|
| 1 | Fill | Write bandwidth: 8-line bands that cycle colours, written in 16-bit words. Result in Mpix/s. |
| 2 | Lines | N horizontal lines over a background. They fall, and change colour when they wrap. |
| 3 | Rectangles | 2×2 to 32×32, falling, clipped at the top and bottom. |
| 4 | Characters | 1-bit glyphs (the BIOS 8×8 font) expanded to coloured pixels, N per frame. |
| 5 | Sprites | 16×16 to 128×128 balls with colour key 0. They move ±1 pixel and bounce, with half of each sprite allowed past the edge, so they get clipped. |
| 6 | Tiles | A full screen of opaque 8×8 tiles, copied in words, plus N colour-keyed tiles that scroll one tile every two frames. |
| 7 | Parallax | A gradient plus 1–3 colour-keyed tile layers at 1, 2 and 4 px per frame, each about 80% full and clipped at the screen edges. Result: the cost of each layer. |
| 8 | Rotation | 32×32 sprites rotating. Affine sampling is done in microcode, since the current channels only step in one dimension. |
| 9 | Scaling | The same sprite scaled from 16 to 96 pixels, clipped at the edges. |
| 10 | Line shift | An `.LXI` image (RISE, NEBULA or MANDEL) shifted line by line: bands go left and right, then the image moves up. Converted to 16 colours for 4 bpp. |

**Colour key.** A transparent pixel is not written. Instead, the destination is
read (`MOV.EQ ZERO, DATA0`), which only advances its index.

## Files

| Path | What it is |
|---|---|
| `gpudemo.c` | modes, measuring, budget search, summary |
| `scenes.c` | the scenes |
| `hw.c` | ports, timer, VBlank, CRTC, palette, GPU |
| `gpu/*.asm` | microprograms. They start with `common.inc` (the parameter block into R16–R31). `bgfill.inc` is the background, `subs.inc` holds `rnd` and `xy0` (pixel address in YI and XI), and `gen32.inc` and `objmove.inc` are shared by scenes 8 and 9. |
| `mkpkth.py` | assembles a microprogram (expanding `.include`) into a loader packet as a C array, `pk_*.h` |
| `build.sh` | runs mkpkth.py for every microprogram |

`make demo` builds `GPUDEMO.COM`: it needs python3, the LX-GPU assembler
(`deps/lx-gpu-rtl/build/asm`) and z88dk, and uses
`zcc +cpm -compiler=sdcc -SO3 gpudemo.c scenes.c hw.c -o GPUDEMO.COM -create-app`.

**Microcode notes:**

- **Channels v2.** The microprograms use the channels in linear mode (PITCH = 0): the pixel index is `{YI[2:0], XI}`, the step is DXI, and MODE_PAGE holds the format and the page. BANK stays at 0 (POS) all the time: a write to DXI (bank 1) or MODE_PAGE (bank 4) selects its bank and goes back to 0 right after, so the position writes in the loops need no bank change.
- **YI and XI.** Write both: unlike the old IDX, writing XI does not clear YI.
- **Packet sizes.** The packets are copied into fixed slots (`PK_SCENE_W`, `PK_OV_W`, `PK_MISC_W` words in `demo.h`); `PK_FITS` stops the build if a microprogram grows past its slot.
- **Page changes.** Before lx-gpu-rtl c5d4911, writing to PAGE the page the channel already had did not cancel a pending page change (A → B → A with no access in between left B). It is fixed there; the demo avoids the pattern anyway.
