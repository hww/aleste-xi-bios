; gpulib.inc - rutinas comunes de los tests de la LX-GPU (GPULOOP, GPUCHW,
; GPUSPR, GPUIRQ). Shaeon (Carlos Palmero).
;
; Mientras trabajan, los tests ponen la pagina 1 del supervisor (4000h-7FFFh)
; en el slot 2 con las interrupciones cortadas, y llaman al BDOS solo despues
; de dejarlo todo como estaba (map_out). La pagina 80h de la GPU (800000h) son
; los bancos 0-3 del slot 2; el paquete va en 940000h (banco 50h).
; Registros de la GPU (pagina MMIO 3): 0-2 GPU_ADDR, 3 COMMAND (1 START),
; 4 STATUS (bit 7 parada, bits 6-0 error), 5 IRQ_CODE (leerlo la reconoce).
BDOS     equ 5
PAGE     equ 0F1h              ; pagina MMIO
GPU      equ 3
PKBANK   equ 50h

map_in:                        ; DI, guarda E9h/FAh/F1h, pagina 1 -> slot 2
        di
        in a, (0E9h)
        ld (sv_e9), a
        in a, (0FAh)
        ld (sv_fa), a
        in a, (PAGE)
        ld (sv_page), a
        ld a, 0FBh
        out (0FAh), a
        ret

map_out:                       ; deja la MMU como estaba y EI
        ld a, (sv_page)
        out (PAGE), a
        ld a, (sv_e9)
        out (0E9h), a
        ld a, (sv_fa)
        out (0FAh), a
        ei
        ret

load_pk:                       ; HL = paquete, BC = bytes -> 940000h
        ld a, PKBANK
        out (0E9h), a
        ld de, 4000h
        ldir
        ret

gpu_start:                     ; GPU_ADDR = 940000h y START (deja la pagina 3)
        ld a, GPU
        out (PAGE), a
        xor a
        out (0), a
        out (1), a
        ld a, 94h
        out (2), a
        ld a, 1
        out (3), a
        ret

gpu_wait:                      ; espera la parada: A = STATUS (FFh si no para)
        ld a, GPU
        out (PAGE), a
        ld bc, 0
gw1:    in a, (4)
        bit 7, a
        ret nz
        dec bc
        ld a, b
        or c
        jr nz, gw1
        ld a, 0FFh
        ret

gpu_run:                       ; gpu_start + gpu_wait
        call gpu_start
        jr gpu_wait

; ---- salida (solo con la MMU restaurada)
pstr:   ld c, 9                ; DE = texto acabado en $
        jp BDOS

crlf:   ld de, m_crlf
        jr pstr

pspace: ld e, ' '
        ld c, 2
        jp BDOS

phex:   push af                ; A en hexadecimal
        rrca
        rrca
        rrca
        rrca
        call phex1
        pop af
phex1:  and 0Fh
        add a, 90h
        daa
        adc a, 40h
        daa
        ld e, a
        ld c, 2
        jp BDOS

pword:  push hl                ; HL en hexadecimal
        ld a, h
        call phex
        pop hl
        ld a, l
        jr phex

result:                        ; A = 0: OK; si no, FAIL
        or a
        ld de, m_ok
        jr z, pstr
        ld de, m_fail
        jr pstr

m_crlf:   db 13, 10, "$"
m_ok:     db "OK", 13, 10, "$"
m_fail:   db "FAIL", 13, 10, "$"
sv_e9:    db 0
sv_fa:    db 0
sv_page:  db 0
