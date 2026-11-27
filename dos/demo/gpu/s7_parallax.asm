; s7_parallax.asm - GPUDEMO, escena 7: parallax y recorte.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Un fondo de juego de varias capas: un degradado (en palabras de 16 bits) y
; P7 capas (0-3) de tiles de 8x8 con color clave (las gemas de la escena 6),
; que dan la vuelta y se desplazan 1, 2 y 4 pixeles por cuadro. Cada capa es
; un mapa de 64 columnas con un tile en ~80% de las casillas. Como el
; desplazamiento no es multiplo de 8, los tiles de los bordes se recortan
; (por la izquierda y por la derecha) pixel a pixel.
;
; Usa los tiles de la escena 6 (pagina P9). P14 tabla de 16 palabras de
; relleno para el degradado (una por franja de 8 lineas, ciclica).
.org 0x20
        .include "common.inc"
        MOV     R15, R16        ; 1 (8 bpp) o 2 (4 bpp): pixeles -> palabras
        SHR     R15, #8
        ; ---- fondo: degradado por franjas, en palabras
        MOV     R0, R16
        AND     R0, #0xFF
        MOV     BANK, #4
        MOV     MODE_PAGE0, R0
        MOV     MODE_PAGE3, R26
        MOV     BANK, #1
        MOV     DXI0, #1
        MOV     DXI3, #0
        MOV     BANK, #0
        MOV     R1, R17         ; palabras por linea
        SHR     R1, R15
        MOV     R2, R18         ; palabras visibles
        SHR     R2, R15
        MOV     R3, ZERO
        MOV     R4, ZERO        ; y
bgr:    MOV     R5, R4
        SHR     R5, #3
        AND     R5, #15
        ADD     R5, R30
        MOV     YI3, #0
        MOV     XI3, R5
        MOV     R5, DATA3
        MOV     YI0, #0
        MOV     XI0, R3
        MOV     LOOP_CNT, R2
        LDI     LOOP_TGT, #bgp
bgp:  L MOV     DATA0, R5
        ADD     R3, R1
        ADD     R4, #1
        CMP     R4, R19
        JMP.NE  bgr
        FLUSH   0
        ; ---- capas
        MOV     BANK, #4
        MOV     MODE_PAGE0, R16
        MOV     BANK, #1
        MOV     DXI0, #1
        MOV     BANK, #0
        MOV     R0, R16
        AND     R0, #-256
        OR      R0, R25
        MOV     BANK, #4
        MOV     MODE_PAGE3, R0       ; CH3: tiles
        MOV     BANK, #1
        MOV     DXI3, #1
        MOV     BANK, #0
        MOV     R13, R18        ; R13 = columnas visibles
        SHR     R13, #3
        MOV     R9, ZERO        ; capa
layer:  CMP     R9, R23
        JMP.CS  done
        LDI     R0, #1          ; velocidad 1, 2, 4
        CMP     R9, #1
        JMP.NE  sp1
        LDI     R0, #2
sp1:    CMP     R9, #2
        JMP.NE  sp2
        LDI     R0, #4
sp2:    MUL     R0, R21         ; desplazamiento = cuadro * velocidad, mod 512
        AND     R0, #511
        MOV     R14, R0         ; R14 = columna del mapa (>> 3), R15 lo dejamos
        SHR     R14, #3
        AND     R0, #7
        MOV     R31, R0         ; R31 = pixeles del primer tile fuera por la izquierda
        MOV     R6, ZERO        ; fila de tiles
lrow:   MOV     R5, ZERO        ; k: tile visible 0..columnas
lcol:   MOV     R0, R14         ; columna del mapa
        ADD     R0, R5
        AND     R0, #63
        MUL     R0, #37         ; h = col * 37 + fila * 11 + capa * 101
        MOV     R1, R6
        MUL     R1, #11
        ADD     R0, R1
        MOV     R1, R9
        MUL     R1, #101
        ADD     R0, R1
        AND     R0, #255
        MOV     R1, R0
        AND     R1, #15
        CMP     R1, #13
        JMP.CS  lnext           ; casilla vacia (~19%)
        SHR     R0, #4          ; tile 8-15
        AND     R0, #7
        ADD     R0, #8
        MUL     R0, #64
        MOV     R4, R0          ; primer pixel del tile
        MOV     R1, R5          ; x = k * 8 - recorte
        MUL     R1, #8
        SUB     R1, R31
        LDI     R7, #8          ; ancho visible
        CMP     R1, #0
        JMP.GE  lx1
        ADD     R4, R31         ; recorte por la izquierda
        SUB     R7, R31
        MOV     R1, ZERO
lx1:    MOV     R0, R1          ; recorte por la derecha
        ADD     R0, R7
        CMP     R0, R18
        JMP.LE  lx2
        MOV     R7, R18
        SUB     R7, R1
lx2:    CMP     R7, #0
        JMP.LE  lnext
        MOV     R2, R6
        MUL     R2, #8
        MOV     R8, ZERO
ltr:    MOV     YI3, #0
        MOV     XI3, R4
        CALL    xy0
        MOV     LOOP_CNT, R7
        LDI     LOOP_TGT, #lkp
lkp:    MOV     R0, DATA3 !f
        MOV.NE  DATA0, R0
      L MOV.EQ  ZERO, DATA0
        ADD     R4, #8
        ADD     R2, #1
        ADD     R8, #1
        CMP     R8, #8
        JMP.NE  ltr
lnext:  ADD     R5, #1
        CMP     R5, R13
        JMP.LS  lcol            ; columnas + 1 (el ultimo tile, recortado)
        ADD     R6, #1
        MOV     R0, R6
        MUL     R0, #8
        CMP     R0, R19
        JMP.CC  lrow
        ADD     R9, #1
        JMP     layer
done:   FLUSH   0
        FLUSH   3
        HALT    0
        .include "subs.inc"
