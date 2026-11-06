; ==============================================================================
; MODULE: UNDEFINED METHOD HANDLER
; ==============================================================================
module undefined_method

SECTION DRV_CODE

; ------------------------------------------------------------------------------
; _undefined_method - Handler for calls to an undefined (uninitialized) method
; ------------------------------------------------------------------------------
; Called when executed code invokes a method that has not been initialized.
; The handler halts the CPU to prevent execution from continuing in an
; undefined state.
;
; Input:  None (reached via an indirect call / vector dispatch)
; Output: Does not return (CPU halted until NMI or reset)
; Clobbers: None (interrupts are disabled, CPU stopped)
; ------------------------------------------------------------------------------

PUBLIC _undefined_method

_undefined_method:
    di                    ; Disable interrupts (maskable)
    halt                  ; Stop the CPU until NMI or reset occurs

    ; --------------------------------------------------------------------------
    ; Alternative: more advanced version (diagnostics + graceful halt)
    ; --------------------------------------------------------------------------
    ; call _save_cpu_state  ; Snapshot the register/flag state
    ; call _print_error     ; Print an error message to the console
    ; call _dump_stack      ; Dump the stack (if possible)
    ; jp   _system_halt     ; Transfer control to the system halt handler

    ; --------------------------------------------------------------------------
    ; Alternative: minimal KISS version
    ; --------------------------------------------------------------------------
    ; di
    ; halt

    ; --------------------------------------------------------------------------
    ; Alternative: debug output (blink LED or emit code to debug port)
    ; --------------------------------------------------------------------------
    ; ld  a, 0x55
    ; out (0x00), a         ; Send the error code to the debug port
    ; jr  $                 ; Infinite loop