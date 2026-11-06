; =============================================================================
; DOSGATE.ASM - Gateways between .COM programs and LX-DOS (Aleste LX)
; Shaeon (Carlos Palmero), for the AlesteLX_MiSTer core.
;
; Memory maps (supervisor slot 3):
;   LX-DOS    windows 0-2 = banks 20h, 21h, 22h   window 3 = ROM (drivers)
;   program   windows 0-2 = banks 24h, 25h, 26h   window 3 = ROM (drivers)
; Window 3 is the only shared one, so the gateways live here (DRV_CODE):
;
;   __dos_exec        (called from LX-DOS) installs the program map and jumps
;                     to 0100h with SP = BF00h and 0000h pushed on the stack.
;                     Returns when the program finishes (JP 0, RET to 0000h,
;                     or BDOS function 0).
;   __dos_bdos_gate   CALL 5 -> JP BF00h -> here. Saves C and DE, installs the
;                     LX-DOS map, and calls whatever routine LX-DOS left in
;                     __dos_bdos_vec (using the LX-DOS stack); returns with
;                     HL and A = L, B = H, CP/M-style.
;   __dos_bios_gate   the CP/M BIOS table (BF03h) arrives here with A = function
;                     number and C = character; calls __dos_bios_vec.
;   __dos_isr_gate    IM 1 while a program is running (its 0038h vector jumps
;                     here): installs the LX-DOS map, dispatches the interrupt,
;                     then restores the previous map.
;   __dos_wboot_gate  the program's 0000h: return to LX-DOS (after __dos_exec).
; =============================================================================

module dosgate

defc SYS_BANK0  = 0x20
defc SYS_BANK1  = 0x21
defc SYS_BANK2  = 0x22
defc TPA_BANK0  = 0x24
defc TPA_BANK1  = 0x25
defc TPA_BANK2  = 0x26
defc TPA_TOP    = 0xBF00

EXTERN __int_lx_dispatch

PUBLIC __dos_exec, __dos_bdos_gate, __dos_bios_gate, __dos_isr_gate, __dos_wboot_gate
PUBLIC __dos_bdos_vec, __dos_bios_vec, __dos_reg_c, __dos_reg_de, __dos_reg_a
PUBLIC __dos_xret, __dos_ret_a, __dos_ret_bc, __dos_ret_de, __dos_ret_hl, __dos_reg_hl, __dos_reg_b, __dos_reg_hl_b

SECTION DRV_CODE

; -----------------------------------------------------------------------------
; map_sys - Install the LX-DOS memory map (banks 20h/21h/22h in windows 0-2)
; -----------------------------------------------------------------------------
; Input:  None
; Output: None
; Clobbers: A
; -----------------------------------------------------------------------------
map_sys:
    ld a, SYS_BANK0
    out (0xFC), a
    ld a, SYS_BANK1
    out (0xFD), a
    ld a, SYS_BANK2
    out (0xFE), a
    ret

; Note: between switching the map and switching SP the stack is not used,
; because a stack in one map is not visible from the other.

; -----------------------------------------------------------------------------
; __dos_exec - Enter a .COM program from LX-DOS
; -----------------------------------------------------------------------------
; Assumes the program is already loaded into banks 24h-26h.
; Saves the LX-DOS SP, installs the program map, sets up the TPA stack with
; 0000h pushed so that a RET from the program returns to LX-DOS, and jumps to
; 0100h.
;
; Input:  None
; Output: Returns (via __dos_wboot_gate) when the program terminates
; Clobbers: A, SP (restored on return)
; -----------------------------------------------------------------------------
__dos_exec:
    di
    ld (exec_sp), sp            ; Save the LX-DOS SP
    ld a, TPA_BANK0             ; Install the program map
    out (0xFC), a
    ld a, TPA_BANK1
    out (0xFD), a
    ld a, TPA_BANK2
    out (0xFE), a
                                ; (LX-DOS stack is no longer visible: no stack here)
    ld sp, TPA_TOP              ; Program stack top
    ld hl, 0
    push hl                     ; Program's RET target = 0000h = return to LX-DOS
    ei
    jp 0x0100                   ; Enter the program

