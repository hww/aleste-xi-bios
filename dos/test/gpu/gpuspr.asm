; gpuspr.asm - GPUSPR.COM: el sprite de test_sprite de lx-gpu-rtl desde el
; Z80: dos canales a la vez (CH0 lee, CH1 escribe) por el arbitro de memoria.
; Shaeon (Carlos Palmero), para el emulador alestelx y el core AlesteLX_MiSTer.
;
; Pone el sprite de 16x16 (sprite_data.inc, de mkspr.py: 0 = transparente) en
; 800400h y llena el banco 2 de la pagina 80h (808000h-80BFFFh, donde cae el
; sprite en X=100, Y=100 de una pantalla de 320x200 que empieza en el pixel
; 800h) con el byte bajo de su direccion. La GPU (sprite.asm) copia el sprite y
; el Z80 repasa los 16 KB del banco: cada byte tiene que seguir igual salvo los
; de la lista de mkspr.py, que tienen que tener el valor del sprite.
; pasmo gpuspr.asm GPUSPR.COM
        org 100h
        ld de, m_hello
        call pstr
        call map_in
        xor a                  ; el sprite en 800400h
        out (0E9h), a
        ld hl, spr_src
        ld de, 4400h
        ld bc, 256
        ldir
        ld a, 2                ; el banco 2: el byte bajo de la direccion
        out (0E9h), a
        ld hl, 4000h
        xor a
fill:   ld (hl), a
        inc hl
        inc a
        jr nz, fill
        ld hl, 4000h
        ld de, 4100h
        ld bc, 3F00h
        ldir
        ld hl, pk_spr
        ld bc, pk_spr_end - pk_spr
        call load_pk
        call gpu_run
        ld (st), a
        ld a, 2                ; ---- repasar el banco 2
        out (0E9h), a
        ld iy, spr_patch
        call nextp
        ld hl, 4000h
scan:   ld a, h
        cp 80h
        jr z, sdone
        ld a, h                ; aqui cambia la GPU?
        cp d
        jr nz, nopat
        ld a, l
        cp e
        jr nz, nopat
        ld b, c
        call nextp
        ld a, b
        jr cmpb
nopat:  ld a, l
cmpb:   cp (hl)
        jr z, sok
        ld b, a                ; error
        ld a, (errs)
        or a
        jr nz, cnt
        ld (e_adr), hl
        ld a, b
        ld (e_exp), a
        ld a, (hl)
        ld (e_act), a
cnt:    ld a, (errs)
        inc a
        jr z, sok              ; se queda en FFh
        ld (errs), a
sok:    inc hl
        jr scan
sdone:  call map_out
        ; ---- informe
        ld de, m_status
        call pstr
        ld a, (st)
        call phex
        ld de, m_errs
        call pstr
        ld a, (errs)
        call phex
        call crlf
        ld a, (errs)
        or a
        jr z, rep
        ld de, m_first
        call pstr
        ld a, 80h              ; 4000h-7FFFh -> 808000h-80BFFFh
        call phex
        ld hl, (e_adr)
        ld de, 4000h
        add hl, de
        call pword
        ld de, m_is
        call pstr
        ld a, (e_act)
        call phex
        ld de, m_exp
        call pstr
        ld a, (e_exp)
        call phex
        call crlf
rep:    ld a, (st)
        cp 80h
        ld a, 1
        jp nz, result
        ld a, (errs)
        jp result

nextp:                         ; DE = 4000h + desplazamiento del siguiente
        push hl                ; cambio, C = su valor (DE = 0 al acabar)
        push iy
        pop hl
        ld de, spr_patch_end
        or a
        sbc hl, de
        pop hl
        jr z, np_end
        ld e, (iy+0)
        ld a, (iy+1)
        add a, 40h
        ld d, a
        ld c, (iy+2)
        inc iy
        inc iy
        inc iy
        ret
np_end: ld de, 0
        ret

        include "gpulib.inc"
        include "sprite.inc"
        include "sprite_data.inc"

m_hello:  db "GPUSPR: LX-GPU 16x16 sprite, CH0 -> CH1, 320x200 8 bpp", 13, 10, "$"
m_status: db "status $"
m_errs:   db "  wrong bytes $"
m_first:  db "first at $"
m_is:     db ": $"
m_exp:    db ", expected $"
st:       db 0
errs:     db 0
e_adr:    dw 0
e_act:    db 0
e_exp:    db 0
