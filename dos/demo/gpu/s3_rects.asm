; s3_rects.asm - GPUDEMO, escena 3: lluvia de rectangulos de un tamano.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Relleno de bloques sin transparencia: N cuadrados de P7 x P7 pixeles que
; caen 1-2 pixeles por cuadro; al salir por abajo vuelven por arriba con otro
; color. Se recortan por arriba y por abajo (entran y salen poco a poco).
;
; Objetos (pagina P8, 16 bpp, 4 palabras): x, y (con signo), color (indice de
; la tabla) | velocidad << 8, libre. P13 fondo, P14 tabla de 16 colores,
; P15 semilla.
.org 0x20
        .include "common.inc"
        .include "bgfill.inc"
        MOV     BANK, #4
        MOV     MODE_PAGE1, R24
        MOV     MODE_PAGE2, R24
        MOV     MODE_PAGE3, R26
        MOV     BANK, #1
        MOV     DXI1, #1
        MOV     DXI2, #1
        MOV     DXI3, #0
        MOV     BANK, #0
        MOV     R12, R31
        TST     R22, #1
        JMP.EQ  draw
        ; ---- crear N cuadrados
        MOV     YI2, #0
        MOV     XI2, ZERO
        MOV     R9, R20
ini:    MOV     R13, R18        ; x: 0 .. ancho - lado
        SUB     R13, R23
        CALL    rnd
        MOV     DATA2, R14
        MOV     R13, R19        ; y: 0 .. alto
        CALL    rnd
        MOV     DATA2, R14
        MOV     R8, R12         ; color, velocidad 1-2
        SHR     R8, #8
        AND     R8, #15
        MOV     R0, R12
        SHR     R0, #4
        AND     R0, #1
        ADD     R0, #1
        MUL     R0, #256
        OR      R8, R0
        MOV     DATA2, R8
        MOV     DATA2, ZERO
        SUB     R9, #1
        JMP.NE  ini
        FLUSH   2
        ; ---- mover y dibujar
draw:   MOV     YI1, #0
        MOV     XI1, ZERO
        MOV     YI2, #0
        MOV     XI2, ZERO
        MOV     R9, R20
obj:    MOV     R5, DATA1       ; x
        MOV     R6, DATA1       ; y
        MOV     R8, DATA1       ; color | velocidad
        MOV     R0, DATA1
        MOV     R0, R8
        SHR     R0, #8
        ADD     R6, R0          ; cae
        CMP     R6, R19
        JMP.LT  inside
        MOV     R6, ZERO        ; vuelve por arriba (y = -lado) con otro color
        SUB     R6, R23
        MOV     R0, R8
        ADD     R0, #1
        AND     R0, #15
        AND     R8, #-16
        OR      R8, R0
inside: MOV     DATA2, R5
        MOV     DATA2, R6
        MOV     DATA2, R8
        MOV     DATA2, ZERO
        AND     R8, #15         ; color de la tabla
        ADD     R8, R30
        MOV     YI3, #0
        MOV     XI3, R8
        MOV     R8, DATA3
        MOV     R7, R6          ; filas visibles: [max(y,0), min(y+lado, alto))
        ADD     R7, R23
        CMP     R7, R19
        JMP.LT  c1
        MOV     R7, R19
c1:     MOV     R2, R6
        CMP     R2, #0
        JMP.GE  c2
        MOV     R2, ZERO
c2:     SUB     R7, R2
        JMP.LE  next
        MOV     R1, R5
rows:   CALL    xy0
        MOV     LOOP_CNT, R23
        LDI     LOOP_TGT, #px
px:   L MOV     DATA0, R8
        ADD     R2, #1
        SUB     R7, #1
        JMP.NE  rows
next:   SUB     R9, #1
        JMP.NE  obj
        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
        .include "subs.inc"
