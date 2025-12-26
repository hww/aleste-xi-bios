; ==============================================================================
; CRTC 6845 DRIVER (ALESTE LX)
; ==============================================================================

SECTION DRV_VTABLE_VIDEO
_crtc_vtable:
    jp crtc_detect      ; +00
    jp crtc_install     ; +03
    jp crtc_get_info    ; +06
    jp crtc_ioctl       ; +09: A=Cmd, B=Reg, C=Val@ Out: A=Val
    jp crtc_set_mode    ; +12: A=Mode ID
    jp crtc_set_view    ; +15: H=Slot, L=Page (Offset)

SECTION DRV_DATA
_current_mode: db 0xFF
_info_str:     db "CRTC 6845 LX", 0

SECTION DRV_CODE

; ----------------------------------------------------------------------------
; crtc_detect (+00)
; ----------------------------------------------------------------------------
crtc_detect:
    ld a, 0x0C          ; Start Addr High
    out (0x20), a
    ld a, 0x55          ; Test pattern
    out (0x21), a
    in a, (0x21)
    cp 0x55
    ld a, 1
    ret z
    xor a               ; Not found
    scf
    ret

; ----------------------------------------------------------------------------
; crtc_install (+03)
; ----------------------------------------------------------------------------
crtc_install:
    ld a, 0             ; По умолчанию включаем режим 0
    jp crtc_set_mode

; ----------------------------------------------------------------------------
; crtc_get_info (+06)
; ----------------------------------------------------------------------------
crtc_get_info:
    ld hl, _info_str
    ret

; ----------------------------------------------------------------------------
; crtc_ioctl (+09) - HAL Access
; A=0: Write Reg (B=Reg, C=Val)
; A=1: Read Reg  (B=Reg) -> Out A
; A=2: Wait VSync
; ----------------------------------------------------------------------------
crtc_ioctl:
    or a
    jr z, @write_reg
    dec a
    jr z, @read_reg
    dec a
    jr z, @wait_vsync
    ret

@write_reg:
    ld a, b \ out (0x20), a
    ld a, c \ out (0x21), a
    ret

@read_reg:
    ld a, b \ out (0x20), a
    in a, (0x21)
    ret

@wait_vsync:
    ; Для Aleste LX VSync обычно ловится в статусном порту или через 6845
    ; Здесь классический опрос бита 3 (VSync) в 6845 Status (если реализовано)
    in a, (0x20)
    and 0x08
    jr z, @wait_vsync
    ret

; ----------------------------------------------------------------------------
; crtc_set_mode (+12)
; A = Mode ID
; ----------------------------------------------------------------------------
crtc_set_mode:
    ld (_current_mode), a
    
    ; Логика получения таблицы (как в твоем исходнике)
    call get_mode_table_ptr ; Вход: A, Выход: HL=Table, B=Size
    
    ld c, 0x20
@loop:
    ld a, (hl) \ out (c), a \ inc hl
    inc c
    ld a, (hl) \ out (c), a \ inc hl
    dec c
    djnz @loop
    ret

; ----------------------------------------------------------------------------
; crtc_set_view (+15)
; H = Slot (0-3), L = Page (0-255)
; ----------------------------------------------------------------------------
crtc_set_view:
    ld a, 0x0C \ out (0x20), a
    ld a, h    \ out (0x21), a
    ld a, 0x0D \ out (0x20), a
    ld a, l    \ out (0x21), a
    ret

; --- Вспомогательная логика ---
get_mode_table_ptr:
    ; Вход: A = номер режима
    ; Выход: HL = адрес таблицы (где первым байтом идет количество), B = количество
    ld l, a
    ld h, 0
    add hl, hl          ; hl = index * 2 (так как в crtc_mode_tables dw)
    ld de, crtc_mode_tables
    add hl, de
    
    ld e, (hl)
    inc hl
    ld d, (hl)          ; de = адрес начала данных мода
    ex de, hl           ; hl = адрес данных
    ld b, (hl)          ; b = количество регистров
    inc hl              ; hl указывает на первую пару [reg, val]
    ret

include "crtc_modes.asm"