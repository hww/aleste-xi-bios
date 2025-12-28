; ==============================================================================
; CRTC 6845 DRIVER (ALESTE LX) - SINGLETON DRIVER
; ==============================================================================

include "../../inc/hardware/ports_aleste.inc"
include "../../inc/bios/drivers.inc"
include "../../inc/bios/errors.inc"


; Базовые команды для всех драйверов
defc CRTC_CMD_RESET    = 0x00
defc CRTC_CMD_STATUS   = 0x01
defc CRTC_CMD_SUSPEND  = 0x02
defc CRTC_CMD_RESUME   = 0x03

; Диапазоны команд по устройствам
defc CRTC_CMD_VIDEO_BASE      = 0x10
defc CRTC_CMD_AUDIO_BASE      = 0x20
defc CRTC_CMD_STORAGE_BASE    = 0x30
defc CRTC_CMD_MMU_BASE        = 0x60

; Флаги драйверов
defc CRTC_FLAG_IRQ_BIT     = 1 << 0
defc CRTC_FLAG_STATIC_BIT  = 1 << 1
defc CRTC_FLAG_DMA_BIT     = 1 << 2
defc CRTC_FLAG_BANKED_BIT  = 1 << 3
defc CRTC_FLAG_POWER_SAVE  = 1 << 4

; ===================================================================
; ЗАВИСИМОСТИ: MMU (для банков видеопамяти)
; ===================================================================
DEPENDS_ON:
    db "MMU", 0
    db 0

; ===================================================================
; ТАБЛИЦА ПЕРЕХОДОВ (15 байт + специфичные методы)
; ===================================================================
PUBLIC _crtc_vtable
PUBLIC _crtc_detect, _crtc_install, _crtc_get_info, _crtc_command
PUBLIC _crtc_uninstall, _crtc_set_mode, _crtc_set_view

SECTION DRV_VTABLE
align 32

_crtc_vtable:
_crtc_detect:      jp crtc_detect       ; +0  - проверка наличия
_crtc_install:     jp crtc_install      ; +3  - инициализация
_crtc_get_info:    jp crtc_get_info     ; +6  - информация
_crtc_command:     jp crtc_command      ; +9  - универсальные команды
_crtc_uninstall:   jp crtc_uninstall    ; +12 - выгрузка
; Специфичные методы CRTC
_crtc_set_mode:    jp crtc_set_mode     ; +15 - установка видеорежима
_crtc_set_view:    jp crtc_set_view     ; +18 - установка адреса отображения

; ===================================================================
; ДАННЫЕ ДРАЙВЕРА
; ===================================================================
SECTION DRV_DATA

; Текущее состояние
crtc_current_mode:  db 0xFF        ; 0xFF = неинициализировано
crtc_lock_count:    db 0           ; Счётчик блокировок

; Информация о драйвере
crtc_info:
    dw @name
    db 1, 0          ; Версия 1.0
    dw CRTC_FLAG_STATIC_BIT      ; Статическое устройство
    db 0             ; IRQ не используем
    db 0             ; резерв
@name:
    db "CRTC 6845 v1.0", 0

; ===================================================================
; КОД ДРАЙВЕРА
; ===================================================================
SECTION DRV_CODE

; -------------------------------------------------------------------
; crtc_detect - проверка наличия CRTC 6845
; Out: A=1 если найден, A=0 если нет, Carry=1 при ошибке
; -------------------------------------------------------------------
crtc_detect:
    push bc
    
    ; Пробуем записать/считать тестовое значение в регистр 12
    ld a, 0x0C          ; Register 12: Start Address High
    out (CRTC_PORT_ADDR), a
    ld a, 0x55          ; Test pattern
    out (CRTC_PORT_DATA), a
    
    ; Читаем обратно
    ld a, 0x0C
    out (CRTC_PORT_ADDR), a
    in a, (CRTC_PORT_DATA)
    cp 0x55
    jr nz, @not_found
    
    ; Успех
    ld a, 1
    and a        ; сброс carry
    pop bc
    ret

@not_found:
    xor a        ; A=0
    scf          ; carry=1 (ошибка)
    pop bc
    ret

; -------------------------------------------------------------------
; crtc_install - инициализация CRTC
; -------------------------------------------------------------------
crtc_install:
    ; Устанавливаем режим по умолчанию (0)
    ld a, 0
    call crtc_set_mode
    
    xor a        ; успех
    ret

; -------------------------------------------------------------------
; crtc_get_info - возвращает информацию о драйвере
; Out: HL -> crtc_info
; -------------------------------------------------------------------
crtc_get_info:
    ld hl, crtc_info
    xor a        ; успех
    ret

