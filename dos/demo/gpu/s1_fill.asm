; s1_fill.asm - GPUDEMO, escena 1: relleno de la pantalla entera.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Lo mas barato que puede hacer la GPU: escribir. Franjas horizontales de 8
; lineas cuyo color avanza con el cuadro. Escribe palabras de 16 bits (2
; pixeles en 8 bpp, 4 en 4 bpp) linea a linea, respetando el ancho de linea
; del bufer.
;
; P13 palabras por linea del bufer, P14 palabras visibles por linea, P15
; indice de palabra (en la pagina P10) de 16 palabras de relleno, una por
; franja (el color repetido en los pixeles de la palabra).
.org 0x20
        .include "common.inc"
        AND     R16, #0xFF      ; 16 bpp, la pagina del destino
        MOV     PAGE0, R16
        MOV     IDXH0, #0
        MOV     STEP0, #1
        MOV     PAGE1, R26      ; CH1: la tabla de colores (16 bpp)
        MOV     STEP1, #0
        MOV     R1, ZERO        ; y
        MOV     R2, ZERO        ; primera palabra de la linea
row:    MOV     R3, R1          ; color = tabla[(y / 8 + cuadro) & 15]
        SHR     R3, #3
        ADD     R3, R21
        AND     R3, #15
        ADD     R3, R31
        MOV     IDXH1, #0
        MOV     IDX1, R3
        MOV     R3, DATA1
        MOV     IDX0, R2
        MOV     LOOP_CNT, R30
        LDI     LOOP_TGT, #fill
fill: L MOV     DATA0, R3
        ADD     R2, R29
        ADD     R1, #1
        CMP     R1, R19
        JMP.NE  row
        FLUSH   0
        FLUSH   1
        FLUSH   3
        HALT    0
