; s5_sprites.asm - GPUDEMO, escena 5: sprites con color clave.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; N sprites de P7 x P7 pixeles (bolas, cuatro colores) que se mueven 1 pixel
; por cuadro en cada eje y rebotan; pueden salir media bola por cada borde,
; asi que se recortan. El color 0 es transparente: por cada pixel, se lee el
; sprite y, si no es 0, se escribe; si es 0, se lee el destino para avanzar
; su indice sin escribir (MOV.EQ ZERO, DATA0).
;
; Con P6 bit 1, antes crea las cuatro bolas en las paginas P9..P9+3 (en el
; formato del destino): sombreado por la distancia a un brillo arriba a la
; izquierda. P7 = lado | desplazamiento del sombreado << 8.
; Objetos (pagina P8, 16 bpp, 4 palabras): x, y, direccion (bit 0 x hacia la
; izquierda, bit 1 y hacia arriba), bola (0-3).
; Ojo: CH3 no vuelve a la tabla entre sprites. En ch_cache, escribir en PAGE la
; pagina que ya tiene el canal no anula un cambio de pagina pendiente (de otra
; escritura anterior sin acceso en medio): A -> B -> A seguidos deja B.
; P13 fondo, P14 tabla: 4 colores x 3 tonos (oscuro, medio, claro) en 4 bpp
; o 4 bases de rampa en 8 bpp, P15 semilla.
.org 0x20
        .include "common.inc"
        MOV     R15, R23        ; desplazamiento del sombreado
        SHR     R15, #8
        AND     R23, #0xFF      ; lado
        MOV     PAGE3, R26      ; CH3: tabla
        MOV     STEP3, #0
        MOV     R12, R31
        TST     R22, #2
        JMP.EQ  noimg
        ; ---- crear las cuatro bolas
        MOV     STEP3, #1       ; la tabla se lee seguida
        MOV     R9, ZERO        ; bola
gimg:   MOV     R0, R16         ; CH0 en la pagina P9 + bola, formato del destino
        AND     R0, #-256
        ADD     R0, R25
        ADD     R0, R9
        MOV     PAGE0, R0
        MOV     STEP0, #1
        MOV     IDXH0, #0
        MOV     IDX0, ZERO
        MOV     R2, ZERO        ; y
gy:     MOV     R1, ZERO        ; x
gx:     MOV     R3, R1          ; dx = 2x + 1 - lado
        ADD     R3, R1
        ADD     R3, #1
        SUB     R3, R23
        MOV     R4, R2          ; dy
        ADD     R4, R2
        ADD     R4, #1
        SUB     R4, R23
        MOV     R5, R3          ; d2 = dx^2 + dy^2
        MUL     R5, R3
        MOV     R6, R4
        MUL     R6, R4
        ADD     R5, R6
        MOV     R6, R23         ; dentro si d2 < lado^2
        MUL     R6, R23
        MOV     R7, ZERO        ; transparente
        CMP     R5, R6
        JMP.CS  gput
        MOV     R5, R23         ; brillo: distancia a (lado/3, lado/3)
        ADD     R3, R5          ; dx + lado*2/3 (en unidades de medio pixel)
        SHR     R5, #1
        SUB     R3, R5
        ADD     R4, R23
        SUB     R4, R5
        MUL     R3, R3
        MUL     R4, R4
        ADD     R3, R4
        SHR     R3, R15         ; tono 0 (claro) .. 15+
        CMP     R3, #15
        JMP.CC  g1
        LDI     R3, #15
g1:     MOV     R0, R9          ; tabla: base de la bola
        MUL     R0, #4
        ADD     R0, R30
        MOV     IDXH3, #0
        MOV     IDX3, R0
        MOV     R7, DATA3       ; 8 bpp: base de la rampa; 4 bpp: oscuro
        MOV     R0, R16
        SHR     R0, #8
        CMP     R0, #1
        JMP.NE  g4
        ADD     R7, #15         ; 8 bpp: base + 15 - tono
        SUB     R7, R3
        JMP     gput
g4:     CMP     R3, #10         ; 4 bpp: oscuro, medio o claro
        JMP.CS  gput
        MOV     R7, DATA3
        CMP     R3, #2
        JMP.CS  gput
        MOV     R7, DATA3
gput:   MOV     DATA0, R7
        ADD     R1, #1
        CMP     R1, R23
        JMP.NE  gx
        ADD     R2, #1
        CMP     R2, R23
        JMP.NE  gy
        FLUSH   0
        ADD     R9, #1
        CMP     R9, #4
        JMP.NE  gimg
