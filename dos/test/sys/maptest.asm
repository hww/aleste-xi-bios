; maptest.asm - MAPTEST.COM: de que mapa sale cada instruccion tras la salida
; del supervisor (OUT (F0h),A la arma; sale en el M1 que lee el C9 de un RET)
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; En 7F10h (pagina 1, distinta en los dos mapas):
;   supervisor: OUT (F0h),A / RET / LD B,1 / LD C,1 / LD D,1 / ...
;   usuario:    (3 bytes)         / LD B,2 / LD C,2 / LD D,2 / OUT (F2h),A
; El RET vuelve a 7F13h (en la pila, que es la misma en los dos mapas) y,
; tras el OUT (F2h),A del usuario, el supervisor sigue en 7F1Bh con JP fin.
; Imprime B, C y D: 1 = esa instruccion salio del mapa del supervisor,
; 2 = del usuario (lo esperado: 2, 2 y 2). Sin interrupciones.
; pasmo maptest.asm MAPTEST.COM
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
        ld bc, ucode_end - ucode
        ldir
        ld a, 0FFh
        out (0FAh), a
        ld hl, scode           ; la del supervisor (TPA)
        ld de, T
        ld bc, scode_end - scode
        ldir
        ld a, 0FBh             ; usuario: pagina 1 en el slot 2, el resto slot 3
        out (0FBh), a
        ld (ksp), sp
        ld b, 0
        ld c, 0
        ld d, 0
        ld hl, T + 3           ; la vuelta del RET
        push hl
        ld a, 06h              ; nativo + trampas; bit 0 = 0: salir en el RET
        jp T
back:   ld sp, (ksp)           ; ya en supervisor
        ld (rb), bc
        ld a, d
        ld (rd), a
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
        ld a, (rb + 1)
        call pdig
        ld de, m_c
        ld c, 9
        call BDOS
        ld a, (rb)
        call pdig
        ld de, m_d
        ld c, 9
        call BDOS
        ld a, (rd)
        call pdig
        ld de, m_end
        ld c, 9
        jp BDOS
pdig:   add a, '0'
        ld e, a
        ld c, 2
        jp BDOS

scode:  out (0F0h), a          ; 7F10h
        ret                    ; 7F12h: aqui sale
        ld b, 1                ; 7F13h
        ld c, 1                ; 7F15h
        ld d, 1                ; 7F17h
        nop                    ; 7F19h
        nop                    ; 7F1Ah
        jp back                ; 7F1Bh (tras el OUT (F2h),A del usuario)
scode_end:
ucode:  nop                    ; 7F10h
        nop                    ; 7F11h
        nop                    ; 7F12h
        ld b, 2                ; 7F13h
        ld c, 2                ; 7F15h
        ld d, 2                ; 7F17h
        out (0F2h), a          ; 7F19h: al supervisor
        nop                    ; 7F1Bh (no se ejecuta)
        nop
        nop
ucode_end:

m_hello: db "MAPTEST: 1 = supervisor map, 2 = user map", 13, 10, "$"
m_b:     db "1st after the RET: $"
m_c:     db 13, 10, "2nd after the RET: $"
m_d:     db 13, 10, "3rd after the RET: $"
m_end:   db 13, 10, "$"
ksp:     dw 0
rb:      dw 0
rd:      db 0
sv_e9:   db 0
sv_fb:   db 0
sv_f0:   db 0
