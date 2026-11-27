; =============================================================================
; TIMER_LX.ASM - Temporizador del sistema del modo LX (rev 6, puertos F4..F6)
; Shaeon (Carlos Palmero). Registros: aleste-xi-doc, 17-systime-v2.
;
; El hardware da RT, un contador libre de 16 bits a 1 MHz (F4/F5: el F4 copia
; RT en la instantanea del modo, F5 da su byte alto), y un comparador: F4/F5
; escritos fijan COMPARE y lo arman; cuando RT llega, salta la linea 6 del PIC
; una vez. Los milisegundos se cuentan aqui, sumando lo que ha avanzado RT
; desde la ultima lectura (upd). Para no perder vueltas de RT (65,5 ms), el
; comparador se rearma siempre a 30 ms como mucho: es tambien el periodico,
; que lleva en per_left lo que le falta, asi que acepta periodos de hasta
; 65535 us sin que el objetivo pase de los 32768 us que permite el hardware.
;
; get_ms y delay_ms funcionan sin interrupciones si se llaman a menudo (menos
; de 65 ms entre llamadas); con la linea 6 activa, siempre.
; =============================================================================

module timer_lx

defc PORT_MMIO_PAGE = 0xF1
defc TMR_LO         = 0xF4          ; R: instantanea (bajo) / W: COMPARE bajo
defc TMR_HI         = 0xF5          ; R: instantanea (alto) / W: COMPARE alto y arma
defc TMR_CTRL       = 0xF6          ; R: FIRED DUE WINDOW ARMED ENABLE / W: ENABLE
defc TMR_IRQ_LINE   = 6
defc KEEPALIVE      = 30000         ; us entre rearmes como mucho
defc PER_MIN        = 100           ; periodo minimo en us

EXTERN __int_set_handler, __int_enable, __int_disable

SECTION DRV_CODE

PUBLIC __timer_lx_vtable
__timer_lx_vtable:
    jp timer_lx_detect
    jp timer_lx_install
    jp timer_lx_get_info
    jp timer_lx_command
    jp timer_lx_uninstall
    jp timer_lx_get_ms
    jp timer_lx_delay_ms
    jp timer_lx_set_periodic
    jp timer_lx_get_count

; ---- irq_off: DI y P/V = como estaban (IFF2); uso:
;      call irq_off / push af ... pop af / call po_ei
;      LD A,I da P/V = 0 si se acepta una interrupcion mientras se ejecuta (Z80
;      NMOS): entonces se lee otra vez, ya con la interrupcion atendida.
irq_off:
    ld a, i
    jp pe, irq_was_on
    ld a, i
irq_was_on:
    di
    ret

; ---- po_ei: abre si P/V = 1, asi que sirve tambien dentro de una interrupcion
po_ei:
    ret po
    ei
    ret

; ---- detect: RT avanza (dos instantaneas separadas por unos 100 us)
timer_lx_detect:
    call irq_off
    push af
    call rd_rt
    push hl
    ld b, 0
det_w:
    djnz det_w                  ; unos 3300 ciclos, mas de 500 us a 6 MHz
    call rd_rt
    pop de
    pop af
    call po_ei
    or a
    sbc hl, de
    ld a, h
    or l
    ld a, 1
    ret nz
    xor a
    ret

timer_lx_install:
    call irq_off
    push af
    ld a, TMR_IRQ_LINE
    ld hl, timer_isr
    call __int_set_handler      ; abre las interrupciones
    di
    ld hl, 0
    ld (us_acc), hl
    ld (ms), hl
    ld (count), hl
    ld (per_us), hl
    ld (per_left), hl
    ld (per_over), hl
    ld (user_handler), hl
    call rd_rt
    ld (last_rt), hl
    call arm
    ld a, TMR_IRQ_LINE
    call __int_enable           ; abre las interrupciones
    di
    pop af
    call po_ei
    xor a
    ret

timer_lx_uninstall:
    call irq_off
    push af
    xor a
    out (TMR_CTRL), a           ; ENABLE = 0
    ld hl, 0
    ld (user_handler), hl
    ld (per_us), hl
    ld a, TMR_IRQ_LINE
    call __int_disable
    di
    pop af
    call po_ei
    ret

timer_lx_get_info:
    ld hl, info
    ret

timer_lx_command:
    ld hl, 0
    ret

; ---- HL = milisegundos (16 bits, da la vuelta cada 65,5 s). Conserva BC, DE.
timer_lx_get_ms:
    push bc
    push de
    call irq_off
    push af
    call upd
    pop af
    call po_ei
    ld hl, (ms)
    pop de
    pop bc
    ret

; ---- espera HL milisegundos
timer_lx_delay_ms:
    ex de, hl                   ; DE = espera
    call timer_lx_get_ms
    ld b, h
    ld c, l                     ; BC = inicio
dly_loop:
    call timer_lx_get_ms
    or a
    sbc hl, bc                  ; HL = transcurrido
    or a
    sbc hl, de
    jr c, dly_loop
    ret

