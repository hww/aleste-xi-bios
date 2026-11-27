LX-DOS boot disk (Aleste LX).

Command line: Left/Right/Home/End/Del edit the line, Esc clears it,
Up/Down recall the last 8 commands, Tab completes commands and file
names (twice: the list). HELP lists the commands.

Tests of .COM programs:
  HELLO               hello world
  FILETEST            CP/M 2.2 files by FCB
  MSXTEST             MSX-DOS 1 functions (date, time, blocks, sectors)
  ZEXDOC, ZEXALL      Z80 instruction exercisers

Music and graphics:
  MODPLAY LXDEMO.MOD  music in the background (MODPLAY /S stops it)
  MAG file            plays an STM or MOD with a screen: a tribute to MAG,
                      the Magic Sound player of the Aleste 520EX (by
                      hww, H2W Lab), in colour and drawn by the LX-GPU
                      (stars, meters). No extension: .STM, then .MOD.
                      Any key stops it. Songs from the original MAG disk:
                      MAG AXELF, MAG SLL1, MAG HEADACHE (MAGSONGS.DSK in
                      B: has all five: MAG B:CHRUSH, MAG B:TECHNO1).
                      MAG LXDEMO also works.
  GFX8TEST, TUNNEL    320x200x256 mode
  VIEW                shows the .LXI images (a key: next one; Esc quits)

Supervisor and user mode (the syscall protocol):
  SYSTEST             OUT (F2h),A syscalls from user mode, exit on RET,
                      interrupt hold (press keys during its 1 s wait)
  MAPTEST, MAPTEST2   leaving the supervisor on a RET: which map the code
                      after it and the RET's stack come from (expects 2)
  VBTEST              VBlank interrupts with a program running
  TIMTEST             the system timer (rev 6, F4h-F6h): rate, snapshots
                      per mode, the comparator firing once (OK)
  FDCINT              the floppy INT on PIC line 0 (expects 0101010)

LX-GPU:
  GPUTEST             loads a microcode packet and fills 256 bytes at
                      800000h; expects 'GPU status: 80' and OK
  GPULOOP             hardware loops: 10 turns, and 3 x CALL x 4
                      (R0 = 000A and 000C)
  GPUCHW              channel writes and reads in 8 bpp, 4 bpp,
                      16 bpp and 8 bpp STEP 2; shows 800000h-80008Fh
  GPUSPR              16x16 sprite onto a 320x200 8 bpp screen (CH0 to
                      CH1); expects 'wrong bytes 00'
  GPUIRQ              the GPU interrupt (PIC line 5) and IRQ_CODE
  (all of them end with OK)
