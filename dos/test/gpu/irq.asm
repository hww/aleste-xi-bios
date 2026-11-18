; irq.asm - la interrupcion de la GPU (linea 5 del PIC) y el registro 5
; (IRQ_CODE, que al leerlo la reconoce): IRQ 42h, una espera de 16384
; vueltas de un bucle por hardware (unos 300 us a 54 MHz), IRQ 43h y HALT.
.org 0x20
        IRQ     0x42             ; PC=0x020
        LDI     R0, #0           ; PC=0x021
        LDI     R1, #0x4000      ; PC=0x022
        LDI     R2, #0x0026      ; PC=0x023: wait
        MOV     LOOP_TGT, R2     ; PC=0x024
        MOV     LOOP_CNT, R1     ; PC=0x025
wait:
        L ADD   R0, #1           ; PC=0x026
        IRQ     0x43
        HALT
