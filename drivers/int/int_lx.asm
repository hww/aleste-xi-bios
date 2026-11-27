; =============================================================================
; INT_LX.ASM - Interrupciones con el PIC v2 del modo LX (pagina MMIO 0, FF0000)
; Shaeon (Carlos Palmero). Registros: aleste-xi-doc, 12-pic-v2.
;
; Despachador de IM 1: guarda registros y pagina MMIO y atiende las lineas
; con CLAIM -> rutina -> COMPLETE hasta que CLAIM no devuelve ninguna. El
; PIC elige la de mas prioridad (a igual prioridad, la de numero menor).
; Lineas: 0 disquetera, 1 DMA, 2 VBlank, 3 Magic Sound 2, 4 Gate Array
; (300 Hz), 6 temporizador.
; =============================================================================

module int_lx

defc PORT_MMIO_PAGE = 0xF1
defc PIC_PAGE       = 0
defc PIC_COMMAND    = 0x00
defc PIC_CLAIM      = 0x01
defc PIC_VECTOR     = 0x02
defc PIC_ENABLE_LO  = 0x09

; ordenes (bits 6..4) | linea (bits 3..0)
defc CMD_COMPLETE   = 0x10
defc CMD_ENABLE     = 0x20
defc CMD_DISABLE    = 0x30
defc CMD_CLEAR_BOTH = 0x60

SECTION DRV_CODE

PUBLIC __int_lx_vtable
__int_lx_vtable:
    jp int_lx_detect
    jp int_lx_install
    jp int_lx_get_info
    jp int_lx_command
    jp int_lx_uninstall
    jp int_lx_set_handler
    jp int_lx_enable
    jp int_lx_disable
    jp int_lx_isr

; ---- detect: el registro VECTOR del PIC se lee como se escribe
int_lx_detect:
    ld a, PIC_PAGE
    out (PORT_MMIO_PAGE), a
    in a, (PIC_VECTOR)
    ld c, a
    ld a, 0x5A
    out (PIC_VECTOR), a
    in a, (PIC_VECTOR)
    ld b, a
    ld a, c
    out (PIC_VECTOR), a
    ld a, b
    cp 0x5A
    ld a, 1
    ret z
    xor a
    ret

; ---- install: todas las lineas deshabilitadas, borradas y fuera de
;      servicio, prioridad 0, sin rutinas; IM 1 y EI
int_lx_install:
    di
    ld hl, handlers
    ld b, 32
int_clr:
    ld (hl), 0
    inc hl
    djnz int_clr
    ld a, PIC_PAGE
    out (PORT_MMIO_PAGE), a
    ld b, 0
int_ini:
    ld a, CMD_DISABLE
    or b
    out (PIC_COMMAND), a
    and 0x0F
    or CMD_CLEAR_BOTH
    out (PIC_COMMAND), a
    and 0x0F
    or CMD_COMPLETE
    out (PIC_COMMAND), a
    and 0x0F
    or 0x80                     ; prioridad 0
    out (PIC_COMMAND), a
    inc b
    bit 4, b
    jr z, int_ini
    im 1
    ei
    xor a
    ret

int_lx_uninstall:
    di
    ld a, PIC_PAGE
    out (PORT_MMIO_PAGE), a
    ld b, 0
int_un:
    ld a, CMD_DISABLE
    or b
    out (PIC_COMMAND), a
    inc b
    bit 4, b
    jr z, int_un
    ret

int_lx_get_info:
    ld hl, info
    ret

; sin ordenes propias
int_lx_command:
    ld hl, 0
    ret

; ---- set_handler: A = linea, HL = rutina
int_lx_set_handler:
    and 0x0F
    ex de, hl
    ld l, a
    ld h, 0
    add hl, hl
    ld bc, handlers
    add hl, bc
    di                          ; los dos bytes sin que entre una interrupcion
    ld (hl), e
    inc hl
    ld (hl), d
    ei
    ret

; ---- enable / disable: A = linea. El PIC guarda el estado (ENABLE_L/H).
int_lx_enable:
    and 0x0F
    or CMD_ENABLE
    jr cmd_out
int_lx_disable:
    and 0x0F
    or CMD_DISABLE
cmd_out:
    ld c, a
    di
    in a, (PORT_MMIO_PAGE)
    ld b, a
    ld a, PIC_PAGE
    out (PORT_MMIO_PAGE), a
    ld a, c
    out (PIC_COMMAND), a
    ld a, b
    out (PORT_MMIO_PAGE), a
    ei
    ret

; ---- IM 1: despacha y vuelve
int_lx_isr:
    call __int_lx_dispatch
    ei
    reti

; ---- despachador (sin EI/RETI), tambien para las puertas del LX-DOS
;      (bios/dosgate.asm), que lo llaman con otro mapa de memoria
PUBLIC __int_lx_dispatch
__int_lx_dispatch:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    in a, (PORT_MMIO_PAGE)      ; pagina del programa interrumpido
    push af
isr_next:
    ld a, PIC_PAGE              ; la rutina puede haber cambiado la pagina
    out (PORT_MMIO_PAGE), a
    in a, (PIC_CLAIM)           ; bit 7 VALID, bits 3..0 linea
    or a
    jp p, isr_done
    and 0x0F
    ld b, a
    add a, a
    ld e, a
    ld d, 0
    ld hl, handlers
    add hl, de
    ld a, (hl)
    inc hl
    ld h, (hl)
    ld l, a
    or h
    push bc
    ld a, b
    call nz, jp_hl              ; A = linea
    pop bc
    ld a, PIC_PAGE
    out (PORT_MMIO_PAGE), a
    ld a, CMD_COMPLETE
    or b
    out (PIC_COMMAND), a
    jr isr_next
isr_done:
    pop af
    out (PORT_MMIO_PAGE), a
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ret

jp_hl:
    jp (hl)

SECTION DRV_DATA

info:
    dw info_name
    db 1, 0
    dw 0x0003                   ; usa IRQ, estatico
    db 0, 0
info_name:
    db "INT LX PIC", 0

handlers: ds 32
