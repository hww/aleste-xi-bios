; outer.asm - test_outer_loop de lx-gpu-rtl (un CALL en un bucle por hardware,
; con otro bucle dentro de la subrutina, que guarda y repone LOOP_CNT, LOOP_TGT
; y SHADOW) y guarda R0 (debe valer 000Ch) en la palabra 1 de 800000h (16 bpp).
; Diferencia con el original: alli el cuerpo del bucle es "L CALL", pero la
; documentacion prohibe L en las palabras de control (y el hardware no lo
; atiende: R0 acaba en 4); aqui el bucle cierra con una instruccion de datos.
.org 0x20
        LDI     R0, #0           ; PC=0x020
        LDI     R1, #3           ; PC=0x021
        LDI     R2, #0x0025      ; PC=0x022: outer_loop
        MOV     LOOP_TGT, R2     ; PC=0x023
        MOV     LOOP_CNT, R1     ; PC=0x024
outer_loop:
        CALL    inner_func       ; PC=0x025
      L MOV     R13, R13         ; PC=0x026: fin del cuerpo del bucle
        JMP     store            ; PC=0x027 (HALT en el original)
inner_func:
        MOV     R10, LOOP_CNT    ; PC=0x028
        MOV     R11, LOOP_TGT    ; PC=0x029
        MOV     R12, SHADOW      ; PC=0x02A
        LDI     R3, #4           ; PC=0x02B
        LDI     R4, #0x002F      ; PC=0x02C: inner_loop
        MOV     LOOP_TGT, R4     ; PC=0x02D
        MOV     LOOP_CNT, R3     ; PC=0x02E
inner_loop:
      L ADD     R0, #1           ; PC=0x02F
        MOV     SHADOW, R12      ; PC=0x030
        MOV     LOOP_TGT, R11    ; PC=0x031
        MOV     LOOP_CNT, R10    ; PC=0x032
        RET                      ; PC=0x033
store:
        LDI     R5, #0x0080      ; FMT=0 (16 bpp), PAGE=80h
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        MOV     YI1, #0
        MOV     XI1, #1
        MOV     DATA1, R0
        FLUSH   1
        HALT