; -------------------------------------------------------------------
; crtc_command - обработчик универсальных команд
; In: A = код команды
;     BC/HL = дополнительные параметры (зависит от команды)
; -------------------------------------------------------------------
crtc_command:
    cp CRTC_CMD_RESET
    jp z, crtc_install
    
    cp CMD_VIDEO_BASE + 0    ; 0x10 = WRITE_REG
    jr z, @write_reg
    
    cp CMD_VIDEO_BASE + 1    ; 0x11 = READ_REG
    jr z, @read_reg
    
    cp CMD_VIDEO_BASE + 2    ; 0x12 = WAIT_VSYNC
    jr z, @wait_vsync
    
    ; Неизвестная команда
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

@write_reg:
    ; B = регистр, C = значение
    ld a, b
    out (CRTC_PORT_ADDR), a
    ld a, c
    out (CRTC_PORT_DATA), a
    xor a
    ret

@read_reg:
    ; B = регистр, возвращает A = значение
    ld a, b
    out (CRTC_PORT_ADDR), a
    in a, (CRTC_PORT_DATA)
    xor a        ; сброс carry
    ret

@wait_vsync:
    ; Ждём вертикальную синхронизацию
;    ld a, CRTC_REG_STATUS
;    out (CRTC_PORT_ADDR), a
@vsync_loop:
;    in a, (CRTC_PORT_DATA)
;    bit 3, a               ; Бит 3 = VSync в некоторых CRTC
;    jr z, @vsync_loop
    xor a
    ret

; -------------------------------------------------------------------
; crtc_uninstall - выгрузка драйвера
; -------------------------------------------------------------------
crtc_uninstall:
    ; Сбрасываем CRTC в безопасное состояние
    ld hl, crtc_safe_regs
    ld b, 16               ; 16 регистров CRTC
    ld c, CRTC_PORT_ADDR
@reset_loop:
    ld a, (hl)
    out (c), a
    inc hl
    inc c
    ld a, (hl)
    out (c), a
    dec c
    inc hl
    djnz @reset_loop
    
    xor a
    ret

; -------------------------------------------------------------------
; crtc_set_mode - установка видеорежима (специфичный метод)
; In: A = код режима (0-10)
; -------------------------------------------------------------------
crtc_set_mode:
    ; Сохраняем режим
    ld (crtc_current_mode), a
    
    ; Находим таблицу регистров для этого режима
    call get_mode_table
    
    ; Записываем регистры CRTC
    ld b, (hl)          ; B = количество регистров
    inc hl
    ld c, CRTC_PORT_ADDR
    
@write_loop:
    ld a, (hl)          ; Номер регистра
    out (c), a
    inc hl
    inc c
    ld a, (hl)          ; Значение
    out (c), a
    dec c
    inc hl
    djnz @write_loop
    
    xor a
    ret

; -------------------------------------------------------------------
; crtc_set_view - установка адреса отображения (специфичный метод)
; In: H = слот (0-3), L = страница/смещение
; -------------------------------------------------------------------
crtc_set_view:
    ; Устанавливаем Start Address High (регистр 12)
    ld a, CRTC_REG_START_H
    out (CRTC_PORT_ADDR), a
    ld a, h
    out (CRTC_PORT_DATA), a
    
    ; Start Address Low (регистр 13)
    ld a, CRTC_REG_START_L
    out (CRTC_PORT_ADDR), a
    ld a, l
    out (CRTC_PORT_DATA), a
    
    xor a
    ret

; ===================================================================
; ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
; ===================================================================

; Получить таблицу регистров для режима A
get_mode_table:
    ld l, a
    ld h, 0
    add hl, hl          ; index * 2
    ld de, crtc_mode_tables
    add hl, de
    ld e, (hl)
    inc hl
    ld d, (hl)
    ex de, hl
    ret

; ===================================================================
; ДАННЫЕ РЕЖИМОВ
; ===================================================================

crtc_safe_regs:
    ; Безопасные значения регистров
    db 0x00, 0x00   ; R0: Horizontal Total
    db 0x01, 0x00   ; R1: Horizontal Displayed
    db 0x02, 0x00   ; R2: Horizontal Sync Position
    db 0x03, 0x00   ; R3: Sync Width
    db 0x04, 0x00   ; R4: Vertical Total
    db 0x05, 0x00   ; R5: Vertical Total Adjust
    db 0x06, 0x00   ; R6: Vertical Displayed
    db 0x07, 0x00   ; R7: Vertical Sync Position
    db 0x08, 0x00   ; R8: Interlace & Skew
    db 0x09, 0x00   ; R9: Maximum Raster Address
    db 0x0A, 0x00   ; R10: Cursor Start
    db 0x0B, 0x00   ; R11: Cursor End
    db 0x0C, 0x00   ; R12: Start Address High
    db 0x0D, 0x00   ; R13: Start Address Low
    db 0x0E, 0x00   ; R14: Cursor Address High
    db 0x0F, 0x00   ; R15: Cursor Address Low

; Подключаем файлы с таблицами режимов
include "crtc_modes.asm"

