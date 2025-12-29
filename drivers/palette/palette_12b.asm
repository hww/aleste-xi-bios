; =============================================================================
; PALETTE_12B@ASM - 12-bit RGB Palette Driver
; For Aleste LX BIOS
; =============================================================================

module palette_12b

; -----------------------------------------------------------------------------
; ВСЕ defc константы этого драйвера
; -----------------------------------------------------------------------------

; Регистры палитры в MMIO (страница 2)
defc PAL_REG_INDEX     = 0x00
defc PAL_REG_DATA_LO   = 0x01
defc PAL_REG_DATA_HI   = 0x02
defc PAL_REG_BORDER_LO = 0x03
defc PAL_REG_BORDER_HI = 0x04
defc PAL_REG_CTRL      = 0x05
defc PAL_REG_MODIFIER  = 0x06

; Биты управляющего регистра
defc PAL_CTRL_MOD_ENABLE  = 0x80
defc PAL_CTRL_MOD_XOR     = 0x40
defc PAL_CTRL_AUTOINC     = 0x20
defc PAL_CTRL_MODE_MASK   = 0x03

; Режимы палитры
defc PAL_MODE_12BIT    = 0x00
defc PAL_MODE_15BIT    = 0x01
defc PAL_MODE_CPC      = 0x02
defc PAL_MODE_MSX      = 0x03
defc PAL_MODE_YJK      = 0x04

; Команды для palette_command
defc PAL_CMD_SET_MODE      = 0x10
defc PAL_CMD_SET_MODIFIER  = 0x11
defc PAL_CMD_SET_AUTOINC   = 0x12
defc PAL_CMD_SAVE_STATE    = 0x20
defc PAL_CMD_RESTORE_STATE = 0x21

; Системные порты
defc PORT_MMIO_PAGE    = 0xF1

; -----------------------------------------------------------------------------
; VTable конкретного драйвера
; -----------------------------------------------------------------------------

SECTION DRV_VTABLE
ALIGN 32

PUBLIC __palette_12b_vtable

__palette_12b_vtable:
    jp _palette_12b_detect
    jp _palette_12b_install
    jp _palette_12b_get_info
    jp _palette_12b_command
    jp _palette_12b_uninstall
    jp _palette_12b_set_color
    jp _palette_12b_set_border
    jp _palette_12b_load

; -----------------------------------------------------------------------------
; Данные драйвера
; -----------------------------------------------------------------------------

SECTION DRV_DATA

_palette_12b_info:
    dw @name
    db 1, 0        ; Версия 1@0
    dw 0x0002      ; Флаги: DRIVER_FLAG_STATIC
    db 0           ; Нет IRQ
    db 0           ; Резерв
@name:
    db "PALETTE 12-bit RGB", 0

; Текущие настройки
_current_mode:       db PAL_MODE_12BIT
_current_modifier:   db 0
_current_autoinc:    db 0
_current_border:     dw 0x0888  ; Серый по умолчанию
_state_buffer:
    ds 64

; -----------------------------------------------------------------------------
; Вспомогательные функции
; -----------------------------------------------------------------------------

_select_mmio_page:
    ld a, 2          ; Страница 2: регистры палитры
    out (PORT_MMIO_PAGE), a
    ret

_update_control_reg:
    push af
    push bc
    
    call _select_mmio_page
    
    ; Собираем управляющий регистр
    ld a, (_current_autoinc)
    rlca
    rlca
    rlca
    rlca
    rlca                  ; Бит 5: autoinc
    ld b, a
    
    ld a, (_current_modifier)
    and 0x01             ; 0=OR, 1=XOR
    rlca
    rlca
    rlca
    rlca
    rlca
    rlca                  ; Бит 6: modifier type
    or b
    
    ld b, a
    ld a, (_current_mode)
    and PAL_CTRL_MODE_MASK
    or b                  ; [mod_enable|mod_type|autoinc|mode]
    
    out (PAL_REG_CTRL), a
    
    pop bc
    pop af
    ret

; -----------------------------------------------------------------------------
; Основные методы драйвера
; -----------------------------------------------------------------------------

SECTION DRV_CODE

_palette_12b_detect:
    call _select_mmio_page
    
    ; Тест на наличие палитры
    ld a, 0xAA
    out (PAL_REG_INDEX), a
    in a, (PAL_REG_INDEX)
    cp 0xAA
    jr nz, @not_found
    
    ; Успех
    xor a
    ret
    
@not_found:
    ld a, 11        ; ERR_NO_DEVICE
    scf
    ret

_palette_12b_install:
    call _select_mmio_page
    
    ; Установка режима по умолчанию
    ld a, PAL_MODE_12BIT
    ld (_current_mode), a
    
    ; Выключение автоинкремента
    xor a
    ld (_current_autoinc), a
    ld (_current_modifier), a
    
    ; Обновление управляющего регистра
    call _update_control_reg
    
    ; Установка бордюра по умолчанию
    ld hl, (_current_border)
    ld c, l
    ld b, h
    call _palette_12b_set_border
    
    xor a
    ret

