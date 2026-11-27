; s9_scale.asm - GPUDEMO, escena 9: sprites que crecen y menguan (afin).
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; La mecanica de la escena 5 con el sprite de 32x32 a escala: cada sprite
; recorre una tabla de 32 tamanos (de 16 a 96 pixeles y vuelta), y cada
; pixel de destino lee el del sprite en u = columna * (32 / lado), en coma
; fija 8.8. Al crecer pasan de los bordes y se recortan.
;
; Objetos (pagina P8, 16 bpp, 4 palabras): x, y (centro), direccion, fase.
; P7 indice de palabra de la tabla de escalas: 32 pares (lado, paso 8.8).
; P13 fondo.
; P14 tabla de 16 colores, P15 semilla.
.org 0x20
        .include "common.inc"
        .include "gen32.inc"
        .include "bgfill.inc"
        MOV     BANK, #4
        MOV     MODE_PAGE1, R24
        MOV     MODE_PAGE2, R24
        MOV     BANK, #1
        MOV     DXI1, #1
        MOV     DXI2, #1
        MOV     BANK, #0
        MOV     R12, R31
        TST     R22, #1
        JMP.EQ  draw
        MOV     YI2, #0
        MOV     XI2, ZERO
        MOV     R9, R20
ini:    MOV     R13, R18
        CALL    rnd
        MOV     DATA2, R14
        MOV     R13, R19
        CALL    rnd
        MOV     DATA2, R14
        MOV     R0, R12
        SHR     R0, #6
        AND     R0, #3
        MOV     DATA2, R0
        MOV     R0, R12
        SHR     R0, #3
        AND     R0, #31
        MOV     DATA2, R0
        SUB     R9, #1
        JMP.NE  ini
        FLUSH   2
draw:   MOV     YI1, #0
        MOV     XI1, ZERO
        MOV     YI2, #0
        MOV     XI2, ZERO
        LDI     R15, #16        ; rebote con el centro a 16 del borde
        MOV     R9, R20
spr:    MOV     R5, DATA1
        MOV     R6, DATA1
        MOV     R7, DATA1
        MOV     R8, DATA1
        .include "objmove.inc"
        MOV     DATA2, R5
        MOV     DATA2, R6
        MOV     DATA2, R7
        MOV     DATA2, R8
        ; escala = tabla[(fase + cuadro / 2) & 31]: lado y paso
        MOV     R0, R21
        SHR     R0, #1
        ADD     R0, R8
        AND     R0, #31
        ADD     R0, R0
        ADD     R0, R23
        FLUSH   3
        MOV     BANK, #4
        MOV     MODE_PAGE3, R26
        MOV     BANK, #1
        MOV     DXI3, #1
        MOV     BANK, #0
        MOV     YI3, #0
        MOV     XI3, R0
        MOV     R10, DATA3      ; lado
        MOV     R11, DATA3      ; paso (8.8)
        MOV     R0, R16
        AND     R0, #-256
        OR      R0, R25
        ADD     R0, #4
        MOV     BANK, #4
        MOV     MODE_PAGE3, R0       ; el sprite
        MOV     BANK, #1
        MOV     DXI3, #0
        MOV     BANK, #0
        MOV     R13, R10        ; media caja
        SHR     R13, #1
        MOV     R1, R5          ; x0, columnas saltadas
        SUB     R1, R13
        MOV     R3, ZERO
        CMP     R1, #0
        JMP.GE  k1
        SUB     R3, R1
        MOV     R1, ZERO
k1:     MOV     R7, R5
        SUB     R7, R13
        ADD     R7, R10
        CMP     R7, R18
        JMP.LE  k2
        MOV     R7, R18
k2:     SUB     R7, R1          ; ancho visible
        JMP.LE  nexts
        MOV     R2, R6
        SUB     R2, R13
        MOV     R4, ZERO
        CMP     R2, #0
        JMP.GE  k3
        SUB     R4, R2
        MOV     R2, ZERO
k3:     MOV     R8, R6
        SUB     R8, R13
        ADD     R8, R10
        CMP     R8, R19
        JMP.LE  k4
        MOV     R8, R19
k4:     SUB     R8, R2          ; filas visibles
        JMP.LE  nexts
        MUL     R3, R11         ; u0 = saltadas * paso
        MUL     R4, R11         ; v = saltadas * paso
        MOV     R31, R11        ; paso (xy0 usa R10/R11)
        MOV     R12, R7
row:    MOV     R14, R4         ; fila del sprite: (v >> 8) * 32
        SHR     R14, #8
        MUL     R14, #32
        CALL    xy0
        MOV     R13, R3         ; u
        MOV     R7, R12
px:     MOV     R0, R13
        SHR     R0, #8
        ADD     R0, R14
        MOV     YI3, #0
        MOV     XI3, R0
        MOV     R0, DATA3 !f
        MOV.NE  DATA0, R0
        MOV.EQ  ZERO, DATA0
        ADD     R13, R31
        SUB     R7, #1
        JMP.NE  px
        ADD     R4, R31
        ADD     R2, #1
        SUB     R8, #1
        JMP.NE  row
nexts:  SUB     R9, #1
        JMP.NE  spr
        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
        .include "subs.inc"
