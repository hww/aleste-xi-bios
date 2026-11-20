; mag.asm - MAG.COM: reproductor de musica con pantalla, homenaje al MAG de
; la Magic Sound del Aleste 520EX (escrito por hww), con color y la LX-GPU.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
;   MAG fichero     toca un STM o un MOD (sin extension: .STM y luego .MOD)
;                   con la pantalla; cualquier tecla para la musica y sale.
;
; La musica la toca el LX-DOS (extension del BDOS: A0h carga y toca, A1h
; posicion y titulo, AAh estado de los canales). La pantalla es la de
; 320x200 con 256 colores del LX-DOS (A3h-A7h). El texto lo pone el BDOS; la
; LX-GPU dibuja el titulo al doble con un degradado (title.asm) y, 50 veces
; por segundo, mueve las estrellas y pinta los medidores (frame.asm). El
; ritmo lo marca el temporizador de 1 MHz (F4h/F5h) y el degradado se mueve
; rotando 16 colores de la paleta.
;
; Memoria de la GPU (pagina 94h, banco 50h del slot 2, en 4000h-7FFFh del
; supervisor mientras se usa, con las interrupciones cortadas): 940000h el
; paquete de frame.asm, 941000h el de title.asm, 942000h los datos de
; frame.asm (medidores, semilla y estrellas).
; pasmo mag.asm MAG.COM
BDOS    equ 5
TAIL    equ 80h
PAGE    equ 0F1h
GPU     equ 3
PKBANK  equ 50h
NSTARS  equ 160
DATA    equ 6000h               ; 942000h visto en 4000h-7FFFh
        org 100h
        ld sp, stack
        ; ---- el fichero: primer nombre de la linea de ordenes
        ld hl, TAIL
        ld b, (hl)
        inc hl
skip:   ld a, b
        or a
        jp z, usage
        ld a, (hl)
        cp ' '
        jr nz, gotarg
        inc hl
        dec b
        jr skip
gotarg: ld de, fname
        ld c, 0                 ; C = 1 si lleva extension
copy:   ld a, (hl)
        cp ' '
        jr z, copied
        cp '.'
        jr nz, copy1
        ld c, 1
copy1:  ld (de), a
        inc de
        inc hl
        djnz copy
copied: ld (fend), de
        ld a, c
        or a
        jr nz, load
        ld hl, ext_stm          ; sin extension: .STM y si no esta, .MOD
        call addext
        call play
        cp 1
        jr nz, loaded
        ld hl, ext_mod
        call addext
load:   call play
loaded: or a
        jp nz, lderr

        ; ---- la pantalla
        call cls                ; antes de ponerla, para que no salga basura
        ld e, 1
        ld c, 0A3h
        call BDOS
        ld de, pal
        ld c, 0A4h
        call BDOS
        call stars_init
        ld hl, texts
        call ctexts
        ld de, f_box            ; el recuadro de los medidores
        ld c, 0A5h
        call BDOS
        ld de, f_inner
        ld c, 0A5h
        call BDOS
        ld de, t_ch1
        call text
        ld de, t_ch2
        call text
        ld de, t_ch3
        call text
        ld de, t_ch4
        call text
        ld de, t_playing
        call text
        ld de, info
        ld c, 0A1h
        call BDOS
        ld hl, info + 9         ; el titulo de la cancion
        ld (t_title + 7), hl
        ld de, t_title
        call text
        ld de, t_big            ; "ALESTE LX" abajo, y la GPU lo copia al doble
        call text
        ld hl, 1000h
        call gpu_go
        call gpu_wait
        ld de, f_bottom
        ld c, 0A5h
        call BDOS
        ld de, t_pos
        call text
        ld c, 0F4h              ; empieza el ritmo
        in l, (c)
        inc c
        in h, (c)
        ld (last), hl

        ; ---- 50 cuadros por segundo hasta que se pulse una tecla
