; objmove.inc - GPUDEMO: mueve el objeto (R5, R6) +-1 en cada eje segun R7
; (bit 0 x hacia la izquierda, bit 1 y hacia arriba) y rebota en
; [-R15, ancho - R15] y [-R15, alto - R15]. Usa R0.
        TST     R7, #1
        JMP.NE  mv_xl
        ADD     R5, #1
        MOV     R0, R18
        SUB     R0, R15
        CMP     R5, R0
        JMP.LT  mv_y
        XOR     R7, #1
        JMP     mv_y
mv_xl:  SUB     R5, #1
        MOV     R0, ZERO
        SUB     R0, R15
        CMP     R5, R0
        JMP.GT  mv_y
        XOR     R7, #1
mv_y:   TST     R7, #2
        JMP.NE  mv_yu
        ADD     R6, #1
        MOV     R0, R19
        SUB     R0, R15
        CMP     R6, R0
        JMP.LT  mv_end
        XOR     R7, #2
        JMP     mv_end
mv_yu:  SUB     R6, #1
        MOV     R0, ZERO
        SUB     R0, R15
        CMP     R6, R0
        JMP.GT  mv_end
        XOR     R7, #2
mv_end:
