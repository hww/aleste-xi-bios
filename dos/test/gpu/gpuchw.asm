; gpuchw.asm - GPUCHW.COM: escritura y lectura por los canales de la LX-GPU
; en cada formato (8 bpp, 4 bpp, 16 bpp, y un byte por palabra con 8 bpp y
; STEP 2), desde el Z80.
; Shaeon (Carlos Palmero), para el emulador alestelx y el core AlesteLX_MiSTer.
;
; Llena 800000h-8000FFh con EEh y pone 01 02 03 04 en 800050h; la GPU
; (chwrite.asm) escribe y lee por los canales, y el Z80 compara 800000h-80008Fh
; con la tabla de chwexp.inc y lo muestra. Los pixeles van como los lee el
; video del Aleste: los bytes por direccion y, dentro de cada byte, desde los
; bits altos (8 bpp: el pixel i es el byte i; 4 bpp: bits 7-4 y luego 3-0).
; pasmo gpuchw.asm GPUCHW.COM
        org 100h
        ld de, m_hello
        call pstr
        call map_in
        xor a
        out (0E9h), a
        ld hl, 4000h
        ld de, 4001h
        ld bc, 0FFh
        ld (hl), 0EEh
        ldir
        ld hl, 0201h
        ld (4050h), hl
        ld hl, 0403h
        ld (4052h), hl
        ld hl, pk_chw
        ld bc, pk_chw_end - pk_chw
        call load_pk
        call gpu_run
        ld (st), a
        xor a
        out (0E9h), a
        ld hl, 4000h
        ld de, buf
        ld bc, 90h
        ldir
        call map_out
        ; ---- informe: 9 lineas de 16 bytes, * donde no coincide
        ld de, m_status
        call pstr
        ld a, (st)
        call phex
        call crlf
        ld hl, buf
        ld (ptrb), hl
        ld hl, expect
        ld (ptre), hl
        xor a
        ld (off), a
        ld (bad), a
line:   ld a, (off)
        call phex
        ld a, 16
        ld (cnt), a
byte:   call pspace
        ld hl, (ptrb)
        ld a, (hl)
        call phex
        ld hl, (ptrb)
        ld de, (ptre)
        ld a, (de)
        cp (hl)
        jr z, same
        ld hl, bad
        inc (hl)
        ld e, '*'
        ld c, 2
        call BDOS
same:   ld hl, (ptrb)
        inc hl
        ld (ptrb), hl
        ld hl, (ptre)
        inc hl
        ld (ptre), hl
        ld hl, cnt
        dec (hl)
        jr nz, byte
        call crlf
        ld a, (off)
        add a, 16
        ld (off), a
        cp 90h
        jr nz, line
        ld a, (st)
        cp 80h
        ld a, 1
        jr nz, rslt
        ld a, (bad)
rslt:   jp result

        include "gpulib.inc"
        include "chwrite.inc"
        include "chwexp.inc"

m_hello:  db "GPUCHW: LX-GPU channels, 8/4/16 bpp, 8 bpp STEP 2", 13, 10, "$"
m_status: db "status $"
st:       db 0
bad:      db 0
off:      db 0
cnt:      db 0
ptrb:      dw 0
ptre:      dw 0
buf:      ds 90h