frame:  ld e, 0FFh
        ld c, 6
        call BDOS
        or a
        jp nz, quit
        call wait20
        call meters
        call gpu_wait           ; el cuadro anterior ya acabo
        call map_in
        ld a, PKBANK
        out (0E9h), a
        ld hl, lvl              ; anchos y picos a 942000h
        ld de, DATA
        ld bc, 16
        ldir
        ld hl, 0
        call gpu_start
        call map_out
        call cycle
        call status
        jr frame

quit:   ld de, 0                ; para la musica y vuelve al texto
        ld c, 0A0h
        call BDOS
        call gpu_wait
        ld e, 0
        ld c, 0A3h
        call BDOS
        jp 0

lderr:  push af
        ld de, m_err1
        call pstr
        ld de, fname
        call pstrz
        pop af
        ld de, m_nf
        cp 1
        jr z, lde1
        ld de, m_disk
        cp 2
        jr z, lde1
        ld de, m_fmt
        cp 3
        jr z, lde1
        ld de, m_big
lde1:   call pstr
        jp 0
usage:  ld de, m_usage
        call pstr
        jp 0

; ---- el fichero
addext: ld de, (fend)           ; HL = ".STM",0
        ld bc, 5
        ldir
        ret
play:   ld de, fname            ; A = codigo del BDOS A0h
        ld c, 0A0h
        jp BDOS

; ---- texto
text:   ld c, 0A7h              ; DE = bloque de A7h
        jp BDOS
ctexts: ld a, (hl)              ; HL = lista de (y, color, "texto",0), FFh
        cp 0FFh
        ret z
        ld (blk + 2), a         ; y
        inc hl
        ld a, (hl)
        ld (blk + 4), a         ; tinta
        inc hl
        ld (blk + 7), hl        ; el texto
        ld b, 0                 ; largo
ct1:    ld a, (hl)
        inc hl
        or a
        jr z, ct2
        inc b
        jr ct1
ct2:    push hl
        ld l, b                 ; x = 160 - 4 * largo
        ld h, 0
        add hl, hl
        add hl, hl
        ex de, hl
        ld hl, 160
        or a
        sbc hl, de
        ld (blk), hl
        ld de, blk
        call text
        pop hl
        jr ctexts

cls:    ld de, f_all
        ld c, 0A5h
        jp BDOS

; ---- ritmo: espera a que pasen 20 ms desde el cuadro anterior
wait20: ld c, 0F4h
        in l, (c)
        inc c
        in h, (c)
        ld de, (last)
        or a
        sbc hl, de              ; HL = us desde el ultimo cuadro
        ld de, 20000
        or a
        sbc hl, de
        jr c, wait20
        ld de, 20000            ; si va muy atrasado, no intenta recuperar
        or a
        sbc hl, de
        ld hl, (last)
        jr nc, w20late
        ld de, 20000
        add hl, de
        ld (last), hl
        jr w20cnt
w20late: ld c, 0F4h
        in l, (c)
        inc c
        in h, (c)
        ld (last), hl
w20cnt: ld a, (ticks)           ; 50 cuadros, un segundo
        inc a
        cp 50
        jr nz, w20s
        ld a, (secs)
        inc a
        cp 60
        jr nz, w20m
        ld a, (mins)
        inc a
        cp 100
        jr nz, w20x
        xor a
w20x:   ld (mins), a
        xor a
w20m:   ld (secs), a
        xor a
w20s:   ld (ticks), a
        ret

; ---- medidores: nota nueva = volumen; luego baja 4 pixeles por cuadro,
; nunca por encima del volumen de ahora. El pico sube con la barra, aguanta
; 30 cuadros y baja 2 pixeles por cuadro. Por canal (cst, 6 bytes): notas
; vistas, cuadros que aguanta el pico, barra y pico (palabras).
meters: ld de, voices
        ld c, 0AAh
        call BDOS
        ld ix, voices
        ld iy, cst
        ld hl, lvl
        ld (outp), hl
mt1:    ld e, (ix+0)            ; DE = volumen * 4 (0-256)
        ld d, 0
        sla e
        rl d
        sla e
        rl d
        ld a, (ix+1)
        cp (iy+0)
        jr z, mtdec
        ld (iy+0), a            ; nota nueva: la barra al volumen
        ex de, hl
        jr mtcur
mtdec:  ld l, (iy+2)
        ld h, (iy+3)
        ld a, h
        or l
        jr z, mtd1
        dec hl
        dec hl
        dec hl
        dec hl
mtd1:   push hl
        or a
        sbc hl, de
        pop hl
        jr c, mtcur
        ex de, hl               ; no por encima del volumen
mtcur:  ld (iy+2), l
        ld (iy+3), h
        ld e, (iy+4)            ; DE = pico
        ld d, (iy+5)
        push hl
        or a
        sbc hl, de
        pop hl
        jr c, mtkeep
        ex de, hl               ; la barra llega al pico
        ld (iy+1), 30
        jr mtpk
mtkeep: ld a, (iy+1)
        or a
        jr z, mtfall
        dec (iy+1)
        jr mtpk
mtfall: ld a, d
        or e
        jr z, mtpk
        dec de
        dec de
mtpk:   ld (iy+4), e
        ld (iy+5), d
        push hl                 ; HL = barra, DE = pico
        ld hl, (outp)
        pop bc                  ; (B se guarda abajo)
        ld (hl), c
        inc hl
        ld (hl), b
        inc hl
        ex de, hl               ; marca: 2 pixeles antes del pico, si >= 4
        ld bc, 4
        or a
        sbc hl, bc
        jr c, mtnone
        inc hl
        inc hl
        jr mtset
mtnone: ld hl, 0
mtset:  ex de, hl
        ld (hl), e
        inc hl
        ld (hl), d
        inc hl
        ld (outp), hl
        ld de, 4
        add ix, de
        ld de, 6
        add iy, de
        ld a, (mtcnt)
        dec a
        ld (mtcnt), a
        jp nz, mt1
        ld a, 4
        ld (mtcnt), a
        ret

; ---- el degradado del titulo: colores 16-31 = grad[(i + fase) & 31]
cycle:  ld a, (phase)
        inc a
        and 31
        ld (phase), a
        in a, (PAGE)
        push af
        ld a, 2                 ; paleta: pagina MMIO 2
        out (PAGE), a
        ld a, (phase)
        ld e, a
        ld d, 16                ; color
cy1:    ld a, d
        out (0), a
        ld a, e
        and 31
        add a, a
        ld c, a
        ld b, 0
        ld hl, grad
        add hl, bc
        ld a, (hl)
        out (1), a
        inc hl
        ld a, (hl)
        out (2), a
        inc e
        inc d
        ld a, d
        cp 32
        jr nz, cy1
        pop af
        out (PAGE), a
        ret

; ---- la linea de abajo: posicion, fila y tiempo. Solo se redibujan los
; numeros que cambian (el texto del BDOS es lento para hacerlo cada cuadro).
status: ld de, info
        ld c, 0A1h
        call BDOS
        ld a, (info + 1)
        inc a
        ld hl, o_pos
        ld de, 48
        call num
        ld a, (info + 2)
        ld hl, o_len
        ld de, 72
        call num
        ld a, (info + 4)
        ld hl, o_row
        ld de, 136
        call num
        ld a, (mins)
        ld hl, o_min
        ld de, 272
        call num
        ld a, (secs)
        ld hl, o_sec
        ld de, 296
        ; sigue en num
num:    cp (hl)                 ; A en x = DE si no es el de (HL)
        ret z
        ld (hl), a
        ld (t_num), de
        ld hl, s_num
        call dec2
        ld de, t_num
        jp text
dec2:   ld b, '0' - 1           ; A (0-99) en (HL), dos cifras
d2a:    inc b
        sub 10
        jr nc, d2a
        add a, '0' + 10
        ld (hl), b
        inc hl
        ld (hl), a
        ret

; ---- GPU
map_in: di                      ; pagina 1 del supervisor -> slot 2
        in a, (0E9h)
        ld (sv_e9), a
        in a, (0FAh)
        ld (sv_fa), a
        in a, (PAGE)
        ld (sv_page), a
        ld a, 0FBh
        out (0FAh), a
        ret
map_out: ld a, (sv_page)
        out (PAGE), a
        ld a, (sv_e9)
        out (0E9h), a
        ld a, (sv_fa)
        out (0FAh), a
        ei
        ret
gpu_start:                      ; HL = desplazamiento del paquete en 940000h
        ld a, GPU
        out (PAGE), a
        ld a, l
        out (0), a
        ld a, h
        out (1), a
        ld a, 94h
        out (2), a
        ld a, 1
        out (3), a
        ret
gpu_go: call map_in             ; gpu_start con la MMU como estaba
        call gpu_start
        jr map_out
gpu_wait:                       ; espera a que pare (o unos 0,3 s)
        in a, (PAGE)
        push af
        ld a, GPU
        out (PAGE), a
        ld bc, 0
gw1:    in a, (4)
        bit 7, a
        jr nz, gw2
        dec bc
        ld a, b
        or c
        jr nz, gw1
gw2:    pop af
        out (PAGE), a
        ret

; ---- los paquetes, la semilla y las estrellas en la memoria de la GPU
stars_init:
        call map_in
        ld a, PKBANK
        out (0E9h), a
        ld hl, pk_frame
        ld de, 4000h
        ld bc, pk_frame_end - pk_frame
        ldir
        ld hl, pk_title
        ld de, 5000h
        ld bc, pk_title_end - pk_title
        ldir
        ld hl, DATA             ; medidores a cero
        ld b, 16
si0:    ld (hl), 0
        inc hl
        djnz si0
        ld hl, 1234h            ; semilla de la GPU
        ld (DATA + 10h), hl
        ld ix, DATA + 20h       ; estrellas: x * 16, y, velocidad, color
        ld b, 0                 ; numero de estrella
si1:    push bc
        call rand               ; x * 16 = H * 20 + (L & 15)
        ld a, l
        and 15
        ld e, h
        ld d, 0
        ld l, e
        ld h, d
        add hl, hl
        add hl, hl
        add hl, de              ; * 5
        add hl, hl
        add hl, hl              ; * 20
        ld e, a
        add hl, de
        ld (ix+0), l
        ld (ix+1), h
        call rand               ; y = H * 25 / 32
        ld e, h
        ld d, 0
        ld l, e
        ld h, d
        add hl, hl
        add hl, hl
        add hl, de              ; * 5
        ld e, l
        ld d, h
        add hl, hl
        add hl, hl
        add hl, de              ; * 25
        ld b, 5
si2:    srl h
        rr l
        djnz si2
        ld (ix+2), l
        ld (ix+3), h
        pop bc
        ld a, b                 ; capa: velocidad y color
        and 3
        ld e, a
        ld d, 0
        ld hl, speeds
        add hl, de
        ld a, (hl)
        ld (ix+4), a
        ld (ix+5), 0
        ld a, e
        add a, 32
        ld (ix+6), a
        ld (ix+7), 0
        ld de, 8
        add ix, de
        inc b
        ld a, b
        cp NSTARS
        jr nz, si1
        jp map_out

rand:   ld hl, (seed)           ; xorshift de 16 bits
        ld a, h
        rra
        ld a, l
        rra
        xor h
        ld h, a
        ld a, l
        rra
        ld a, h
        rra
        xor l
        ld l, a
        xor h
        ld h, a
        ld (seed), hl
        ret

; ---- salida en modo texto
pstr:   ld c, 9
        jp BDOS
pstrz:  ld a, (de)              ; DE = texto acabado en 0
        or a
        ret z
        push de
        ld e, a
        ld c, 2
        call BDOS
        pop de
        inc de
        jr pstrz

        include "pal.inc"
        include "frame.inc"
        include "title.inc"

speeds: db 6, 12, 20, 32        ; 1/16 de pixel por cuadro
ext_stm: db ".STM", 0
ext_mod: db ".MOD", 0

; bloques de A5h: x, y, ancho, alto, color
f_all:    dw 0, 0, 320, 200
          db 0
f_box:    dw 16, 126, 288, 50
          db 7
f_inner:  dw 18, 128, 284, 46
          db 2
f_bottom: dw 0, 192, 320, 8
          db 0
; bloques de A7h: x, y, tinta, fondo, transparente, texto
blk:      dw 0, 0
          db 0, 0, 0
          dw 0
t_big:    dw 88, 192
          db 1, 0, 0
          dw s_big
t_ch1:    dw 24, 131
          db 8, 2, 0
          dw s_ch1
t_ch2:    dw 24, 142
          db 8, 2, 0
          dw s_ch2
t_ch3:    dw 24, 153
          db 8, 2, 0
          dw s_ch3
t_ch4:    dw 24, 164
          db 8, 2, 0
          dw s_ch4
t_playing: dw 16, 181
          db 1, 0, 0
          dw s_playing
t_title:  dw 88, 181
          db 6, 0, 0
          dw 0
t_pos:    dw 16, 191
          db 8, 0, 0
          dw s_pos
t_num:    dw 0, 191
          db 1, 0, 0
          dw s_num
s_big:    db "ALESTE LX", 0
s_ch1:    db "1", 0
s_ch2:    db "2", 0
s_ch3:    db "3", 0
s_ch4:    db "4", 0
s_playing: db "Playing:", 0
s_pos:    db "Pos   /    Row                    :", 0
s_num:    db "00", 0

; textos centrados: y, tinta, texto
texts:  db 24, 1, "MAGIC  SOUND  TRACKER", 0
        db 36, 5, "You are listening to sound made by the", 0
        db 44, 5, "MAGIC SOUND 2 card, designed only for", 0
        db 52, 3, "the ALESTE LX computer", 0
        db 66, 1, "STEREO  OUTPUT", 0
        db 76, 9, "4 sound channels, 48 kHz, by DMA", 0
        db 84, 9, "8/16-bit samples, 8-bit volume", 0
        db 96, 3, "PROTRACKER & SCREAM TRACKER COMPATIBLE", 0
        db 104, 4, "graphics drawn by the LX-GPU", 0
        db 116, 6, "(C) 2026 Shaeon. Original: hww, H2W Lab", 0
        db 0FFh

m_usage: db "MAG: plays a Scream Tracker (STM) or ProTracker (MOD) song", 13, 10
         db "with the Magic Sound 2 and the LX-GPU. Any key stops it.", 13, 10
         db "Usage: MAG file   (no extension: .STM, then .MOD)", 13, 10, "$"
m_err1:  db "MAG: $"
m_nf:    db ": not found", 13, 10, "$"
m_disk:  db ": disk error", 13, 10, "$"
m_fmt:   db ": not a 4-channel MOD or an STM", 13, 10, "$"
m_big:   db ": too big", 13, 10, "$"

mtcnt:   db 4
outp:    dw 0
seed:    dw 0ACE1h
last:    dw 0
ticks:   db 0
secs:    db 0
mins:    db 0
phase:   db 0
o_pos:   db 0FFh
o_len:   db 0FFh
o_row:   db 0FFh
o_min:   db 0FFh
o_sec:   db 0FFh
fend:    dw 0
sv_e9:   db 0
sv_fa:   db 0
sv_page: db 0
cst:     ds 24
lvl:     ds 16              ; ancho y marca de cada canal (palabras): a 942000h
voices:  ds 16
info:    ds 32
fname:   ds 20
         ds 256
stack:
