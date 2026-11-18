; gpuloop.asm - GPULOOP.COM: los bucles por hardware de la LX-GPU desde el
; Z80 (test_loop y test_outer_loop de lx-gpu-rtl, que ademas guardan R0).
; Shaeon (Carlos Palmero), para el emulador alestelx y el core AlesteLX_MiSTer.
;
; loop.asm: un bucle de 10 vueltas -> R0 = 000Ah en la palabra 0 de 800000h.
; outer.asm: 3 vueltas de un bucle cuyo cuerpo es un CALL a una subrutina con
; otro bucle de 4 (guardando LOOP_CNT, LOOP_TGT y SHADOW) -> R0 = 000Ch en la
; palabra 1. Las dos tienen que acabar con STATUS = 80h (parada sin error).
; pasmo gpuloop.asm GPULOOP.COM
        org 100h
        ld de, m_hello
        call pstr
        call map_in
        xor a                  ; las dos palabras a FFFFh
        out (0E9h), a
        ld hl, 0FFFFh
        ld (4000h), hl
        ld (4002h), hl
        ld hl, pk_loop
        ld bc, pk_loop_end - pk_loop
        call load_pk
        call gpu_run
        ld (st1), a
        ld hl, pk_outer
        ld bc, pk_outer_end - pk_outer
        call load_pk
        call gpu_run
        ld (st2), a
        xor a
        out (0E9h), a
        ld hl, (4000h)
        ld (r1), hl
        ld hl, (4002h)
        ld (r2), hl
        call map_out
        ; ---- informe
        ld de, m_loop
        call pstr
        ld a, (st1)
        call phex
        ld de, m_r0
        call pstr
        ld hl, (r1)
        call pword
        call crlf
        ld de, m_outer
        call pstr
        ld a, (st2)
        call phex
        ld de, m_r0
        call pstr
        ld hl, (r2)
        call pword
        call crlf
        ld b, 0                ; fallos
        ld a, (st1)
        cp 80h
        jr z, c1
        inc b
c1:     ld a, (st2)
        cp 80h
        jr z, c2
        inc b
c2:     ld hl, (r1)
        ld de, 000Ah
        or a
        sbc hl, de
        jr z, c3
        inc b
c3:     ld hl, (r2)
        ld de, 000Ch
        or a
        sbc hl, de
        jr z, c4
        inc b
c4:     ld a, b
        jp result

        include "gpulib.inc"
        include "loop.inc"
        include "outer.inc"

m_hello:  db "GPULOOP: LX-GPU hardware loops (10, and 3 x CALL x 4)", 13, 10, "$"
m_loop:   db "loop:  status $"
m_outer:  db "outer: status $"
m_r0:     db "  R0 $"
st1:      db 0
st2:      db 0
r1:       dw 0
r2:       dw 0
