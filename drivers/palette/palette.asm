; ==============================================================================
; PALETTE DRIVER (ALESTE LX) - NATIVE ONLY (RGB/YJK)
; ==============================================================================

SECTION DRV_VTABLE_PALETTE
ALIGN 32
_pal_vtable:
    jp pal_detect        ; +00
    jp pal_install       ; +03
    jp pal_get_info      ; +06
    jp pal_ioctl         ; +09: A=Cmd (0:Mode, 1:Modifier)
    jp pal_set_color     ; +12: A=Index, BC=12/15-bit Color (0RRRGGGG BBBBBBBB)
    jp pal_set_border    ; +15: BC=Color
    jp pal_load_buffer   ; +18: HL=Ptr to buffer, B=Count (0=256)

SECTION DRV_DATA
_pal_info_str:  db "PALETTE LX NATIVE", 0
_pal_ctrl_reg:  db 0b00100000 ; По умолчанию: 12-bit mode + Auto-increment

SECTION DRV_CODE

; --- Внутренний хелпер выбора страницы MMIO ---
_pal_select_page:
    ld a, 2             ; Страница 2: Регистры палитры
    out (0xF1), a
    ret

pal_detect:
    ld a, 1             ; Палитра доступна всегда
    ret

pal_install:
    call _pal_select_page
    ld a, (_pal_ctrl_reg)
    out (0x05), a       ; Инициализация контрольного регистра
    ret

pal_get_info:
    ld hl, _pal_info_str
    ret

; --- IOCTL (+09) ---
; A=0: Установка режима (B=0:RGB, B=1:YJK, B=2:15-bit)
; A=1: Установка модификатора (B=Value)
pal_ioctl:
    push af
    call _pal_select_page
    pop af
    or a
    jr z, @set_mode
    cp 1
    jr z, @set_mod
    ret

@set_mode:
    ; Здесь мы можем менять биты в R5 (Control Register)
    ; Например, переключение между RGB и YJK или битности
    ld a, b
    ; [Логика сборки байта для R5 на основе B]
    out (0x05), a
    ret

@set_mod:
    ld a, b
    out (0x06), a       ; PAL_MODIFIER (OR/XOR эффекты на весь экран)
    ret

; --- Установка одного цвета (+12) ---
pal_set_color:
    push af
    call _pal_select_page
    pop af
    out (0x00), a       ; Установить индекс (A)
    ld a, c \ out (0x01), a ; Low byte (GGGG BBBB или часть 15-bit)
    ld a, b \ out (0x02), a ; High byte (0RRR RGGG или 15-bit)
    ret

; --- Установка бордюра (+15) ---
pal_set_border:
    call _pal_select_page
    ld a, c \ out (0x03), a ; Border Low
    ld a, b \ out (0x04), a ; Border High
    ret

; --- Массовая загрузка (+18) ---
; HL: указатель на массив uint16_t, B: количество цветов
pal_load_buffer:
    call _pal_select_page
    xor a
    out (0x00), a       ; Начинаем с индекса 0
    ; Автоинкремент индекса в R5 должен быть включен!
@loop:
    ld a, (hl) \ inc hl \ out (0x01), a
    ld a, (hl) \ inc hl \ out (0x02), a
    djnz @loop
    ret