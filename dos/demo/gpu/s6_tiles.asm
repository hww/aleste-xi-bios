; s6_tiles.asm - GPUDEMO, escena 6: tiles y scroll de juego.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; El coste de redibujar una pantalla de tiles: una capa de fondo opaca de
; tiles de 8x8 (ladrillos) que cubre la pantalla entera, copiada en palabras
; de 16 bits, y encima N tiles con color clave (gemas) que se desplazan un
; tile (8 pixeles) cada dos cuadros y dan la vuelta por los lados.
;
; Con P6 bit 1 crea los 16 tiles en la pagina P9 (formato del destino,
; tile t en el pixel t * 64): 0-7 ladrillos, 8-15 gemas (rombo, 0 fuera).
; Objetos (pagina P8, 16 bpp, 4 palabras): columna, fila, tile (8-15), libre.
; P13 color del mortero, P14 tabla de 16 colores (0-7 ladrillo, 8-15 gema),
; P15 semilla.
.org 0x20
        .include "common.inc"
        MOV     R15, R16        ; R15 = desplazamiento pixeles -> palabras: 1 o 2
        SHR     R15, #8
        MOV     PAGE3, R26      ; CH3: tabla
        MOV     STEP3, #0
        MOV     R12, R31
        TST     R22, #2
        JMP.EQ  noimg
        ; ---- crear los 16 tiles
        MOV     R0, R16
        AND     R0, #-256
        OR      R0, R25
        MOV     PAGE0, R0
        MOV     STEP0, #1
        MOV     IDXH0, #0
        MOV     IDX0, ZERO
        MOV     R9, ZERO        ; tile
gt:     MOV     R0, R9          ; color del tile
        ADD     R0, R30
        MOV     IDXH3, #0
        MOV     IDX3, R0
        MOV     R8, DATA3
        MOV     R2, ZERO        ; y
gy:     MOV     R1, ZERO        ; x
gx:     CMP     R9, #8
        JMP.CS  gem
        MOV     R7, R8          ; ladrillo: mortero en y % 4 == 3 y en una x
        MOV     R0, R2
        AND     R0, #3
        CMP     R0, #3
        JMP.EQ  mort
        MOV     R0, R2          ; x de la junta: 7, o 3 en la otra hilada
        AND     R0, #4
        ADD     R0, R1
        AND     R0, #7
        CMP     R0, #7
        JMP.NE  gput
mort:   MOV     R7, R29
        JMP     gput
gem:    MOV     R3, R1          ; |2x - 7| + |2y - 7|
        ADD     R3, R1
        SUB     R3, #7
        JMP.PL  g1
        MVN     R3, R3
        ADD     R3, #1
g1:     MOV     R4, R2
        ADD     R4, R2
        SUB     R4, #7
        JMP.PL  g2
        MVN     R4, R4
        ADD     R4, #1
g2:     ADD     R3, R4
        MOV     R7, ZERO
        CMP     R3, #9
        JMP.CS  gput
        MOV     R7, R8
        CMP     R3, #3
        JMP.CS  gput
        MOV     R7, R29         ; brillo en el centro
gput:   MOV     DATA0, R7
        ADD     R1, #1
        CMP     R1, #8
        JMP.NE  gx
        ADD     R2, #1
        CMP     R2, #8
        JMP.NE  gy
        ADD     R9, #1
        CMP     R9, #16
        JMP.NE  gt
        FLUSH   0
noimg:  MOV     R13, R18        ; columnas y filas de tiles
        SHR     R13, #3
        MOV     R14, R19
        SHR     R14, #3
        ; ---- crear N gemas
        TST     R22, #1
        JMP.EQ  base
        MOV     PAGE2, R24
        MOV     STEP2, #1
        MOV     IDXH2, #0
        MOV     IDX2, ZERO
        MOV     R9, R20
        MOV     R10, R13        ; rnd usa R13/R14: guardar
        MOV     R11, R14
ini:    MOV     R13, R10
        CALL    rnd
        MOV     DATA2, R14
        MOV     R13, R11
        CALL    rnd
        MOV     DATA2, R14
        MOV     R0, R12
        SHR     R0, #5
        AND     R0, #7
        ADD     R0, #8
        MOV     DATA2, R0
        MOV     DATA2, ZERO
        SUB     R9, #1
        JMP.NE  ini
        FLUSH   2
        MOV     R13, R10
        MOV     R14, R11
        ; ---- capa de fondo: palabras de 16 bits
base:   MOV     R0, R16         ; CH2: destino en 16 bpp
        AND     R0, #0xFF
        MOV     PAGE2, R0
        MOV     STEP2, #1
        MOV     PAGE1, R25      ; CH1: los tiles en 16 bpp
        MOV     STEP1, #1
        LDI     R7, #8          ; palabras por fila de tile: 8 >> desplazamiento
        SHR     R7, R15
        MOV     R6, ZERO        ; fila de tiles
brow:   MOV     R5, ZERO        ; columna
bcol:   MOV     R0, R5          ; tile = (col * 3 + fila * 5 + col / 4) & 7
        MUL     R0, #3
        MOV     R1, R6
        MUL     R1, #5
        ADD     R0, R1
        MOV     R1, R5
        SHR     R1, #2
        ADD     R0, R1
        AND     R0, #7
        MUL     R0, #64         ; primer pixel del tile
        SHR     R0, R15         ; a palabras
        MOV     R2, R6          ; primera palabra del destino:
        MUL     R2, #8          ;   fila * 8 * palabras por linea + col * palabras por fila
        MOV     R1, R17
        SHR     R1, R15
        MUL     R2, R1
        MOV     R1, R5
        MUL     R1, R7
        ADD     R2, R1
shd:    MOV     R8, ZERO        ; fila del tile
trow:   MOV     IDXH1, #0
        MOV     IDX1, R0
        MOV     IDXH2, #0
        MOV     IDX2, R2
        MOV     LOOP_CNT, R7
        LDI     LOOP_TGT, #cp
cp:     MOV     R1, DATA1
      L MOV     DATA2, R1
        ADD     R0, R7
        MOV     R1, R17         ; + ancho de linea en palabras
        SHR     R1, R15
        ADD     R2, R1
        ADD     R8, #1
        CMP     R8, #8
        JMP.NE  trow
        ADD     R5, #1
        CMP     R5, R13
        JMP.NE  bcol
        ADD     R6, #1
        CMP     R6, R14
        JMP.NE  brow
        FLUSH   2
        ; ---- gemas con color clave, desplazadas (cuadro / 2) columnas
        MOV     PAGE0, R16
        MOV     STEP0, #1
        MOV     R0, R16
        AND     R0, #-256
        OR      R0, R25
        MOV     PAGE3, R0       ; CH3: tiles en el formato del destino
        MOV     STEP3, #1
        MOV     PAGE1, R24      ; CH1: objetos
        MOV     IDXH1, #0
        MOV     IDX1, ZERO
        MOV     R15, R21        ; desplazamiento: (cuadro / 2) mod columnas
        SHR     R15, #1
mod:    CMP     R15, R13
        JMP.CC  modd
        SUB     R15, R13
        JMP     mod
modd:   MOV     R9, R20
gemd:   MOV     R5, DATA1       ; columna
        MOV     R6, DATA1       ; fila
        MOV     R7, DATA1       ; tile
        MOV     R0, DATA1
        ADD     R5, R15
        CMP     R5, R13
        JMP.CC  g3
        SUB     R5, R13
g3:     MOV     R4, R7          ; primer pixel del tile
        MUL     R4, #64
        MOV     R1, R5
        MUL     R1, #8
        MOV     R2, R6
        MUL     R2, #8
        MOV     R8, ZERO
grw:    MOV     IDXH3, #0
        MOV     IDX3, R4
        CALL    xy0
        LDI     R0, #8
        MOV     LOOP_CNT, R0
        LDI     LOOP_TGT, #kp
kp:     MOV     R0, DATA3 !f
        MOV.NE  DATA0, R0
      L MOV.EQ  ZERO, DATA0
        ADD     R4, #8
        ADD     R2, #1
        ADD     R8, #1
        CMP     R8, #8
        JMP.NE  grw
        SUB     R9, #1
        JMP.NE  gemd
        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
        .include "subs.inc"