; -----------------------------------------------------------------------------
; __dos_wboot_gate - Program's 0000h vector: return to LX-DOS
; -----------------------------------------------------------------------------
; Reinstalls the LX-DOS map, restores the saved SP, and RETs back into
; __dos_exec (whose caller then continues).
;
; Input:  None (reached via RET-to-0000h, JP 0, or BDOS 0)
; Output: Returns to LX-DOS caller of __dos_exec
; Clobbers: A, SP
; -----------------------------------------------------------------------------
__dos_wboot_gate:
    di
    ld a, SYS_BANK0             ; Restore the LX-DOS map
    out (0xFC), a
    ld a, SYS_BANK1
    out (0xFD), a
    ld a, SYS_BANK2
    out (0xFE), a
    ld sp, (exec_sp)            ; Restore the LX-DOS SP
    ei
    ret                         ; Return from __dos_exec

; -----------------------------------------------------------------------------
; __dos_bdos_gate - BDOS gateway for CALL 5 (reached via JP BF00h)
; -----------------------------------------------------------------------------
; Input:  C = BDOS function, DE = parameter, HL = parameter, B = parameter
; Output: A = L, B = H (CP/M style); DE preserved for programs that need it.
;         If __dos_xret is set by LX-DOS, returns A/BC/DE/HL from __dos_ret_*.
; Clobbers: As per the called BDOS function
; -----------------------------------------------------------------------------
__dos_bdos_gate:
    di
    ld (prog_sp), sp            ; Save the program's SP
    ld (__dos_reg_de), de       ; Save DE (restored on return)
    ld (__dos_reg_hl), hl       ; Save HL (also used as __dos_reg_hl_b byte pair)
    ld a, b
    ld (__dos_reg_b), a         ; Save B
    ld a, c
    ld (__dos_reg_c), a         ; Save C (BDOS function number)
    or a
    jp z, __dos_wboot_gate      ; Function 0 = terminate program
    ld a, SYS_BANK0             ; Install LX-DOS map (without touching the stack)
    out (0xFC), a
    ld a, SYS_BANK1
    out (0xFD), a
    ld a, SYS_BANK2
    out (0xFE), a
    ld sp, (exec_sp)            ; LX-DOS stack, below its call to __dos_exec
    ei
    push ix                     ; Preserve the program's IX (LX-DOS C code uses IX)
    push iy
    ld hl, (__dos_bdos_vec)     ; LX-DOS BDOS entry point
    call jp_hl                  ; -> HL
    pop iy
    pop ix

; -----------------------------------------------------------------------------
; gate_back - Common return path: restore the program map and registers
; -----------------------------------------------------------------------------
gate_back:
    di
    ex de, hl                   ; Move the result to DE while switching maps
    ld a, TPA_BANK0             ; Install the program map
    out (0xFC), a
    ld a, TPA_BANK1
    out (0xFD), a
    ld a, TPA_BANK2
    out (0xFE), a
    ld sp, (prog_sp)            ; Restore the program's SP
    ld a, (__dos_xret)
    or a
    jr nz, gate_xret            ; LX-DOS requested a full-register return
    ex de, hl
    ld a, l                     ; A = L
    ld b, h                     ; B = H
    ld de, (__dos_reg_de)       ; DE is preserved (some programs rely on it)
    ei
    ret

; Full-register return (MSX-DOS functions that return A, BC, DE and HL at
; once): LX-DOS leaves the values in __dos_ret_* and sets __dos_xret.
gate_xret:
    xor a
    ld (__dos_xret), a          ; Clear the full-return flag
    ld bc, (__dos_ret_bc)
    ld de, (__dos_ret_de)
    ld hl, (__dos_ret_hl)
    ld a, (__dos_ret_a)
    ei
    ret