noimg:  .include "bgfill.inc"
        MOV     PAGE1, R24      ; CH1/CH2: objetos
        MOV     PAGE2, R24
        MOV     STEP1, #1
        MOV     STEP2, #1
        TST     R22, #1
        JMP.EQ  draw
        ; ---- crear N sprites
        MOV     IDXH2, #0
        MOV     IDX2, ZERO
        MOV     R9, R20
ini:    MOV     R13, R18        ; x: -lado/2 .. ancho - lado/2
        CALL    rnd
        MOV     R0, R23
        SHR     R0, #1
        SUB     R14, R0
        MOV     DATA2, R14
        MOV     R13, R19        ; y
        CALL    rnd
        SUB     R14, R0
        MOV     DATA2, R14
        MOV     R0, R12
        SHR     R0, #6
        AND     R0, #3
        MOV     DATA2, R0       ; direccion
        MOV     R0, R12
        SHR     R0, #9
        AND     R0, #3
        MOV     DATA2, R0       ; bola
        SUB     R9, #1
        JMP.NE  ini
        FLUSH   2
        ; ---- mover y dibujar
draw:   MOV     IDXH1, #0
        MOV     IDX1, ZERO
        MOV     IDXH2, #0
        MOV     IDX2, ZERO
        MOV     R9, R20
        MOV     R15, R23        ; R15 = lado / 2
        SHR     R15, #1
spr:    MOV     R5, DATA1       ; x
        MOV     R6, DATA1       ; y
        MOV     R7, DATA1       ; direccion
        MOV     R8, DATA1       ; bola
        ; x: +-1 y rebote en [-lado/2, ancho - lado/2]
        TST     R7, #1
        JMP.NE  xl
        ADD     R5, #1
        MOV     R0, R18
        SUB     R0, R15
        CMP     R5, R0
        JMP.LT  ydir
        XOR     R7, #1
        JMP     ydir
xl:     SUB     R5, #1
        MOV     R0, ZERO
        SUB     R0, R15
        CMP     R5, R0
        JMP.GT  ydir
        XOR     R7, #1
ydir:   TST     R7, #2
        JMP.NE  yu
        ADD     R6, #1
        MOV     R0, R19
        SUB     R0, R15
        CMP     R6, R0
        JMP.LT  mvd
        XOR     R7, #2
        JMP     mvd
yu:     SUB     R6, #1
        MOV     R0, ZERO
        SUB     R0, R15
        CMP     R6, R0
        JMP.GT  mvd
        XOR     R7, #2
mvd:    MOV     DATA2, R5
        MOV     DATA2, R6
        MOV     DATA2, R7
        MOV     DATA2, R8
        ; recorte: columnas [max(x,0), min(x+lado, ancho)), filas igual
        MOV     R3, ZERO        ; sx: primera columna del sprite
        MOV     R1, R5          ; x0 en pantalla
        CMP     R1, #0
        JMP.GE  k1
        SUB     R3, R1
        MOV     R1, ZERO
k1:     MOV     R10, R5         ; x1
        ADD     R10, R23
        CMP     R10, R18
        JMP.LE  k2
        MOV     R10, R18
k2:     SUB     R10, R1         ; ancho visible
        JMP.LE  nexts
        MOV     R4, ZERO        ; sy
        MOV     R2, R6          ; y0
        CMP     R2, #0
        JMP.GE  k3
        SUB     R4, R2
        MOV     R2, ZERO
k3:     MOV     R11, R6         ; y1
        ADD     R11, R23
        CMP     R11, R19
        JMP.LE  k4
        MOV     R11, R19
k4:     SUB     R11, R2         ; filas visibles
        JMP.LE  nexts
        MOV     R0, R16         ; CH3: la bola (pagina P9 + bola, formato del destino)
        AND     R0, #-256
        ADD     R0, R25
        ADD     R0, R8
        MOV     PAGE3, R0
        MOV     STEP3, #1
        MOV     R13, R4         ; primer pixel de la bola: sy * lado + sx
        MUL     R13, R23
        ADD     R13, R3
        MOV     R7, R10         ; R7 = ancho visible (la direccion ya esta guardada)
srow:   MOV     IDXH3, #0
        MOV     IDX3, R13
        MOV     R14, R10        ; xy0 usa R10/R11: guardar
        MOV     R8, R11
        CALL    xy0
        MOV     R10, R14
        MOV     R11, R8
        MOV     LOOP_CNT, R7
        LDI     LOOP_TGT, #sp
sp:     MOV     R0, DATA3 !f
        MOV.NE  DATA0, R0
      L MOV.EQ  ZERO, DATA0
        ADD     R13, R23
        ADD     R2, #1
        SUB     R11, #1
        JMP.NE  srow
nexts:  SUB     R9, #1
        JMP.NE  spr
        FLUSH   0
        FLUSH   1
        FLUSH   2
        FLUSH   3
        HALT    0
        .include "subs.inc"
