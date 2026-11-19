; timtest.asm - TIMTEST.COM: el temporizador libre de 1 MHz de F4h-F7h
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; F4h-F7h: contador libre de 32 bits a 1 MHz (F4h = bits 7-0 ... F7h = bits
; 31-24), dentro de la MMU; se lee tambien en modo usuario con el bloqueo de
; puertos. Una lectura de F4h-F7h hace una copia de los 32 bits; las
; siguientes leen esa copia mientras lleguen con como mucho tres M1 desde la
; anterior (una instruccion en medio, contando el prefijo ED de IN r,(C)).
;  1. Ritmo: 100 ticks del contador de milisegundos del sistema (pagina MMIO
;     0, 45h/46h) tienen que ser 100.000 us (+-100).
;  2. Copia: IN A,(C) / NOP / IN E,(C) leen lo mismo (el contador avanza
;     entre las dos: 4 us a 4 MHz); con cuatro NOP en medio, u ocho, la
;     segunda lectura es nueva.
;  3. Modo usuario con el bloqueo (F0h bit 4): lee los 32 bits con
;     IN r,(C) / INC C y vuelve con OUT (F2h),A; el valor tiene que estar
;     entre los que lee el supervisor antes y despues.
; Sin interrupciones mientras mide.
; pasmo timtest.asm TIMTEST.COM
BDOS    equ 5
TMR     equ 0F4h
T       equ 7F10h
        org 100h
        ld de, m_hello
        call pstr
        di
        in a, (0F1h)
        ld (sv_f1), a
        ; ---- 1. ritmo, con el contador de milisegundos
        xor a                  ; pagina MMIO 0 (el sistema)
        out (0F1h), a
        call ms_edge           ; espera a que cambie el milisegundo
        ld (ms0), hl
        call rd32
        ld (t0), hl
        ld (t0 + 2), de
r1:     call ms_now            ; hasta 100 ms despues
        ld de, (ms0)
        or a
        sbc hl, de
        ld de, 100
        or a
        sbc hl, de
        jr c, r1
        call rd32
        ld (t1), hl
        ld (t1 + 2), de
        ld a, (sv_f1)
        out (0F1h), a
        ; delta = t1 - t0
        ld hl, (t1)
        ld de, (t0)
        or a
        sbc hl, de
        ld (dt), hl
        ld hl, (t1 + 2)
        ld de, (t0 + 2)
        sbc hl, de
        ld (dt + 2), hl
        ; ---- 2. la copia
        ld c, TMR
        in a, (c)
        nop
        nop
        nop
        nop                    ; cuatro M1: la siguiente lectura es nueva
        in a, (c)
        nop                    ; uno (con el prefijo ED, tres): la misma copia
        in e, (c)
        ld (c_same), a
        ld a, e
        ld (c_same + 1), a
        in a, (c)
        ld b, a
        nop
        nop
        nop
        nop
        nop
        nop
        nop                    ; ocho M1 en medio: lectura nueva
        in e, (c)
        ld a, b
        ld (c_new), a
        ld a, e
        ld (c_new + 1), a
        ; ---- 3. modo usuario con el bloqueo
        call user_test
        ei
        ; ---- informe
        ld de, m_rate
        call pstr
        ld hl, (dt + 2)
        call pword
        ld hl, (dt)
        call pword
        ld de, m_rate2
        call pstr
        ld de, m_same
        call pstr
        ld a, (c_same)
        call phex
        call pspace
        ld a, (c_same + 1)
        call phex
        ld de, m_new
        call pstr
        ld a, (c_new)
        call phex
        call pspace
        ld a, (c_new + 1)
        call phex
        ld de, m_user
        call pstr
        ld hl, (u0 + 2)
        call pword
        ld hl, (u0)
        call pword
        call pspace
        ld hl, (uv + 2)
        call pword
        ld hl, (uv)
        call pword
        call pspace
        ld hl, (u1 + 2)
        call pword
        ld hl, (u1)
        call pword
        call crlf
        ; ---- comprobar
        ld b, 0
        ld hl, (dt + 2)        ; 100000 = 186A0h, +-100
        ld a, h
        or a
        jr nz, bad1
        ld a, l
        cp 1
        jr nz, bad1
        ld hl, (dt)
        ld de, 86A0h - 100
        or a
        sbc hl, de
        jr c, bad1
        ld de, 200
        or a
        sbc hl, de
        jr c, ok1
bad1:   inc b
ok1:    ld a, (c_same)
        ld hl, c_same + 1
        cp (hl)
        jr z, ok2
        inc b
