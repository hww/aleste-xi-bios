; =============================================================================
; PALETTE.INC - Color Palette Driver Interface (Assembly)
; For Aleste LX BIOS
; =============================================================================
; ТОЛЬКО extern объявления и макросы!
; НИКАКИХ equ, =, defc!

; -----------------------------------------------------------------------------
; 1. ТОЛЬКО extern объявления методов полиморфной VTable
; -----------------------------------------------------------------------------

; Обязательные методы драйвера
extern _palette_detect
extern _palette_install
extern _palette_get_info
extern _palette_command
extern _palette_uninstall

; Специфичные методы палитры
extern _palette_set_color
extern _palette_set_border
extern _palette_load

; -----------------------------------------------------------------------------
; 2. ТОЛЬКО макросы для удобства (KISS)
; -----------------------------------------------------------------------------

; Вызовы методов палитры
macro PAL_CALL_DETECT
    call _palette_detect
endm

macro PAL_CALL_INSTALL
    call _palette_install
endm

macro PAL_CALL_GET_INFO
    call _palette_get_info
endm

macro PAL_CALL_COMMAND
    call _palette_command
endm

macro PAL_CALL_UNINSTALL
    call _palette_uninstall
endm

macro PAL_CALL_SET_COLOR
    call _palette_set_color
endm

macro PAL_CALL_SET_BORDER
    call _palette_set_border
endm

macro PAL_CALL_LOAD
    call _palette_load
endm

; -----------------------------------------------------------------------------
; 3. Макросы для работы с конкретными драйверами
; -----------------------------------------------------------------------------

; Активация драйверов
macro PAL_ACTIVATE_12BIT
    ld hl, _palette_12b_vtable
    call _palette_activate_driver
endm

macro PAL_ACTIVATE_CPC
    ld hl, _palette_cpc_vtable
    call _palette_activate_driver
endm

macro PAL_ACTIVATE_MSX
    ld hl, _palette_msx_vtable
    call _palette_activate_driver
endm