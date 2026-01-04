; =============================================================================
; AUDIO@ASM - Polymorphic Audio Driver Structure
; For Aleste LX BIOS
; =============================================================================

module audio_driver

include "../../inc/bios/undefined_method.inc"

; -----------------------------------------------------------------------------
; ПОЛИМОРФНАЯ VTABLE АУДИО
; -----------------------------------------------------------------------------

SECTION DRV_VTABLE
ALIGN 32

; Экспортируем ВСЕ метки VTable с двойным подчеркиванием
PUBLIC __audio_vtable
PUBLIC __audio_detect, __audio_install, __audio_command, __audio_uninstall
PUBLIC __audio_play_note, __audio_stop, __audio_set_volume
PUBLIC __audio_set_noise_type, __audio_set_noise_custom, __audio_enable_noise
PUBLIC __audio_set_envelope, __audio_enable_envelope, __audio_enable_tone
PUBLIC __audio_read_register, __audio_write_register
PUBLIC __audio_play_chord, __audio_beep

__audio_vtable:
__audio_detect:           jp _undefined_method    ; 0
__audio_install:          jp _undefined_method    ; 3
__audio_command:          jp _undefined_method    ; 6
__audio_uninstall:        jp _undefined_method    ; 9
__audio_play_note:        jp _undefined_method    ; 12
__audio_stop:             jp _undefined_method    ; 15
__audio_set_volume:       jp _undefined_method    ; 18
__audio_set_noise_type:   jp _undefined_method    ; 21
__audio_set_noise_custom: jp _undefined_method    ; 24
__audio_enable_noise:     jp _undefined_method    ; 27
__audio_set_envelope:     jp _undefined_method    ; 30
__audio_enable_envelope:  jp _undefined_method    ; 33
__audio_enable_tone:      jp _undefined_method    ; 36
__audio_read_register:    jp _undefined_method    ; 39
__audio_write_register:   jp _undefined_method    ; 42
__audio_play_chord:       jp _undefined_method    ; 45
__audio_beep:             jp _undefined_method    ; 48
__audio_vtable_end:

; -----------------------------------------------------------------------------
; ЗАВИСИМОСТИ И ДАННЫЕ
; -----------------------------------------------------------------------------

SECTION DRV_DATA
DEPENDS_ON:
    db 0  ; Конец списка


_audio_info:
    dw @name
    db 1, 0        ; Версия 1.0
    dw 0x0002      ; Флаги: DRIVER_FLAG_STATIC
    db 0           ; Нет IRQ
    db 0           ; Резерв
@name:
    db "AY-3-8910 Audio Driver", 0


SECTION DRV_CODE

; -----------------------------------------------------------------------------
; Системные функции управления полиморфизмом
; -----------------------------------------------------------------------------

; Активировать конкретный драйвер палитры
; Вход: HL = адрес VTable конкретного драйвера
PUBLIC __audio_activate_driver
__audio_activate_driver:
    ld de, __audio_vtable
    ld (current_driver), de
    ld bc, __audio_vtable_end-__audio_vtable
    ldir
    ret

; Получить текущую активную VTable
; Выход: HL = адрес текущей VTable
PUBLIC __audio_get_current_vtable
__audio_get_current_vtable:
    ld hl, (current_driver)
    ret


SECTION DRV_DATA
current_driver: dw 0x0000