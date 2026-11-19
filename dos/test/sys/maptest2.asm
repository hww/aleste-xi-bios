; maptest2.asm - MAPTEST2.COM: en la salida del supervisor (el M1 que lee el
; C9 de un RET), de que mapa salen la pila del RET y el codigo al que vuelve
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; La pila esta en 7F40h, en la pagina 1, que es distinta en los dos mapas:
; en el del supervisor 7F40h vale 7F20h, y en el del usuario, 7F28h.
;   7F10h  supervisor: OUT (F0h),A / RET
;   7F20h  supervisor: LD B,1 / ... / JP fin  usuario: LD B,4 / OUT (F2h),A
;   7F28h  supervisor: LD B,3 / ... / JP fin  usuario: LD B,2 / OUT (F2h),A
; B: 2 = pila y codigo del usuario (lo esperado), 4 = pila del supervisor y
; codigo del usuario, 3 = pila del usuario y codigo del supervisor, 1 = todo
; del supervisor. Tras el OUT (F2h),A el supervisor sigue en el JP fin.
; Sin interrupciones.
; pasmo maptest2.asm MAPTEST2.COM
BDOS    equ 5
T       equ 7F10h
        org 100h
        ld de, m_hello
        ld c, 9
        call BDOS
        di
        in a, (0E9h)
        ld (sv_e9), a
        in a, (0FBh)
        ld (sv_fb), a
        in a, (0F0h)
        ld (sv_f0), a
        ld a, 41h              ; pagina 1 del usuario: slot 2, banco 41h
        out (0E9h), a
        ld a, 0FBh             ; verla por la pagina 1 del supervisor
        out (0FAh), a
        ld hl, ucode
        ld de, T
        ld bc, img_len
        ldir
        ld a, 0FFh
        out (0FAh), a
        ld hl, scode           ; la del supervisor (TPA)
        ld de, T
        ld bc, img_len
        ldir
        ld a, 0FBh             ; usuario: pagina 1 en el slot 2, el resto slot 3
        out (0FBh), a
        ld (ksp), sp
        ld sp, T + 30h         ; 7F40h
        ld b, 0
        ld a, 06h              ; nativo + trampas; bit 0 = 0: salir en el RET
        jp T
back:   ld sp, (ksp)           ; ya en supervisor
        ld a, b
        ld (rb), a
        ld a, (sv_f0)
        out (0F0h), a
        ld a, (sv_fb)
        out (0FBh), a
        ld a, (sv_e9)
        out (0E9h), a
        ei
        ld de, m_b
        ld c, 9
        call BDOS
        ld a, (rb)
        add a, '0'
        ld e, a
        ld c, 2
        call BDOS
        ld de, m_end
        ld c, 9
        jp BDOS

; las dos imagenes de 7F10h-7F41h
scode:  out (0F0h), a          ; 7F10h
        ret                    ; 7F12h: aqui sale
        ds 0Dh, 0
        ld b, 1                ; 7F20h
        nop
        nop
        jp back                ; 7F24h
        ds 1, 0
        ld b, 3                ; 7F28h
        nop
        nop
        jp back                ; 7F2Ch
        ds 17, 0
        dw T + 10h             ; 7F40h: la vuelta segun el supervisor
img_len equ $ - scode
ucode:  ds 10h, 0              ; 7F10h
        ld b, 4                ; 7F20h
        out (0F2h), a          ; 7F22h: al supervisor (sigue en 7F24h)
        ds 4, 0
        ld b, 2                ; 7F28h
        out (0F2h), a          ; 7F2Ah: al supervisor (sigue en 7F2Ch)
        ds 20, 0
        dw T + 18h             ; 7F40h: la vuelta segun el usuario

m_hello: db "MAPTEST2: exit on RET: 2 = user stack and code (expected),", 13, 10
         db "4 = supervisor stack, user code, 3 = user stack, supervisor code", 13, 10, "$"
m_b:     db "B = $"
m_end:   db 13, 10, "$"
ksp:     dw 0
rb:      db 0
sv_e9:   db 0
sv_fb:   db 0
sv_f0:   db 0
