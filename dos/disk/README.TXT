LX-DOS boot disk (Aleste LX).

Tests of .COM programs:
  HELLO               hello world
  FILETEST            CP/M 2.2 files by FCB
  MSXTEST             MSX-DOS 1 functions (date, time, blocks, sectors)
  ZEXDOC, ZEXALL      Z80 instruction exercisers

Music and graphics:
  MODPLAY LXDEMO.MOD  music in the background (MODPLAY /S stops it)
  GFX8TEST, TUNNEL    320x200x256 mode
  VIEW                shows the .LXI images (a key: next one; Esc quits)

Supervisor and user mode (the syscall protocol):
  SYSTEST             OUT (F2h),A syscalls from user mode, exit on RET,
                      interrupt hold (press keys during its 1 s wait)
  MAPTEST, MAPTEST2   which map each instruction is fetched from when
                      the MMU switches maps
  VBTEST              VBlank interrupts with a program running
  FDCINT              the floppy INT on PIC line 0 (expects 0101010)
