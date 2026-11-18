; fill.asm - prueba de la LX-GPU en alestelx: rellena 256 bytes en la
; direccion fisica 800000h con 5Ah (canal 1, 8 bpp, paso 1) y se detiene.
.org 0x20
        LDI     R5, #0x0180      ; FMT=1 (8 bpp), PAGE=80h
        MOV     PAGE1, R5
        LDI     R5, #1
        MOV     STEP1, R5
        MOV     IDXH1, #0
        MOV     IDX1, #0
        LDI     R2, #0x005A      ; el byte de relleno
        LDI     R1, #256         ; pixeles
        LDI     R3, #0x002B      ; inicio del cuerpo del bucle (fill_loop)
        MOV     LOOP_TGT, R3
        MOV     LOOP_CNT, R1
fill_loop:
        L MOV   DATA1, R2        ; PC=0x2B
        FLUSH   1
        HALT