ok2:    ld a, (c_new)
        ld hl, c_new + 1
        cp (hl)
        jr nz, ok3
        inc b
ok3:    ld hl, uv              ; u0 < uv < u1
        ld de, u0
        call cmp32
        jr nc, ok4
        inc b
ok4:    ld hl, u1
        ld de, uv
        call cmp32
        jr nc, ok5
        inc b
ok5:    ld a, b
        or a
        ld de, m_ok
        jr z, fin
        ld de, m_fail
fin:    jp pstr

; ---- rd32: los 32 bits en DEHL (IN r,(C) / INC C: una sola copia)
rd32:   ld c, TMR
        in l, (c)
        inc c
        in h, (c)
        inc c
        in e, (c)
        inc c
        in d, (c)
        ret

; ---- el contador de milisegundos (al leer 45h se congela 46h)
ms_now: in a, (45h)
        ld l, a
        in a, (46h)
        ld h, a
        ret
ms_edge: call ms_now
        ld (ms0), hl
me1:    call ms_now
        ld de, (ms0)
        or a
        sbc hl, de
        jr z, me1
        jp ms_now

; ---- cmp32: (HL) - (DE) en 32 bits; C si (HL) <= (DE)
cmp32:  push bc
        ld b, 4
        or a
        push hl
        push de
cm1:    ld a, (de)
        ld c, a
        ld a, (hl)
        sbc a, c
        inc hl
        inc de
        djnz cm1
        pop de
        pop hl
        pop bc
        ret c
        ; iguales?
        push bc
        ld b, 4
cm2:    ld a, (de)
        cp (hl)
        jr nz, cm3
        inc hl
        inc de
        djnz cm2
        pop bc
        scf                    ; iguales: no vale
        ret
cm3:    pop bc
        or a
        ret

; ---- user_test: lee el temporizador en modo usuario con el bloqueo
; Como MAPTEST: en 7F10h, el supervisor hace OUT (F0h),A / RET y el usuario
; sigue en 7F13h (su pagina 1 esta en el slot 2, banco 41h).
user_test:
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
        call rd32
        ld (u0), hl
        ld (u0 + 2), de
        ld (ksp), sp
        ld hl, T + 3           ; la vuelta del RET
        push hl
        ld a, 16h              ; nativo + trampas + bloqueo; bit 0 = 0: salir
        jp T
back:   ld sp, (ksp)           ; ya en supervisor (por el OUT (F2h),A)
        ld a, (sv_f0)
        out (0F0h), a
        ld a, (sv_fb)
        out (0FBh), a
        ld a, (sv_e9)
        out (0E9h), a
        call rd32              ; (tras mas de tres M1: lectura nueva)
        ld (u1), hl
        ld (u1 + 2), de
        ret

scode:  out (0F0h), a          ; 7F10h
        ret                    ; 7F12h: aqui sale
        ds 22, 0               ; 7F13h-7F28h
        jp back                ; 7F29h (tras el OUT (F2h),A del usuario)
img_len equ $ - scode
ucode:  ds 3, 0                ; 7F10h
        ld c, TMR              ; 7F13h
        in l, (c)              ; 7F15h
        inc c
        in h, (c)
        inc c
        in e, (c)
        inc c
        in d, (c)              ; 7F1Eh
        ld (uv), hl            ; 7F20h
        ld (uv + 2), de        ; 7F23h
        out (0F2h), a          ; 7F27h: al supervisor (sigue en 7F29h)
        ds 3, 0

; ---- salida
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

m_hello: db "TIMTEST: 1 MHz timer at F4h-F7h", 13, 10, "$"
m_rate:  db "100 ms of the system timer = $"
m_rate2: db "h us (186A0h expected)", 13, 10, "$"
m_same:  db "reads 1 NOP apart: $"
m_new:   db " (same)", 13, 10, "reads 8 NOPs apart: $"
m_user:  db " (different)", 13, 10, "user mode, locked: $"
m_crlf:  db 13, 10, "$"
m_ok:    db "OK", 13, 10, "$"
m_fail:  db "FAIL", 13, 10, "$"
sv_f1:   db 0
sv_e9:   db 0
sv_fb:   db 0
sv_f0:   db 0
ksp:     dw 0
ms0:     dw 0
t0:      ds 4
t1:      ds 4
dt:      ds 4
u0:      ds 4
uv:      ds 4
u1:      ds 4
c_same:  ds 2
c_new:   ds 2
