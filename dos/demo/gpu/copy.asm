; copy.asm - GPUDEMO: copia palabras de 16 bits de una pagina a otra.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; P13 pagina de origen, P14 primera palabra del origen, P3 pagina de
; destino, P1 primera palabra del destino, P2 numero de palabras.
.org 0x20
        .include "common.inc"
        MOV     PAGE1, R29
        MOV     STEP1, #1
        MOV     IDXH1, #0
        MOV     IDX1, R30
        MOV     PAGE0, R19
        MOV     STEP0, #1
        MOV     IDXH0, #0
        MOV     IDX0, R17
        MOV     LOOP_CNT, R18
        LDI     LOOP_TGT, #cp
cp:     MOV     R0, DATA1
      L MOV     DATA0, R0
        FLUSH   0
        FLUSH   1
        HALT    0
