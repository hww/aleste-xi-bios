; s10_shift.asm - GPUDEMO, escena 10: desplazamiento de una imagen por lineas.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; Una prueba visual sin rampa: una imagen (.LXI del disco) desplazada linea
; a linea. Con P7 = 0, las franjas de 8 lineas van alternando hacia la
; izquierda y hacia la derecha (4 pixeles por cuadro); con P7 = 1, la
; imagen sube (2 lineas por cuadro). Se copia desde la imagen, que esta en
; otra pagina, al bufer de atras, asi que nunca se pisa lo que falta por
; leer; dando la vuelta por los bordes, y repitiendo la imagen si la
; pantalla es mas ancha (640x200) o cortandola si es mas baja (320x96).
; En palabras de 16 bits (el desplazamiento es multiplo de 4 pixeles).
;
; P13 pagina de la imagen (en el formato del destino), P14 palabras por
; linea de la imagen, P15 lineas de la imagen.
.org 0x20
        .include "common.inc"
        MOV     R15, R16        ; pixeles -> palabras: >> 1 o >> 2
        SHR     R15, #8
        MOV     R0, R16
        AND     R0, #0xFF
        MOV     PAGE0, R0
        MOV     STEP0, #1
        MOV     PAGE1, R29
        MOV     STEP1, #1
        MOV     R1, R17         ; palabras por linea del destino
        SHR     R1, R15
        MOV     R2, R18         ; palabras visibles
        SHR     R2, R15
        LDI     R0, #4          ; desplazamiento: cuadro * 4 pixeles, en palabras
        SHR     R0, R15
        MUL     R0, R21
        MOV     R11, R0
sm:     CMP     R11, R30        ; mod palabras de la imagen
        JMP.CC  smd
        SUB     R11, R30
        JMP     sm
smd:    MOV     R12, R21        ; subida: cuadro * 2 lineas, mod lineas de la imagen
        ADD     R12, R21
sv:     CMP     R12, R31
        JMP.CC  svd
        SUB     R12, R31
        JMP     sv
svd:    MOV     R3, ZERO        ; y
        MOV     R4, ZERO        ; primera palabra de la linea del destino
row:    MOV     R5, R3          ; linea de la imagen
        MOV     R6, ZERO        ; palabra de inicio en la linea de la imagen
        TST     R23, #1
        JMP.EQ  horiz
        ADD     R5, R12         ; vertical: sube
        JMP     vmod
horiz:  MOV     R6, R11         ; horizontal: franjas alternas
        TST     R3, #8
        JMP.EQ  vmod
        MOV     R6, R30
        SUB     R6, R11
        CMP     R6, R30
        JMP.CC  vmod
        MOV     R6, ZERO
vmod:   CMP     R5, R31
        JMP.CC  vok
        SUB     R5, R31
        JMP     vmod
vok:    MUL     R5, R30         ; primera palabra de la linea de la imagen
        MOV     IDXH0, #0
        MOV     IDX0, R4
        MOV     R7, R2          ; palabras que faltan en el destino
seg:    MOV     R8, R30         ; tramo: hasta el final de la linea de la imagen
        SUB     R8, R6
        CMP     R8, R7
        JMP.LS  s1
        MOV     R8, R7
s1:     MOV     R0, R5
        ADD     R0, R6
        MOV     IDXH1, #0
        MOV     IDX1, R0
        MOV     LOOP_CNT, R8
        LDI     LOOP_TGT, #cp
cp:     MOV     R0, DATA1
      L MOV     DATA0, R0
        MOV     R6, ZERO        ; el siguiente tramo empieza en el borde
        SUB     R7, R8
        JMP.NE  seg
        ADD     R4, R1
        ADD     R3, #1
        CMP     R3, R19
        JMP.NE  row
        FLUSH   0
        FLUSH   1
        HALT    0
