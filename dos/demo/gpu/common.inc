; common.inc - GPUDEMO: principio de todos los microprogramas.
; Las dos LDI las parchea GPUDEMO con el indice de palabra y la pagina del
; bloque de parametros P0-P15, que se copian a R16-R31 (ver demo.h).
        LDI     R30, #0
        LDI     R31, #0
        MOV     PAGE3, R31
        MOV     IDXH3, #0
        MOV     STEP3, #1
        MOV     IDX3, R30
        MOV     R16, DATA3      ; P0 destino {modo, pagina}
        MOV     R17, DATA3      ; P1 ancho de linea
        MOV     R18, DATA3      ; P2 ancho visible
        MOV     R19, DATA3      ; P3 alto
        MOV     R20, DATA3      ; P4 N
        MOV     R21, DATA3      ; P5 cuadro
        MOV     R22, DATA3      ; P6 banderas
        MOV     R23, DATA3      ; P7 tamano / fase
        MOV     R24, DATA3      ; P8 pagina de los objetos
        MOV     R25, DATA3      ; P9 pagina de los graficos
        MOV     R26, DATA3      ; P10 pagina de los datos del Z80
        MOV     R27, DATA3      ; P11 pagina de la fuente
        MOV     R28, DATA3      ; P12
        MOV     R29, DATA3      ; P13
        MOV     R30, DATA3      ; P14
        MOV     R31, DATA3      ; P15
        FLUSH   3               ; la cache de un canal no mira la pagina: al cambiarla, FLUSH
