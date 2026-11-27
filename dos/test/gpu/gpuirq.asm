; gpuirq.asm - GPUIRQ.COM: la interrupcion de la LX-GPU (linea 5 del PIC) y
; el registro 5 (IRQ_CODE, que al leerlo la reconoce).
; Shaeon (Carlos Palmero), para el emulador alestelx y el core AlesteLX_MiSTer.
;
; La GPU (irq.asm) hace IRQ 42h, espera unos 300 us, hace IRQ 43h y para. Con
; las interrupciones cortadas, el Z80 mira el pendiente de la linea 5 en el PIC
; (PIC v2, pagina MMIO 0: 3 PENDING_L, 0 COMMAND) y lee IRQ_CODE:
;  1. espera el pendiente de la 42h y lee IRQ_CODE (42h), lo que la reconoce;
;  2. borra el pendiente del PIC: tiene que quedar a 0 (la linea ya bajo);
;  3. espera la parada (STATUS 80h): el pendiente vuelve (la 43h es un flanco
;     nuevo, que no habria si la 42h no se hubiera reconocido) e IRQ_CODE = 43h;
;  4. lo borra y tiene que quedar a 0.
; pasmo gpuirq.asm GPUIRQ.COM
PIC      equ 0                 ; pagina MMIO del PIC
L5       equ 20h               ; bit de la linea 5
CLR5     equ 45h               ; COMMAND: CLEAR_PENDING (4), linea 5
        org 100h
        ld de, m_hello
        call pstr
        call map_in
        ld hl, pk_irq
        ld bc, pk_irq_end - pk_irq
        call load_pk
        xor a                  ; nada pendiente de antes en la linea 5
        out (PAGE), a
        ld a, CLR5             ; CLEAR_PENDING, linea 5
        out (0), a
        call gpu_start
        ld a, PIC              ; 1. el pendiente de la 42h
        out (PAGE), a
        ld bc, 0
w1:     in a, (3)
        and L5
        jr nz, g1
        dec bc
        ld a, b
        or c
        jr nz, w1
g1:     ld (p1), a
        ld a, GPU
        out (PAGE), a
        in a, (5)
        ld (c1), a
        ld a, PIC              ; 2. borrar: queda a 0
        out (PAGE), a
        ld a, CLR5             ; CLEAR_PENDING, linea 5
        out (0), a
        in a, (3)
        and L5
        ld (p2), a
        call gpu_wait          ; 3. la parada y la 43h
        ld (st), a
        ld a, PIC
        out (PAGE), a
        in a, (3)
        and L5
        ld (p3), a
        ld a, GPU
        out (PAGE), a
        in a, (5)
        ld (c2), a
        ld a, PIC              ; 4. borrar: queda a 0
        out (PAGE), a
        ld a, CLR5             ; CLEAR_PENDING, linea 5
        out (0), a
        in a, (3)
        and L5
        ld (p4), a
        call map_out
        ; ---- informe
        ld de, m_1
        call pstr
        ld a, (p1)
        call phex
        ld de, m_code
        call pstr
        ld a, (c1)
        call phex
        ld de, m_2
        call pstr
        ld a, (p2)
        call phex
        call crlf
        ld de, m_3
        call pstr
        ld a, (st)
        call phex
        ld de, m_pend
        call pstr
        ld a, (p3)
        call phex
        ld de, m_code
        call pstr
        ld a, (c2)
        call phex
        ld de, m_2
        call pstr
        ld a, (p4)
        call phex
        call crlf
        ld hl, expect          ; comparar
        ld de, p1
        ld b, 7
        ld c, 0
cmp:    ld a, (de)
        cp (hl)
        jr z, cmp1
        inc c
cmp1:   inc hl
        inc de
        djnz cmp
        ld a, c
        jp result

        include "gpulib.inc"
        include "irq.inc"

m_hello:  db "GPUIRQ: LX-GPU interrupt (PIC line 5) and IRQ_CODE", 13, 10, "$"
m_1:      db "IRQ 1: pending $"
m_code:   db "  IRQ_CODE $"
m_2:      db "  cleared $"
m_3:      db "status $"
m_pend:   db "  pending $"
; lo leido, en el orden de expect
p1:       db 0
c1:       db 0
p2:       db 0
st:       db 0
p3:       db 0
c2:       db 0
p4:       db 0
expect:   db L5, 42h, 0, 80h, L5, 43h, 0
