
; ==============================================================================
; СТАНДАРТНЫЕ ОБРАБОТЧИКИ ОШИБОК
; ==============================================================================
include "../inc/bios/errors.inc"

; Успешное завершение (публичная метка)
PUBLIC _err_success
_err_success:
    xor a
    and a
    ret

; Ошибка "не поддерживается" (публичная метка)
PUBLIC _err_not_supported
_err_not_supported:
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

; Ошибка "устройство не найдено" (публичная метка)
PUBLIC _err_no_device
_err_no_device:
    ld a, ERR_NO_DEVICE
    scf
    ret

; Ошибка "таймаут" (публичная метка)
PUBLIC _err_timeout
_err_timeout:
    ld a, ERR_TIMEOUT
    scf
    ret

; Универсальный обработчик ошибки
; Вход: A = код ошибки
; Выход: A = тот же код, Carry=1
PUBLIC _err_set
_err_set:
    scf
    ret