_palette_12b_get_info:
    ld hl, _palette_12b_info
    ret

_palette_12b_command:
    ; A = команда
    ; HL = параметр
    
    cp PAL_CMD_SET_MODE
    jr z, @cmd_set_mode
    cp PAL_CMD_SET_MODIFIER
    jr z, @cmd_set_modifier
    cp PAL_CMD_SET_AUTOINC
    jr z, @cmd_set_autoinc
    cp PAL_CMD_SAVE_STATE
    jr z, @cmd_save_state
    cp PAL_CMD_RESTORE_STATE
    jr z, @cmd_restore_state
    
    ; Неизвестная команда
    ld a, 1        ; ERR_NOT_SUPPORTED
    scf
    ret

@cmd_set_mode:
    ; L = режим
    ld a, l
    and 0x0F
    cp 5
    jr nc, @invalid_mode
    
    ld (_current_mode), a
    call _update_control_reg
    xor a
    ret

@cmd_set_modifier:
    ; L = тип модификатора
    ; H = значение
    ld a, l
    and 0x01
    ld (_current_modifier), a
    
    ld a, h
    call _select_mmio_page
    out (PAL_REG_MODIFIER), a
    
    call _update_control_reg
    xor a
    ret

@cmd_set_autoinc:
    ; L = 0/1
    ld a, l
    and 0x01
    ld (_current_autoinc), a
    call _update_control_reg
    xor a
    ret

@cmd_save_state:
    ; HL = буфер для сохранения
    push hl
    pop de
    
    ; Сохраняем текущие настройки
    ld hl, _current_mode
    ld bc, 64
    ldir
    
    xor a
    ret

@cmd_restore_state:
    ; HL = буфер с состоянием
    push hl
    pop de
    
    ; Восстанавливаем настройки
    ld hl, _state_buffer
    ld bc, 64
    ldir
    
    ; Применяем восстановленные настройки
    ld a, (_current_mode)
    call _select_mmio_page
    out (PAL_REG_CTRL), a
    
    ld a, (_current_modifier)
    call _select_mmio_page
    out (PAL_REG_MODIFIER), a
    
    xor a
    ret

@invalid_mode:
    ld a, 2        ; ERR_INVALID_PARAM
    scf
    ret

_palette_12b_uninstall:
    call _select_mmio_page
    
    ; Сброс управляющего регистра
    xor a
    out (PAL_REG_CTRL), a
    out (PAL_REG_MODIFIER), a
    
    ; Сброс бордюра
    ld bc, 0x0888  ; Серый
    call _palette_12b_set_border
    
    xor a
    ret

_palette_12b_set_color:
    ; A = индекс
    ; BC = цвет (12-bit: 0RRRGGGG BBBBBBBB)
    
    call _select_mmio_page
    
    ; ПРЯМАЯ ЗАПИСЬ БЕЗ ПРОВЕРОК
    ; Доверяем вызывающему коду
    ; Если индекс >255, он обрежется до 0-255 (A 8-bit)
    
    out (PAL_REG_INDEX), a
    ld a, c
    out (PAL_REG_DATA_LO), a
    ld a, b
    out (PAL_REG_DATA_HI), a
    
    xor a
    ret

_palette_12b_set_border:
    ; BC = цвет бордюра
    
    call _select_mmio_page
    
    ld a, c
    out (PAL_REG_BORDER_LO), a
    ld a, b
    out (PAL_REG_BORDER_HI), a
    
    ld (_current_border), bc
    
    xor a
    ret

_palette_12b_load:
    ; HL = указатель на массив цветов
    ; B = количество (0 = 256)
    ; C = начальный индекс
    
    call _select_mmio_page
    
    ; Установка начального индекса
    ld a, c
    out (PAL_REG_INDEX), a
    
    ; Если B = 0, значит 256 цветов
    ld a, b
    or a
    jr nz, @start_loop
    ld b, 0  ; 256 итераций

@start_loop:
    ld a, (_current_autoinc)
    or a
    jr nz, @loop_autoinc
    
    ; Без автоинкремента
@loop_manual:
    push bc
    ld c, (hl)
    inc hl
    ld b, (hl)
    inc hl
    
    ld a, c
    out (PAL_REG_DATA_LO), a
    ld a, b
    out (PAL_REG_DATA_HI), a
    
    ; Инкремент индекса вручную
    in a, (PAL_REG_INDEX)
    inc a
    out (PAL_REG_INDEX), a
    
    pop bc
    djnz @loop_manual
    jr @done

    ; С автоинкрементом
@loop_autoinc:
    ld c, (hl)
    inc hl
    ld b, (hl)
    inc hl
    
    ld a, c
    out (PAL_REG_DATA_LO), a
    ld a, b
    out (PAL_REG_DATA_HI), a
    
    djnz @loop_autoinc

@done:
    xor a
    ret

