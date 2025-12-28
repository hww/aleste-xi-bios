; =============================================================================
; PALETTE@ASM - Polymorphic Palette Driver
; For Aleste LX BIOS
; =============================================================================

module palette_driver

include "../../inc/bios/undefined_method.inc"

; -----------------------------------------------------------------------------
; ВСЕ defc константы драйвера (только здесь!)
; -----------------------------------------------------------------------------

; -----------------------------------------------------------------------------
; Полиморфная VTable палитры
; -----------------------------------------------------------------------------

SECTION DRV_VTABLE
ALIGN 32

; Экспортируем ВСЕ метки VTable
PUBLIC _palette_vtable
PUBLIC _palette_detect, _palette_install, _palette_get_info
PUBLIC _palette_command, _palette_uninstall
PUBLIC _palette_set_color, _palette_set_border, _palette_load

_palette_vtable:
_palette_detect:    jp _undefined_method
_palette_install:   jp _undefined_method
_palette_get_info:  jp _undefined_method
_palette_command:   jp _undefined_method
_palette_uninstall: jp _undefined_method
_palette_set_color: jp _undefined_method
_palette_set_border:jp _undefined_method
_palette_load:      jp _undefined_method
_palette_vtable_end:

; -----------------------------------------------------------------------------
; Зависимости драйвера
; -----------------------------------------------------------------------------

SECTION DRV_DEPS

DEPENDS_ON:
    db 0  ; Конец списка (палитра самостоятельная)

; -----------------------------------------------------------------------------
; Данные драйвера
; -----------------------------------------------------------------------------

SECTION DRV_DATA

_palette_info:
    dw @name
    db 1, 0        ; Версия 1@0
    dw 0x0002      ; Флаги: DRIVER_FLAG_STATIC
    db 0           ; Вектор прерывания
    db 0           ; Резерв
@name:
    db "PALETTE Polymorphic Driver", 0

SECTION DRV_CODE

; -----------------------------------------------------------------------------
; Системные функции управления полиморфизмом
; -----------------------------------------------------------------------------

; Активировать конкретный драйвер палитры
; Вход: HL = адрес VTable конкретного драйвера
PUBLIC _palette_activate_driver
_palette_activate_driver:
    ld de, _palette_vtable
    ld (current_driver), de
    ld bc, _palette_vtable_end-_palette_vtable
    ldir
    ret

; Получить текущую активную VTable
; Выход: HL = адрес текущей VTable
PUBLIC _palette_get_current_vtable
_palette_get_current_vtable:
    ld hl, (current_driver)
    ret


SECTION DRV_DATA
current_driver: dw 0x0000