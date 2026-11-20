; clear.asm - GPUDEMO: llena un bufer entero con una palabra de 16 bits.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; P0 destino (se usa la pagina, en 16 bpp), P13 palabra, P14 numero de palabras.
.org 0x20
        .include "common.inc"
        AND     R16, #0xFF
        MOV     PAGE0, R16
        MOV     IDXH0, #0
        MOV     STEP0, #1
        MOV     IDX0, ZERO
        MOV     LOOP_CNT, R30
        LDI     LOOP_TGT, #fill
fill: L MOV     DATA0, R29
        FLUSH   0
        FLUSH   3
        HALT    0
