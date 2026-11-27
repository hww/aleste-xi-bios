; s8_rotate.asm - GPUDEMO, escena 8: sprites que giran (muestreo afin).
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; La mecanica de la escena 5 (color clave, rebote, recorte en los bordes)
; con sprites de 32x32 que giran: cada pixel de la caja de destino (46x46,
; la diagonal) se lleva al sprite con
;   u = cos * dx + sin * dy + 16,   v = -sin * dx + cos * dy + 16
; en coma fija 8.8, sumando cos y sin pixel a pixel. Si (u, v) cae fuera
; del sprite o en un pixel 0, no se escribe. Sin ayuda del hardware: la GPU
; de ahora solo avanza el indice de un canal en una dimension.
;
; Objetos (pagina P8, 16 bpp, 4 palabras): x, y (centro), direccion, fase y
; velocidad de giro (fase | velocidad << 8). P7 indice de palabra de la
; tabla de senos (64 entradas, 8.8), P13 fondo, P14 tabla de 16 colores, P15 semilla.
.org 0x20
        .include "common.inc"
        .include "gen32.inc"
        .include "bgfill.inc"
        MOV     BANK, #4
        MOV     MODE_PAGE1, R24
        MOV     MODE_PAGE2, R24
        MOV     BANK, #1
        MOV     DXI1, #1
        MOV     DXI2, #1
        MOV     BANK, #0
        MOV     R12, R31
        TST     R22, #1
        JMP.EQ  draw
        MOV     YI2, #0
        MOV     XI2, ZERO
        MOV     R9, R20
ini:    MOV     R13, R18
        CALL    rnd
        MOV     DATA2, R14
        MOV     R13, R19
        CALL    rnd
        MOV     DATA2, R14
        MOV     R0, R12
        SHR     R0, #6
        AND     R0, #3
        MOV     DATA2, R0
        MOV     R0, R12         ; fase 0-63, velocidad 1-2
        SHR     R0, #3
        AND     R0, #63
        MOV     R1, R12
        AND     R1, #256
        ADD     R1, #256
        OR      R0, R1
        MOV     DATA2, R0
        SUB     R9, #1
        JMP.NE  ini
        FLUSH   2
draw:   MOV     R0, R16         ; CH3: el sprite (pagina P9 + 4)
        AND     R0, #-256
        OR      R0, R25
        ADD     R0, #4
        MOV     BANK, #4
        MOV     MODE_PAGE3, R0
        MOV     BANK, #1
        MOV     DXI3, #0
        MOV     BANK, #0
        MOV     YI1, #0
        MOV     XI1, ZERO
        MOV     YI2, #0
        MOV     XI2, ZERO
        LDI     R15, #23        ; media caja
        MOV     R9, R20
spr:    MOV     R5, DATA1
        MOV     R6, DATA1
        MOV     R7, DATA1
        MOV     R8, DATA1
        .include "objmove.inc"
        MOV     DATA2, R5
        MOV     DATA2, R6
        MOV     DATA2, R7
        MOV     DATA2, R8
        ; angulo = (fase + cuadro * velocidad) & 63: seno y coseno de la tabla
        MOV     R0, R8
        SHR     R0, #8
        MUL     R0, R21
        ADD     R0, R8
        AND     R0, #63
        MOV     R1, R0
        ADD     R1, #16
        AND     R1, #63
        ADD     R0, R23
        ADD     R1, R23
        FLUSH   3
        MOV     BANK, #4
        MOV     MODE_PAGE3, R26      ; (la tabla de senos, en la pagina de los datos)
        MOV     BANK, #0
        MOV     YI3, #0
        MOV     XI3, R0
        MOV     R10, DATA3      ; sin
        MOV     YI3, #0
        MOV     XI3, R1
        MOV     R11, DATA3      ; cos
        MOV     R0, R16
        AND     R0, #-256
        OR      R0, R25
        ADD     R0, #4
        MOV     BANK, #4
        MOV     MODE_PAGE3, R0       ; vuelve al sprite (con un acceso en medio)
        MOV     BANK, #0
        ; caja recortada: x de [cx-23, cx+23) dentro de [0, ancho)
        MOV     R1, R5
        SUB     R1, R15         ; x0
        MOV     R3, ZERO        ; columnas saltadas
        CMP     R1, #0
        JMP.GE  k1
        SUB     R3, R1
        MOV     R1, ZERO
k1:     MOV     R7, R5
        ADD     R7, R15
        CMP     R7, R18
        JMP.LE  k2
        MOV     R7, R18
k2:     SUB     R7, R1          ; ancho visible
        JMP.LE  nexts
        MOV     R2, R6
        SUB     R2, R15         ; y0
        MOV     R4, ZERO        ; filas saltadas
        CMP     R2, #0
        JMP.GE  k3
        SUB     R4, R2
        MOV     R2, ZERO
k3:     MOV     R8, R6
        ADD     R8, R15
        CMP     R8, R19
        JMP.LE  k4
        MOV     R8, R19
k4:     SUB     R8, R2          ; filas visibles
        JMP.LE  nexts
        ; dx0 = saltadas - 23, dy = saltadas - 23
        SUB     R3, R15
        SUB     R4, R15
        MOV     R31, R7         ; ancho visible (R7 se reutiliza)
row:    MOV     R13, R11        ; u0 = cos * dx0 + sin * dy + 16.5
        MUL     R13, R3
        MOV     R0, R10
        MUL     R0, R4
        ADD     R13, R0
        LDI     R0, #4224
        ADD     R13, R0
        MOV     R14, R11        ; v0 = cos * dy - sin * dx0 + 16.5
        MUL     R14, R4
        MOV     R0, R10
        MUL     R0, R3
        SUB     R14, R0
        LDI     R0, #4224
        ADD     R14, R0
        MOV     R7, R10         ; xy0 usa R10/R11
        MOV     R12, R11
        CALL    xy0
        MOV     R10, R7
        MOV     R11, R12
        MOV     R7, R31
px:     MOV     R0, R13         ; u
        SAR     R0, #8
        CMP     R0, #32
        JMP.CS  skip
        MOV     R12, R14        ; v
        SAR     R12, #8
        CMP     R12, #32
        JMP.CS  skip
        MUL     R12, #32
        ADD     R12, R0
        MOV     YI3, #0
        MOV     XI3, R12
        MOV     R0, DATA3 !f
        JMP.EQ  skip
        MOV     DATA0, R0
        JMP     pnext
skip:   MOV     ZERO, DATA0
pnext:  ADD     R13, R11
        SUB     R14, R10
        SUB     R7, #1
        JMP.NE  px
        ADD     R2, #1
        ADD     R4, #1
        SUB     R8, #1
        JMP.NE  row
nexts:  SUB     R9, #1
        JMP.NE  spr
        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
        .include "subs.inc"