; -----------------------------------------------------------------------------
; __dos_bios_gate - CP/M BIOS gateway (table at BF03h)
; -----------------------------------------------------------------------------
; Input:  A = BIOS function number, C = character
; Output: As per the BIOS function
; Clobbers: As per the called BIOS function
; -----------------------------------------------------------------------------
__dos_bios_gate:
    di
    ld (prog_sp), sp
    ld (__dos_reg_a), a         ; Save A (function number)
    ld a, c
    ld (__dos_reg_c), a         ; Save C (character)
    ld a, (__dos_reg_a)
    cp 1
    jp z, __dos_wboot_gate      ; Function 1 = WBOOT (warm boot)
    ld a, SYS_BANK0             ; Install LX-DOS map
    out (0xFC), a
    ld a, SYS_BANK1
    out (0xFD), a
    ld a, SYS_BANK2
    out (0xFE), a
    ld sp, (exec_sp)            ; LX-DOS stack
    ei
    push ix                     ; Preserve the program's IX/IY
    push iy
    ld hl, (__dos_bios_vec)     ; LX-DOS BIOS entry point
    call jp_hl
    pop iy
    pop ix
    jr gate_back                ; Common return path

; -----------------------------------------------------------------------------
; __dos_isr_gate - IM 1 handler while a program is running
; -----------------------------------------------------------------------------
; The program's 0038h vector jumps here. Saves the current bank map, installs
; the LX-DOS map, dispatches the interrupt via __int_lx_dispatch, then restores
; the previous bank map and returns with RETI.
;
; Input:  None (invoked by the IM 1 interrupt)
; Output: Returns via RETI (interrupts restored)
; Clobbers: As per the dispatched handler
; -----------------------------------------------------------------------------
__dos_isr_gate:
    ld (isr_sp), sp             ; Save the interrupted SP
    ld sp, isr_stack_top        ; Switch to the dedicated ISR stack
    push af
    in a, (0xFC)                ; Read the current map (program's or LX-DOS's)
    push af
    in a, (0xFD)
    push af
    in a, (0xFE)
    push af
    call map_sys                ; Install the LX-DOS map
    call __int_lx_dispatch      ; LX-DOS and ROM routines are now visible
    pop af                      ; Restore the previous bank map
    out (0xFE), a
    pop af
    out (0xFD), a
    pop af
    out (0xFC), a
    pop af
    ld sp, (isr_sp)             ; Restore the interrupted SP
    ei
    reti

; -----------------------------------------------------------------------------
; jp_hl - Indirect jump to HL
; -----------------------------------------------------------------------------
; Input:  HL = target address
; Output: Does not return (jumps to HL)
; -----------------------------------------------------------------------------
jp_hl:
    jp (hl)

SECTION DRV_DATA

__dos_bdos_vec: dw 0            ; LX-DOS BDOS entry point (set by LX-DOS)
__dos_bios_vec: dw 0            ; LX-DOS BIOS entry point (set by LX-DOS)
__dos_reg_c:    db 0            ; Saved C (BDOS function / BIOS character)
__dos_reg_a:    db 0            ; Saved A (BIOS function number)
__dos_reg_de:   dw 0            ; Saved DE (restored on BDOS return)
__dos_reg_hl_b:                 ; Same as __dos_reg_hl, byte-accessed from C
__dos_reg_hl:   dw 0            ; Saved HL and B of the program (block/absolute functions)
__dos_reg_b:    db 0            ; Saved B
__dos_xret:     db 0            ; Non-zero = return A/BC/DE/HL from __dos_ret_*
__dos_ret_a:    db 0            ; Full-register return: A
__dos_ret_bc:   dw 0            ; Full-register return: BC
__dos_ret_de:   dw 0            ; Full-register return: DE
__dos_ret_hl:   dw 0            ; Full-register return: HL
exec_sp:        dw 0            ; LX-DOS SP saved by __dos_exec
prog_sp:        dw 0            ; Program SP saved by the gateways
isr_sp:         dw 0            ; Interrupted SP saved by __dos_isr_gate
isr_stack:      ds 256          ; Dedicated ISR stack
isr_stack_top:                  ; Top of the ISR stack