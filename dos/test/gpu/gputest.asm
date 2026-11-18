; gputest.asm - GPUTEST.COM: la LX-GPU desde el Z80 (pagina MMIO 3)
; Shaeon (Carlos Palmero), para el emulador alestelx y el core AlesteLX_MiSTer.
;
; Copia un paquete de microcodigo [SIZE, START, palabras...] (fill.asm, de
; tools/asm de lx-gpu-rtl) a la memoria fisica 940000h (slot 2, banco 50h),
; pone a 0 los 256 bytes de 800000h (slot 2, banco 0) y una marca 33h en
; 800100h, escribe GPU_ADDR = 940000h y COMMAND = START, y espera a que la
; GPU se detenga (STATUS bit 7). Comprueba que los 256 bytes valen 5Ah, que
; la marca sigue en 33h y que la GPU no paro con error.
; Registros de la GPU (pagina MMIO 3): 0-2 GPU_ADDR, 3 COMMAND (1 START,
; 2 RESET), 4 STATUS (bit 7 parada, bits 6-0 codigo de error).
; pasmo gputest.asm GPUTEST.COM
BDOS     equ 5
PAGE     equ 0F1h              ; pagina MMIO
GPU      equ 3
PKBANK   equ 50h               ; el paquete: slot 2, banco 50h = 940000h
DSTBANK  equ 00h               ; el destino: slot 2, banco 0 = 800000h

        org 100h
        ld de, m_hello
        ld c, 9
        call BDOS
        di
        in a, (0E9h)
        ld (sv_e9), a
        in a, (0FAh)
        ld (sv_fa), a
        in a, (PAGE)
        ld (sv_page), a
        ld a, 0FBh             ; pagina 1 del supervisor -> slot 2
        out (0FAh), a
        ld a, PKBANK           ; el paquete en 940000h
        out (0E9h), a
        ld hl, packet
        ld de, 4000h
        ld bc, packet_end - packet
        ldir
        ld a, DSTBANK          ; el destino a 0, la marca en 800100h
        out (0E9h), a
        ld hl, 4000h
        ld de, 4001h
        ld bc, 0FFh
        ld (hl), 0
        ldir
        ld a, 33h
        ld (4100h), a
        ; ---- la GPU
        ld a, GPU
        out (PAGE), a
        xor a
        out (0), a             ; GPU_ADDR = 940000h
        out (1), a
        ld a, 94h
        out (2), a
        ld a, 1                ; START
        out (3), a
        ld bc, 0               ; espera a que se detenga (o se acaba)
wait:   in a, (4)
        bit 7, a
        jr nz, done
        dec bc
        ld a, b
        or c
        jr nz, wait
        ld a, 0FFh             ; no se detuvo
done:   ld (status), a
        ld a, (sv_page)
        out (PAGE), a
        ; ---- comprobar el destino
        ld hl, 4000h
        ld b, 0                ; 256 bytes
chk:    ld a, (hl)
        cp 5Ah
        jr nz, bad
        inc hl
        djnz chk
        ld a, (4100h)
        cp 33h
        jr nz, bad
        ld a, 1
        ld (ok), a
bad:    ld a, (sv_e9)
        out (0E9h), a
        ld a, (sv_fa)
        out (0FAh), a
        ei
        ld de, m_status
        ld c, 9
        call BDOS
        ld a, (status)
        call phex
        ld a, (status)
        cp 80h                 ; parada sin error
        jr nz, fail
        ld a, (ok)
        or a
        jr z, fail
        ld de, m_ok
        jr pr
fail:   ld de, m_fail
pr:     ld c, 9
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

packet:
        include "packet.inc"
packet_end:

m_hello:  db "GPUTEST: LX-GPU fill 256 bytes at 800000h", 13, 10, "$"
m_status: db "GPU status: $"
m_ok:     db 13, 10, "OK", 13, 10, "$"
m_fail:   db 13, 10, "FAIL", 13, 10, "$"
sv_e9:    db 0
sv_fa:    db 0
sv_page:  db 0
status:   db 0
ok:       db 0
