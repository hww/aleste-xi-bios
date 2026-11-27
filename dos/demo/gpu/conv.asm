; conv.asm - GPUDEMO: pasa una imagen de 8 bpp a 4 bpp con una tabla.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Cada pixel (0-255) se cambia por tabla[pixel] (0-15): el color de la
; paleta de 16 mas parecido, que calcula GPUDEMO. P13 pagina de origen
; (8 bpp), P3 pagina de destino (4 bpp), P2 numero de pixeles, P14 indice
; de byte de la tabla (256 bytes, pagina P10).
.org 0x20
        .include "common.inc"
        LDI     R0, #0x0100     ; CH1: origen, 8 bpp
        OR      R0, R29
        MOV     BANK, #4
        MOV     MODE_PAGE1, R0
        MOV     BANK, #1
        MOV     DXI1, #1
        MOV     BANK, #0
        MOV     YI1, #0
        MOV     XI1, ZERO
        LDI     R0, #0x0200     ; CH0: destino, 4 bpp
        OR      R0, R19
        MOV     BANK, #4
        MOV     MODE_PAGE0, R0
        MOV     BANK, #1
        MOV     DXI0, #1
        MOV     BANK, #0
        MOV     YI0, #0
        MOV     XI0, ZERO
        LDI     R0, #0x0100     ; CH2: la tabla, 8 bpp
        OR      R0, R26
        MOV     BANK, #4
        MOV     MODE_PAGE2, R0
        MOV     BANK, #1
        MOV     DXI2, #0
        MOV     BANK, #0
        MOV     LOOP_CNT, R18
        LDI     LOOP_TGT, #cv
cv:     MOV     R0, DATA1
        ADD     R0, R30
        MOV     YI2, #0
        MOV     XI2, R0
        MOV     R0, DATA2
      L MOV     DATA0, R0
        FLUSH   0
        FLUSH   1
        FLUSH   2
        HALT    0
