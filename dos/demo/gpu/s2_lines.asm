; s2_lines.asm - GPUDEMO, escena 2: lineas horizontales sobre un fondo.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; El coste de muchas ordenes pequenas: N lineas horizontales de colores que
; caen 1-3 pixeles por cuadro y vuelven por arriba (con otro color). Cada
; cuadro: fondo y las N lineas, pixel a pixel en el formato del destino.
;
; Objetos (pagina P8, 16 bpp, 4 palabras): x, y, largo, color | velocidad << 8
; (el color es un indice de la tabla de 16).
; P13 palabra del fondo, P14 indice de palabra (pagina P10) de 16 colores,
; P15 semilla.
.org 0x20
        .include "common.inc"
        .include "bgfill.inc"
        MOV     PAGE1, R24      ; CH1 lee los objetos, CH2 los escribe
        MOV     PAGE2, R24
        MOV     STEP1, #1
        MOV     STEP2, #1
        MOV     PAGE3, R26      ; CH3: la tabla de colores
        MOV     STEP3, #0
        MOV     R12, R31        ; semilla
        TST     R22, #1
        JMP.EQ  draw
        ; ---- crear N lineas
        MOV     IDXH2, #0
        MOV     IDX2, ZERO
        MOV     R9, R20
ini:    MOV     R13, R18        ; largo: 8 .. ancho/2
        SHR     R13, #1
        CALL    rnd
        MOV     R7, R14
        ADD     R7, #8
        MOV     R13, R18        ; x: 0 .. ancho - largo
        SUB     R13, R7
        CALL    rnd
        MOV     R5, R14
        MOV     R13, R19        ; y
        CALL    rnd
        MOV     R6, R14
        MOV     R8, R12         ; color (de la tabla), velocidad 1-2
        SHR     R8, #8
        AND     R8, #15
        MOV     R0, R12
        SHR     R0, #4
        AND     R0, #1
        ADD     R0, #1
        MUL     R0, #256
        OR      R8, R0
        MOV     DATA2, R5
        MOV     DATA2, R6
        MOV     DATA2, R7
        MOV     DATA2, R8
        SUB     R9, #1
        JMP.NE  ini
        FLUSH   2
        ; ---- mover y dibujar
draw:   MOV     IDXH1, #0
        MOV     IDX1, ZERO
        MOV     IDXH2, #0
        MOV     IDX2, ZERO
        MOV     R9, R20
line:   MOV     R5, DATA1       ; x
        MOV     R6, DATA1       ; y
        MOV     R7, DATA1       ; largo
        MOV     R8, DATA1       ; color | velocidad
        MOV     R0, R8
        SHR     R0, #8
        ADD     R6, R0          ; cae
        CMP     R6, R19
        JMP.CC  inside
        SUB     R6, R19         ; vuelve por arriba con otro color
        MOV     R0, R8
        ADD     R0, #1
        AND     R0, #15
        AND     R8, #-16
        OR      R8, R0
inside: MOV     DATA2, R5
        MOV     DATA2, R6
        MOV     DATA2, R7
        MOV     DATA2, R8
        AND     R8, #15         ; color de la tabla
        ADD     R8, R30
        MOV     IDXH3, #0
        MOV     IDX3, R8
        MOV     R8, DATA3
        MOV     R1, R5
        MOV     R2, R6
        CALL    xy0
        MOV     LOOP_CNT, R7
        LDI     LOOP_TGT, #px
px:   L MOV     DATA0, R8
        SUB     R9, #1
        JMP.NE  line
        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
        .include "subs.inc"
