; s4_text.asm - GPUDEMO, escena 4: caracteres (fuente de 1 bit a color).
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Lo que cuesta de verdad escribir texto: cada caracter expande las 8 filas
; de su glifo de 1 bit (la fuente de la BIOS) a 64 pixeles del destino, con
; tinta y fondo. N caracteres por cuadro, uno detras de otro por la
; pantalla (debajo de la linea de estado); cuando se llena, vuelve a empezar
; desplazado 3 pixeles, encima de lo anterior. El caracter y su color
; cambian con el cuadro.
;
; P13 fondo (palabra), P14 tabla de 16 colores.
.org 0x20
        .include "common.inc"
        .include "bgfill.inc"
        MOV     R13, R29        ; fondo de los caracteres = el de la pantalla
        AND     R13, #15
        MOV     R0, R16         ; en 8 bpp, el byte entero
        SHR     R0, #8
        CMP     R0, #1
        JMP.NE  b4
        MOV     R13, R29
        AND     R13, #0xFF
b4:     LDI     R0, #0x0400     ; CH1: la fuente, 1 bpp (pagina P11)
        OR      R0, R27
        MOV     BANK, #4
        MOV     MODE_PAGE1, R0
        MOV     MODE_PAGE3, R26      ; CH3: tabla de colores
        MOV     BANK, #1
        MOV     DXI1, #1
        MOV     DXI3, #0
        MOV     BANK, #0
        MOV     R5, ZERO        ; x
        LDI     R6, #8          ; y
        MOV     R15, ZERO       ; desplazamiento de la capa
        MOV     R9, R20         ; caracteres
        MOV     R12, R21        ; caracter: empieza en el cuadro
chr:    ADD     R12, #7         ; siguiente caracter (33-126)
        CMP     R12, #94
        JMP.CC  c1
        SUB     R12, #94
c1:     MOV     R7, R12
        ADD     R7, #1
        MUL     R7, #64         ; primer pixel del glifo (desde el espacio)
        MOV     R0, R12         ; tinta de la tabla
        AND     R0, #15
        ADD     R0, R30
        MOV     YI3, #0
        MOV     XI3, R0
        MOV     R4, DATA3
        MOV     R8, ZERO        ; fila
grow:   MOV     YI1, #0       ; fuente: glifo + fila
        MOV     XI1, R7
        MOV     R1, R5
        MOV     R2, R6
        ADD     R2, R8
        CALL    xy0
        LDI     R0, #8
        MOV     LOOP_CNT, R0
        LDI     LOOP_TGT, #px
px:     MOV     R11, DATA1 !f
        MOV     R10, R13
        MOV.NE  R10, R4
      L MOV     DATA0, R10
        ADD     R7, #8
        ADD     R8, #1
        CMP     R8, #8
        JMP.NE  grow
        ADD     R5, #8          ; siguiente posicion
        MOV     R0, R5
        ADD     R0, #8
        CMP     R0, R18
        JMP.LS  nx
        MOV     R5, R15
        ADD     R6, #8
        MOV     R0, R6
        ADD     R0, #8
        CMP     R0, R19
        JMP.LS  nx
        ADD     R15, #3         ; pantalla llena: otra capa, desplazada
        AND     R15, #7
        MOV     R5, R15
        LDI     R6, #8
        ADD     R6, R15
nx:     SUB     R9, #1
        JMP.NE  chr
        FLUSH   0
        FLUSH   1
        FLUSH   3
        HALT    0
        .include "subs.inc"