; ---- HL = periodo en us, DE = rutina (0 = parar)
timer_lx_set_periodic:
    call irq_off
    push af
    push hl
    push de
    call upd                    ; lo anterior se cuenta con el estado viejo
    pop de
    pop hl
    ld a, d
    or e
    jr nz, per_on
    ld hl, 0                    ; parar: solo queda el rearme de 30 ms
    ld (user_handler), hl
    ld (per_us), hl
    jr per_arm
per_on:
    push de
    ld de, PER_MIN
    or a
    sbc hl, de
    add hl, de
    jr nc, per_min_ok
    ex de, hl                   ; menos del minimo: PER_MIN
per_min_ok:
    pop de
    ld (per_us), hl
    ld (per_left), hl
    ld (user_handler), de
    ld hl, 0
    ld (per_over), hl
    ld (count), hl
per_arm:
    call arm
    pop af
    call po_ei
    ret

timer_lx_get_count:
    ld hl, (count)
    ret

; ---- rutina del PIC (linea 6, interrupciones cerradas)
timer_isr:
    call upd
    ld hl, (per_us)
    ld a, h
    or l
    jr z, isr_arm               ; sin periodico: solo el rearme
    ld hl, (per_left)
    ld a, h
    or l
    jr nz, isr_arm              ; aun no toca
    ; toca: el siguiente, contando el retraso para no derivar
    ld hl, (per_us)
    ld de, (per_over)
    or a
    sbc hl, de                  ; per_us - retraso
    jr z, isr_late
    jr nc, isr_left
isr_late:
    ld hl, (per_us)             ; se ha perdido un periodo entero: sin recuperar
isr_left:
    ld (per_left), hl
    ld hl, 0
    ld (per_over), hl
    ld hl, (count)
    inc hl
    ld (count), hl
    ld hl, (user_handler)
    call jp_hl
isr_arm:
    jp arm

jp_hl:
    jp (hl)

; ---- HL = RT (instantanea del modo). Usa A, C.
rd_rt:
    ld c, TMR_LO
    in l, (c)
    inc c
    in h, (c)
    ret

; ---- suma lo que ha avanzado RT desde la ultima vez a ms/us_acc y lo
;      descuenta de per_left. Con las interrupciones cerradas. HL = RT.
;      Usa A, BC, DE.
upd:
    call rd_rt
    push hl
    ld de, (last_rt)
    ld (last_rt), hl
    or a
    sbc hl, de                  ; HL = delta (0..65535 us)
    push hl
    ; periodico: per_left -= delta; si no llega, per_over += lo que falta
    ld a, (per_us)
    ld b, a
    ld a, (per_us + 1)
    or b
    jr z, upd_np
    ex de, hl                   ; DE = delta
    ld hl, (per_left)
    or a
    sbc hl, de
    jr nc, upd_pl
    ; HL = per_left - delta < 0: retraso = delta - per_left = -HL
    xor a
    sub l
    ld l, a
    sbc a, a
    sub h
    ld h, a                     ; HL = -HL
    ld de, (per_over)
    add hl, de
    jr nc, upd_ov
    ld hl, 0xFFFF
upd_ov:
    ld (per_over), hl
    ld hl, 0
upd_pl:
    ld (per_left), hl
upd_np:
    pop hl                      ; HL = delta
    ld de, (us_acc)
    add hl, de
    ld a, 0                     ; A = ms que pasan
    jr nc, upd_nc
    ld de, 536                  ; 65536 us = 65 ms + 536 us
    add hl, de
    ld a, 65
upd_nc:
    ld de, 1000
upd_l:
    or a
    sbc hl, de
    jr c, upd_e
    inc a
    jr upd_l
upd_e:
    add hl, de
    ld (us_acc), hl
    ld hl, (ms)
    ld e, a
    ld d, 0
    add hl, de
    ld (ms), hl
    pop hl                      ; HL = RT
    ret

; ---- arma el comparador a min(per_left, KEEPALIVE) desde RT.
;      Con las interrupciones cerradas. Usa A, BC, DE, HL.
arm:
    call upd                    ; HL = RT, per_left al dia
    ex de, hl                   ; DE = RT
    ld bc, KEEPALIVE
    ld hl, (per_us)
    ld a, h
    or l
    jr z, arm_set
    ld hl, (per_left)
    or a
    sbc hl, bc
    add hl, bc
    jr nc, arm_set              ; per_left >= KEEPALIVE
    ld b, h
    ld c, l                     ; BC = per_left (0 = ya: salta al armar)
arm_set:
    ex de, hl
    add hl, bc                  ; HL = RT + espera
    ld a, l
    out (TMR_LO), a             ; abre WINDOW (no entra INT hasta F5)
    ld a, h
    out (TMR_HI), a             ; arma, ENABLE = 1, FIRED = 0
    ret

SECTION DRV_DATA

info:
    dw info_name
    db 1, 0
    dw 0x0003
    db TMR_IRQ_LINE, 0
info_name:
    db "TIMER LX", 0

last_rt:      dw 0
us_acc:       dw 0
ms:           dw 0
count:        dw 0
per_us:       dw 0
per_left:     dw 0
per_over:     dw 0
user_handler: dw 0
