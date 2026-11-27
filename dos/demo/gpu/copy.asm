; copy.asm - GPUDEMO: copia palabras de 16 bits de una pagina a otra.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; P13 pagina de origen, P14 primera palabra del origen, P3 pagina de
; destino, P1 primera palabra del destino, P2 numero de palabras.
.org 0x20
        .include "common.inc"
        MOV     BANK, #4
        MOV     MODE_PAGE1, R29
        MOV     MODE_PAGE0, R19
        MOV     BANK, #1
        MOV     DXI1, #1
        MOV     DXI0, #1
        MOV     BANK, #0
        MOV     YI1, #0
        MOV     XI1, R30
        MOV     YI0, #0
        MOV     XI0, R17
        MOV     LOOP_CNT, R18
        LDI     LOOP_TGT, #cp
cp:     MOV     R0, DATA1
      L MOV     DATA0, R0
        FLUSH   0
        FLUSH   1
        HALT    0
