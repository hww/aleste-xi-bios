; bgfill.inc - GPUDEMO: fondo de un color (palabra P13) en todo el bufer de
; atras, linea a linea, en palabras de 16 bits. Deja CH0 en el formato del
; destino (P0), con STEP 1. Usa R0-R4.
        MOV     R0, R16
        AND     R0, #0xFF
        MOV     PAGE0, R0
        MOV     STEP0, #1
        MOV     R1, R17         ; palabras por linea del bufer
        MOV     R2, R18         ; palabras visibles
        MOV     R0, R16
        SHR     R0, #8          ; 1 = 8 bpp (2 pixeles por palabra), 2 = 4 bpp (4)
        SHR     R1, #1
        SHR     R2, #1
        CMP     R0, #1
        JMP.EQ  bg_w
        SHR     R1, #1
        SHR     R2, #1
bg_w:   MOV     R3, ZERO
        MOV     R4, R19
bg_row: MOV     IDXH0, #0
        MOV     IDX0, R3
        MOV     LOOP_CNT, R2
        LDI     LOOP_TGT, #bg_px
bg_px: L MOV    DATA0, R29
        ADD     R3, R1
        SUB     R4, #1
        JMP.NE  bg_row
        FLUSH   0
        MOV     PAGE0, R16
        MOV     STEP0, #1
