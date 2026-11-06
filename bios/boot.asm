; =============================================================================
; BOOT.ASM - Boot a system loaded from disk (LXDOS.SYS)
; Shaeon (Carlos Palmero), for the AlesteLX_MiSTer core.
;
; The ROM loads the system into slot 3, banks 20h-22h (C80000-C8AFFF) and
; calls __boot_jump, which lives in the resident part (window 3, C000-FFFF):
; with interrupts disabled, it installs banks 20h-22h into windows 0-2 and
; jumps to 0000h. Window 3 keeps the drivers, which the system uses through
; their fixed addresses (ABI). Video does not occupy any window (the console
; draws via DMA).
; =============================================================================

module boot

defc SYS_BANK0 = 0x20
defc SYS_BANK1 = 0x21
defc SYS_BANK2 = 0x22

SECTION DRV_CODE

; -----------------------------------------------------------------------------
; __boot_jump - Start the system loaded from disk
; -----------------------------------------------------------------------------
; Called by the ROM after loading LXDOS.SYS into banks 20h-22h. Disables
; interrupts, installs the system banks into windows 0-2 (window 3 keeps the
; drivers), and jumps to 0000h (system entry point). The system stack lives
; in the upper part of window 2.
;
; Input:  None
; Output: Does not return (jumps to 0000h)
; Clobbers: A
; -----------------------------------------------------------------------------
PUBLIC __boot_jump
__boot_jump:
    di
    ld a, SYS_BANK0
    out (0xFC), a               ; window 0
    ld a, SYS_BANK1
    out (0xFD), a               ; window 1
    ld a, SYS_BANK2
    out (0xFE), a               ; window 2 (system stack, upper part)
    jp 0x0000

; -----------------------------------------------------------------------------
; __boot_rom - Return to the ROM (monitor)
; -----------------------------------------------------------------------------
; Restores windows 0-2 to BIOS banks 0-2 and jumps to 0000h, as on a reset.
;
; Input:  None
; Output: Does not return (jumps to 0000h)
; Clobbers: A
; -----------------------------------------------------------------------------
PUBLIC __boot_rom
__boot_rom:
    di
    xor a
    out (0xFC), a               ; window 0 = bank 0
    inc a
    out (0xFD), a               ; window 1 = bank 1
    inc a
    out (0xFE), a               ; window 2 = bank 2
    jp 0x0000