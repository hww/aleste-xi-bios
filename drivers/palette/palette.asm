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
PUBLIC __palette_vtable
PUBLIC __palette_detect, __palette_install, __palette_get_info
PUBLIC __palette_command, __palette_uninstall
PUBLIC __palette_set_color, __palette_set_border, __palette_load

__palette_vtable:
__palette_detect:    jp _undefined_method
__palette_install:   jp _undefined_method
__palette_get_info:  jp _undefined_method
__palette_command:   jp _undefined_method
__palette_uninstall: jp _undefined_method
__palette_set_color: jp _undefined_method
__palette_set_border:jp _undefined_method
__palette_load:      jp _undefined_method
__palette_vtable_end:

; -----------------------------------------------------------------------------
; Зависимости драйвера
; -----------------------------------------------------------------------------

SECTION DRV_DATA

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
PUBLIC __palette_activate_driver
__palette_activate_driver:
    ld de, __palette_vtable
    ld (current_driver), de
    ld bc, __palette_vtable_end-__palette_vtable
    ldir
    ret

; Получить текущую активную VTable
; Выход: HL = адрес текущей VTable
PUBLIC __palette_get_current_vtable
__palette_get_current_vtable:
    ld hl, (current_driver)
    ret


SECTION DRV_DATA
current_driver: dw 0x0000