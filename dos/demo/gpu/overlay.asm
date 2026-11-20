; overlay.asm - GPUDEMO: una linea de texto con la fuente 8x8 de la BIOS.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Expande los glifos de 1 bit a pixeles del formato del destino (8 o 4 bpp),
; con tinta y fondo: la linea de estado de cada cuadro y los resultados.
; La fuente es la de la BIOS, que GPUDEMO copia al principio de la pagina
; P11: el indice de un canal es un numero de 19 bits con signo, asi que en
; 1 bpp solo llega a los primeros 32 KB de una pagina (la de la ROM esta en
; C0E000h). IDXH se escribe siempre antes que IDX (escribir IDX lo usa y lo
; pone a 0).
;
; P0 destino, P1 ancho de linea, P2 indice de byte del texto (ASCIIZ, en la
; pagina P10), P11 pagina de la fuente, P13 x, P14 y, P15 tinta | fondo << 8.
.org 0x20
        .include "common.inc"
        MOV     R13, R31        ; tinta
        AND     R13, #0xFF
        MOV     R14, R31        ; fondo
        SHR     R14, #8
        MOV     PAGE0, R16      ; CH0: destino
        MOV     STEP0, #1
        LDI     R0, #0x0400     ; CH1: la fuente, 1 bpp
        OR      R0, R27
        MOV     PAGE1, R0
        MOV     STEP1, #1
        LDI     R0, #0x0100     ; CH2: el texto, 8 bpp
        OR      R0, R26
        MOV     PAGE2, R0
        MOV     IDXH2, #0
        MOV     STEP2, #1
        MOV     IDX2, R18
        MOV     R5, R29         ; x
chr:    MOV     R7, DATA2 !f    ; caracter (0 = fin)
        JMP.EQ  done
        SUB     R7, #32
        MUL     R7, #64         ; primer pixel del glifo
        MOV     R8, ZERO        ; fila
grow:   MOV     IDXH1, #0       ; fuente: glifo + fila
        MOV     IDX1, R7
        MOV     R9, R30         ; destino: (y + fila) * ancho + x
        ADD     R9, R8
        MUL     R9, R17
        MOV     R10, MHI
        ADD     R9, R5
        ADC     R10, ZERO
        MOV     IDXH0, R10
        MOV     IDX0, R9
        LDI     R0, #8
        MOV     LOOP_CNT, R0
        LDI     LOOP_TGT, #px
px:     MOV     R11, DATA1 !f
        MOV     R12, R14
        MOV.NE  R12, R13
      L MOV     DATA0, R12
        ADD     R7, #8
        ADD     R8, #1
        CMP     R8, #8
        JMP.NE  grow
        ADD     R5, #8
        JMP     chr
done:   FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
