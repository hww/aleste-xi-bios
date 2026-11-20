; gen32.inc - GPUDEMO: con P6 bit 1, crea el sprite de 32x32 de las escenas
; 8 y 9 en la pagina P9 + 4 (formato del destino): un circulo con un
; tablero de 4x4 colores (tabla P14) y un borde blanco (color 15), para que
; se vea que gira. Fuera del circulo, 0 (transparente). Usa R0-R9 y CH0/CH3.
        TST     R22, #2
        JMP.EQ  g32x
        MOV     R0, R16
        AND     R0, #-256
        OR      R0, R25
        ADD     R0, #4
        MOV     PAGE0, R0
        MOV     STEP0, #1
        MOV     IDXH0, #0
        MOV     IDX0, ZERO
        MOV     PAGE3, R26
        MOV     STEP3, #0
        MOV     R2, ZERO
g32y:   MOV     R1, ZERO
g32p:   MOV     R3, R1          ; d2 = (2x - 31)^2 + (2y - 31)^2
        ADD     R3, R1
        SUB     R3, #31
        MUL     R3, R3
        MOV     R4, R2
        ADD     R4, R2
        SUB     R4, #31
        MUL     R4, R4
        ADD     R3, R4
        MOV     R7, ZERO
        CMP     R3, #961        ; 31^2: fuera
        JMP.CS  g32w
        LDI     R7, #15
        CMP     R3, #729        ; 27^2: borde
        JMP.CS  g32w
        MOV     R0, R1          ; tablero
        SHR     R0, #3
        MOV     R5, R2
        SHR     R5, #3
        MUL     R5, #4
        ADD     R0, R5
        ADD     R0, R30
        MOV     IDXH3, #0
        MOV     IDX3, R0
        MOV     R7, DATA3
g32w:   MOV     DATA0, R7
        ADD     R1, #1
        CMP     R1, #32
        JMP.NE  g32p
        ADD     R2, #1
        CMP     R2, #32
        JMP.NE  g32y
        FLUSH   0
        FLUSH   3
g32x:
