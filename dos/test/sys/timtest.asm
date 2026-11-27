; timtest.asm - TIMTEST.COM: el temporizador del sistema rev 6 (F4h-F6h)
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; RT: contador libre de 16 bits a 1 MHz. F4h (lectura) copia RT en la
; instantanea del modo (una de supervisor, otra de usuario) y da el byte bajo;
; F5h da el byte alto de esa instantanea. F4h/F5h escritos (supervisor) fijan
; COMPARE: F4h abre WINDOW (no entra INT), F5h arma (ARMED, ENABLE, FIRED a
; 0). Cuando RT llega, salta la linea 6 del PIC una vez. F6h: bit 4 FIRED,
; 3 DUE, 2 WINDOW, 1 ARMED, 0 ENABLE.
;  1. Ritmo: RT en tres cuadros (VBlank, linea 2 del PIC) entre 48000 y 61500
;     us (50 o 60 Hz).
;  2. Instantanea: F4h, una espera de mas de 1 ms, F5h dos veces: las dos
;     iguales (F5h no copia), y una lectura nueva despues ha avanzado > 900.
;  3. Modo usuario con el bloqueo (F0h bit 4): el supervisor lee el par, el
;     usuario espera > 1 ms y lee el suyo; de vuelta, F5h del supervisor sigue
;     dando su byte alto (las instantaneas son de cada modo) y el valor del
;     usuario esta entre los dos del supervisor.
;  4. Comparador a +2000 us: WINDOW entre F4h y F5h, F6h = ARMED+ENABLE tras
;     armar, la linea 6 salta a los 1900-2400 us con FIRED y DUE.
;  5. Una sola vez: borrado el pendiente, en 70 ms no vuelve a saltar (sin
;     FIRED saltaria otra vez a los 65,5 ms) y FIRED sigue a 1.
;  6. Un objetivo ya pasado salta al armar. Se deja asi: al abrir las
;     interrupciones, la rutina de la ROM atiende la linea 6 y rearma su
;     comparador. (El reloj del LX-DOS pierde los ~65 ms del paso 5.)
; Todo con las interrupciones cerradas.
; pasmo timtest.asm TIMTEST.COM
BDOS    equ 5
TLO     equ 0F4h
TCTL    equ 0F6h
PAGE    equ 0F1h
PIC_CMD equ 00h                ; pagina MMIO 0
PIC_PND equ 03h                ; PENDING_L
CLR_VB  equ 42h                ; CLEAR_PENDING, linea 2
CLR_TM  equ 46h                ; CLEAR_PENDING, linea 6
T       equ 7F10h
        org 100h
        ld de, m_hello
        call pstr
        di
        in a, (PAGE)
        ld (sv_f1), a
        xor a                  ; el PIC
        out (PAGE), a
        ld b, 0                ; B = fallos

        ; ---- 1. ritmo: tres cuadros
        call vb_edge
        call rdrt
        ld (t0), hl
        call vb_edge
        call vb_edge
        call vb_edge
        call rdrt
        ld de, (t0)
        or a
        sbc hl, de
        ld (r1), hl
        ld de, 48000
        call range_lo
        ld de, 61501
        call range_hi

        ; ---- 2. instantanea
        ld c, TLO
        in l, (c)              ; copia RT
        call wait1ms
        inc c
        in h, (c)
        in a, (c)
        ld (r2a), hl           ; el par
        cp h
        call nz, fail
        call wait1ms
        call rdrt
        ld de, (r2a)
        or a
        sbc hl, de
        ld (r2b), hl
        ld de, 901
        call range_lo
        ld de, 32768
        call range_hi

        ; ---- 3. modo usuario
        call user_test
        ld hl, (u_h)           ; F5h de vuelta = el alto del par de antes
        ld a, (s0 + 1)
        cp l
        call nz, fail
        ld hl, (uv)            ; uv - s0 >= 1000
        ld de, (s0)
        or a
        sbc hl, de
        ld de, 1000
        call range_lo
        ld de, 32768
        call range_hi
        ld hl, (s1)            ; s1 - uv > 0
        ld de, (uv)
        or a
        sbc hl, de
        ld de, 1
        call range_lo
        ld de, 32768
        call range_hi

        ; ---- 4. comparador a +2000 us
        ld a, CLR_TM
        out (PIC_CMD), a
        call rdrt
        ld de, 2000
        add hl, de
        ld a, l
        out (TLO), a           ; COMPARE bajo: WINDOW
        in a, (TCTL)
        ld (f_win), a
        and 04h
        call z, fail
        ld a, h
        out (TLO + 1), a       ; arma
        call rdrt
        ld (t0), hl
        in a, (TCTL)
        ld (f_arm), a
        and 1Fh
        cp 03h
        call nz, fail
        in a, (PIC_PND)
        and 40h
        call nz, fail          ; aun no
w4:     call rdrt              ; espera la linea 6 (10 ms como mucho)
        ld de, (t0)
        or a
        sbc hl, de
        ld (r4), hl
        ld de, 10000
        or a
        sbc hl, de
        jr nc, w4x
        in a, (PIC_PND)
        and 40h
        jr z, w4
w4x:    ld hl, (r4)
        ld de, 1900
        call range_lo
        ld de, 2401
        call range_hi
        in a, (TCTL)
        ld (f_fire), a
        and 18h
        cp 18h
        call nz, fail

        ; ---- 5. una sola vez en 70 ms
        ld a, CLR_TM
        out (PIC_CMD), a
        call rdrt
        ld (t0), hl
        ld hl, 0
        ld (acc), hl
w5:     call rdrt              ; acc += lo que avanza RT
        ld de, (t0)
        ld (t0), hl
        or a
        sbc hl, de
        ld de, (acc)
        add hl, de
        ld de, 35000           ; dos tramos de 35 ms
        or a
        sbc hl, de
        jr nc, w5h
        add hl, de
        ld (acc), hl
        in a, (PIC_PND)
        and 40h
        jr z, w5
        ld (p5), a             ; ha vuelto a saltar
        jr w5c2
w5h:    ld (acc), hl
        ld a, (half)
        inc a
        ld (half), a
        cp 2
        jr nz, w5
        in a, (PIC_PND)
        and 40h
        ld (p5), a
w5c2:   ld a, (p5)
        or a
        call nz, fail
        in a, (TCTL)
        ld (f_once), a
        and 10h
        call z, fail

        ; ---- 6. objetivo pasado: salta al armar
        ld a, CLR_TM
        out (PIC_CMD), a
        call rdrt
        ld de, 100
        or a
        sbc hl, de
        ld a, l
        out (TLO), a
        ld a, h
        out (TLO + 1), a
        nop
        in a, (PIC_PND)
        ld (p6), a
        and 40h
        call z, fail

        ld a, (sv_f1)
        out (PAGE), a
        ld a, b
        ld (nfail), a
        ei                     ; la ROM atiende la linea 6 y rearma

        ; ---- informe
        ld de, m_1
        ld hl, (r1)
        call pline
        ld de, m_2
        ld hl, (r2b)
        call pline
        ld de, m_3
        ld hl, (uv)
        ld de, (s0)
        or a
        sbc hl, de
        ld de, m_3
        call pline
        ld de, m_4
        ld hl, (r4)
        call pline
        ld de, m_4f
        call pstr
        ld a, (f_win)
        call phex
        call pspace
        ld a, (f_arm)
        call phex
        call pspace
        ld a, (f_fire)
        call phex
        call crlf
        ld de, m_5
        call pstr
        ld a, (p5)
        call phex
        call pspace
        ld a, (f_once)
        call phex
        call crlf
        ld de, m_6
        call pstr
        ld a, (p6)
        call phex
        call crlf
        ld a, (nfail)
        or a
        ld de, m_ok
        jr z, fin
        ld de, m_fail
fin:    jp pstr

; ---- HL = RT (instantanea del supervisor). Usa C.
rdrt:   ld c, TLO
        in l, (c)
        inc c
        in h, (c)
        ret

; ---- espera el siguiente VBlank (pendiente de la linea 2)
vb_edge: ld a, CLR_VB
        out (PIC_CMD), a
vbe1:   in a, (PIC_PND)
        and 04h
        jr z, vbe1
        ret

; ---- unos 4000 x 24 ciclos: de 5 ms (18 MHz) a 24 ms (4 MHz). Conserva C.
wait1ms: push bc
        ld bc, 4000
wm1:    dec bc
        ld a, b
        or c
        jr nz, wm1
        pop bc
        ret

