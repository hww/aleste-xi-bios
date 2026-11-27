; subs.inc - GPUDEMO: subrutinas (CALL de un nivel: no se anidan).
;
; rnd: R12 = R12 * 75 + 74 (congruencial); R14 = numero al azar en
;      [0, R13), par. Usa R14.
rnd:    MUL     R12, #75
        ADD     R12, #74
        MOV     R14, R12
        SHR     R14, #1
        MUL     R14, R13
        MOV     R14, MHI
        ADD     R14, R14
        RET
; xy0: CH0 al pixel (R1, R2) del destino: R2 * ancho de linea + R1, con
;      YI = parte alta (el bufer de 640x200 en 4 bpp pasa de 65535 pixeles; en
;      modo lineal el indice es {YI[2:0], XI}). Usa R10, R11.
xy0:    MOV     R10, R2
        MUL     R10, R17
        MOV     R11, MHI
        ADD     R10, R1
        ADC     R11, ZERO
        MOV     YI0, R11
        MOV     XI0, R10
        RET
