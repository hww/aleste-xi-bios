; =============================================================================
; IRQSAVE.ASM - Disable and restore interrupts to their previous state
; Shaeon (Carlos Palmero), for the AlesteLX_MiSTer core.
;
;   call __irq_off ... call __irq_on
; __irq_on only re-enables interrupts if they were enabled when __irq_off was
; called (P/V flag of LD A,I = IFF2), so it also works correctly inside an
; interrupt service routine (ISR).
; A single variable is sufficient: while interrupts are disabled no other
; call can preempt the saved state.
; Both routines preserve all registers.
; =============================================================================

module irqsave

SECTION DRV_CODE

PUBLIC __irq_off, __irq_on

; -----------------------------------------------------------------------------
; __irq_off - Disable interrupts and remember the previous enable state
; -----------------------------------------------------------------------------
; Reads IFF2 via the P/V flag of "LD A,I", disables maskable interrupts, and
; stores whether interrupts were previously enabled.
;
; Input:  None
; Output: None
; Clobbers: None (all registers preserved)
; -----------------------------------------------------------------------------
__irq_off:
    push af
    ld a, i             ; P/V flag = IFF2 (previous interrupt enable state)
    di                  ; Disable maskable interrupts
    ld a, 0
    jp po, was_off      ; P/V clear -> interrupts were disabled
    inc a               ; P/V set   -> interrupts were enabled
was_off:
    ld (irq_was_on), a  ; Store previous state (0 = off, 1 = on)
    pop af
    ret

; -----------------------------------------------------------------------------
; __irq_on - Restore interrupts to the state saved by __irq_off
; -----------------------------------------------------------------------------
; Re-enables maskable interrupts only if they were enabled when __irq_off was
; called. Safe to call from within an ISR.
;
; Input:  None
; Output: None
; Clobbers: None (all registers preserved)
; -----------------------------------------------------------------------------
__irq_on:
    push af
    ld a, (irq_was_on)
    or a
    jr z, keep_off      ; Saved state = 0 -> keep interrupts disabled
    ei                  ; Saved state = 1 -> re-enable interrupts
keep_off:
    pop af
    ret

SECTION DRV_DATA
irq_was_on: db 0