; ---- HL >= DE, si no un fallo / HL < DE, si no un fallo (sin signo)
range_lo: push hl
        or a
        sbc hl, de
        pop hl
        ret nc
        jr fail
range_hi: push hl
        or a
        sbc hl, de
        pop hl
        ret c
fail:   inc b
        ret

; ---- user_test: el supervisor lee el par (s0), el usuario espera y lee el
; suyo (uv), y de vuelta el supervisor lee F5h solo (u_h) y luego otro par
; (s1). Como MAPTEST: en 7F10h, el supervisor hace OUT (F0h),A / RET y el
; usuario sigue en 7F13h (su pagina 1 esta en el slot 2, banco 41h).
user_test:
        push bc
        in a, (0E9h)
        ld (sv_e9), a
        in a, (0FBh)
        ld (sv_fb), a
        in a, (0F0h)
        ld (sv_f0), a
        ld a, 41h
        out (0E9h), a
        ld a, 0FBh
        out (0FAh), a
        ld hl, ucode
        ld de, T
        ld bc, img_len
        ldir
        ld a, 0FFh
        out (0FAh), a
        ld hl, scode
        ld de, T
        ld bc, img_len
        ldir
        ld a, 0FBh             ; usuario: pagina 1 en el slot 2, el resto slot 3
        out (0FBh), a
        call rdrt
        ld (s0), hl
        ld (ksp), sp
        ld hl, T + 3           ; la vuelta del RET
        push hl
        ld a, 16h              ; nativo + trampas + bloqueo; bit 0 = 0: salir
        jp T
back:   ld sp, (ksp)           ; ya en supervisor (por el OUT (F2h),A)
        ld c, TLO + 1
        in a, (c)              ; F5h sin F4h: el alto de s0
        ld (u_h), a
        call rdrt
        ld (s1), hl
        ld a, (sv_f0)
        out (0F0h), a
        ld a, (sv_fb)
        out (0FBh), a
        ld a, (sv_e9)
        out (0E9h), a
        pop bc
        ret

ucode:  ds 3, 0                ; 7F10h
        ld bc, 4000            ; 7F13h: mas de 1 ms
uw:     dec bc
        ld a, b
        or c
        jr nz, uw
        ld c, TLO
        in l, (c)
        inc c
        in h, (c)
        ld (uv), hl
uout:   out (0F2h), a          ; al supervisor (sigue 2 bytes despues)
        ds 3, 0
img_len equ $ - ucode
scode:  out (0F0h), a          ; 7F10h
        ret                    ; 7F12h: aqui sale
        ds uout - ucode - 1, 0
        jp back                ; tras el OUT (F2h),A del usuario

; ---- salida
pline:  push hl                ; DE = texto, HL = valor
        call pstr
        pop hl
        call pword
        jr crlf
pstr:   ld c, 9
        jp BDOS
crlf:   ld de, m_crlf
        jr pstr
pspace: ld e, ' '
        ld c, 2
        jp BDOS
phex:   push af
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
pword:  push hl
        ld a, h
        call phex
        pop hl
        ld a, l
        jr phex

m_hello: db "TIMTEST: system timer rev 6 (F4h-F6h)", 13, 10, "$"
m_1:     db "3 frames (VBlank): $"
m_2:     db "snapshot, F5h repeated; next read +$"
m_3:     db "user snapshot after > 1 ms: +$"
m_4:     db "compare +2000 us fired at +$"
m_4f:    db "F6h window/armed/fired: $"
m_5:     db "70 ms later (once): pending, F6h: $"
m_6:     db "past target: pending $"
m_crlf:  db 13, 10, "$"
m_ok:    db "OK", 13, 10, "$"
m_fail:  db "FAIL", 13, 10, "$"
sv_f1:   db 0
sv_e9:   db 0
sv_fb:   db 0
sv_f0:   db 0
nfail:   db 0
half:    db 0
p5:      db 0
p6:      db 0
f_win:   db 0
f_arm:   db 0
f_fire:  db 0
f_once:  db 0
u_h:     db 0
ksp:     dw 0
t0:      dw 0
acc:     dw 0
r1:      dw 0
r2a:     dw 0
r2b:     dw 0
r4:      dw 0
s0:      dw 0
s1:      dw 0
uv:      dw 0
