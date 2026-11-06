; ==============================================================================
; STANDARD ERROR HANDLERS
; ==============================================================================
include "../inc/bios/errors.inc"

; ------------------------------------------------------------------------------
; Success return (public symbol)
; Clears carry flag and returns A = 0 to indicate no error.
; Exit: A = 0, Carry = 0
; ------------------------------------------------------------------------------
PUBLIC _err_success
_err_success:
    xor a           ; A = 0
    and a           ; Clear carry flag (redundant after XOR A, kept for clarity)
    ret

; ------------------------------------------------------------------------------
; "Not supported" error (public symbol)
; Exit: A = ERR_NOT_SUPPORTED, Carry = 1
; ------------------------------------------------------------------------------
PUBLIC _err_not_supported
_err_not_supported:
    ld a, ERR_NOT_SUPPORTED
    scf             ; Set carry flag to signal error
    ret

; ------------------------------------------------------------------------------
; "Device not found" error (public symbol)
; Exit: A = ERR_NO_DEVICE, Carry = 1
; ------------------------------------------------------------------------------
PUBLIC _err_no_device
_err_no_device:
    ld a, ERR_NO_DEVICE
    scf             ; Set carry flag to signal error
    ret

; ------------------------------------------------------------------------------
; "Timeout" error (public symbol)
; Exit: A = ERR_TIMEOUT, Carry = 1
; ------------------------------------------------------------------------------
PUBLIC _err_timeout
_err_timeout:
    ld a, ERR_TIMEOUT
    scf             ; Set carry flag to signal error
    ret

; ------------------------------------------------------------------------------
; Generic error setter (public symbol)
; Input:  A = error code
; Output: A = same error code, Carry = 1
; ------------------------------------------------------------------------------
PUBLIC _err_set
_err_set:
    scf             ; Set carry flag to signal error
    ret