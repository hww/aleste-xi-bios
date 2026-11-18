; loop.asm - test_loop de lx-gpu-rtl (bucle por hardware de 10 vueltas) que
; ademas guarda R0 (debe valer 000Ah) en la palabra 0 de 800000h (16 bpp).
.org 0x20
        LDI     R0, #0           ; PC=0x020
        LDI     R1, #10          ; PC=0x021
        LDI     R2, #0x0025      ; PC=0x022: loop_body
        MOV     LOOP_TGT, R2     ; PC=0x023
        MOV     LOOP_CNT, R1     ; PC=0x024
loop_body:
        L ADD   R0, #1           ; PC=0x025
        JMP     store            ; PC=0x026 (HALT en el original)
store:
        LDI     R5, #0x0080      ; FMT=0 (16 bpp), PAGE=80h
        MOV     PAGE1, R5
        LDI     R5, #1
        MOV     STEP1, R5
        MOV     IDXH1, #0
        MOV     IDX1, #0
        MOV     DATA1, R0
        FLUSH   1
        HALT
