; frame.asm - microprograma de MAG.COM para la LX-GPU: un cuadro.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; MAG lo lanza 50 veces por segundo. Hace dos cosas en la pantalla de 8 bpp
; (C50000h, 320 x 200):
;
; 1. Mueve las estrellas del fondo hacia la izquierda. Cada estrella son
;    4 palabras: x (en 1/16 de pixel), y, velocidad y color (32-35). Se borra
;    solo si su pixel sigue siendo de su color, y se pinta solo sobre el
;    fondo (color 0), asi que pasa por detras del texto. Al salir por la
;    izquierda vuelve por la derecha a una altura al azar (congruencial).
; 2. Pinta los cuatro medidores del recuadro: barras de 6 filas con el ancho
;    que deja MAG (0-256, multiplo de 4), en degradado (colores 64-127), el
;    resto con el color del interior del recuadro (2) y una marca blanca (1)
;    en el pico.
;
; Datos (pagina 94h, palabras de 16 bits; el Z80 los ve en 6000h con el
; banco 50h): 1000h ancho y pico de los canales 0-3 (8 palabras), 1008h la
; semilla, 1010h las estrellas.
.org 0x20
        LDI     R0, #0x0094     ; 16 bpp, pagina 94h
        MOV     PAGE2, R0       ; CH2 lee los datos
        MOV     PAGE3, R0       ; CH3 los escribe
        MOV     IDXH2, #0
        MOV     IDXH3, #0
        MOV     STEP2, #1
        MOV     STEP3, #1
        LDI     R0, #0x01C5     ; 8 bpp, pagina C5h
        MOV     PAGE0, R0       ; CH0: un pixel cada vez (lee y escribe)
        MOV     PAGE1, R0       ; CH1: filas de los medidores
        MOV     IDXH0, #0
        MOV     IDXH1, #0
        MOV     STEP0, #0
        MOV     STEP1, #1

        ; ---- 1. estrellas
        LDI     R0, #0x1008
        MOV     IDX2, R0
        MOV     R20, DATA2      ; semilla
        LDI     R24, #25173     ; R20 = R20 * 25173 + 13849
        LDI     R25, #13849
        LDI     R23, #5120      ; 320 * 16
        LDI     R0, #0x1010
        MOV     IDX2, R0
        MOV     R21, R0         ; donde se escribe la estrella
        LDI     R22, #160         ; estrellas (NSTARS en mag.asm)
star:   MOV     R1, DATA2       ; x * 16
        MOV     R2, DATA2       ; y
        MOV     R3, DATA2       ; velocidad
        MOV     R4, DATA2       ; color
        MOV     R5, R2          ; la borra si sigue alli
        MUL     R5, #320
        MOV     R6, R1
        SHR     R6, #4
        ADD     R5, R6
        MOV     IDX0, R5
        MOV     R7, DATA0
        CMP     R7, R4
        MOV.EQ  DATA0, ZERO
        SUB     R1, R3          ; la mueve
        JMP.PL  same
        ADD     R1, R23         ; por la derecha, a otra altura
        MUL     R20, R24
        ADD     R20, R25
        MOV     R2, R20
        SHR     R2, #8
        MUL     R2, #200
        SHR     R2, #8
same:   MOV     R5, R2          ; la pinta sobre el fondo
        MUL     R5, #320
        MOV     R6, R1
        SHR     R6, #4
        ADD     R5, R6
        MOV     IDX0, R5
        MOV     R7, DATA0 !f
        MOV.EQ  DATA0, R4
        MOV     IDX3, R21       ; guarda x e y
        MOV     DATA3, R1
        MOV     DATA3, R2
        ADD     R21, #4
        SUB     R22, #1
        JMP.NE  star
        LDI     R0, #0x1008
        MOV     IDX3, R0
        MOV     DATA3, R20

        ; ---- 2. medidores
        LDI     R0, #0x1000
        MOV     IDX2, R0
        LDI     R10, #42276       ; barra del canal 0: 132 * 320 + 36
        LDI     R11, #4         ; canales
        LDI     R19, #2         ; interior del recuadro
        LDI     R9, #1          ; marca del pico
chan:   MOV     R12, DATA2      ; ancho
        MOV     R13, DATA2      ; pico
        MOV     R14, R10
        LDI     R15, #6         ; filas
row:    MOV     IDX1, R14
        LDI     R16, #64        ; color del primer grupo de 4 pixeles
        MOV     R17, R12
        SHR     R17, #2         ; grupos llenos
        JMP.EQ  empty
        MOV     LOOP_CNT, R17
        LDI     LOOP_TGT, #full
full:   MOV     DATA1, R16
        MOV     DATA1, R16
        MOV     DATA1, R16
        MOV     DATA1, R16
      L ADD     R16, #1 !nf
empty:  LDI     R18, #64
        SUB     R18, R17        ; grupos vacios
        JMP.EQ  peak
        MOV     LOOP_CNT, R18
        LDI     LOOP_TGT, #rest
rest:   MOV     DATA1, R19
        MOV     DATA1, R19
        MOV     DATA1, R19
      L MOV     DATA1, R19
peak:   MOV     R0, R13 !f
        JMP.EQ  nextr
        MOV     R5, R14
        ADD     R5, R13
        MOV     IDX1, R5
        MOV     DATA1, R9
        MOV     DATA1, R9
nextr:  ADD     R14, #320
        SUB     R15, #1
        JMP.NE  row
        LDI     R0, #3520       ; 11 filas mas abajo
        ADD     R10, R0
        SUB     R11, #1
        JMP.NE  chan

        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
