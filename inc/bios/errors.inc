; ==============================================================================
; BIOS_ERRORS@INC - Коды ошибок BIOS Aleste (ASM версия)
; Должно совпадать с include/bios/errors@h!
;
; Соглашение:
; - Успех: A=0, Carry=0
; - Ошибка: A=код, Carry=1
; - Коды 0x00-0x7F для системы, 0x80-0xFF для приложений
; ==============================================================================

; ==============================================================================
; СИСТЕМНЫЕ ОШИБКИ (0x00-0x3F)
; ==============================================================================

; Успех (обязательно должен быть 0)
ERR_SUCCESS         equ 0x00

; Общие ошибки
ERR_NOT_SUPPORTED   equ 0x01  ; Операция не поддерживается
ERR_INVALID_PARAM   equ 0x02  ; Неверный параметр
ERR_OUT_OF_RANGE    equ 0x03  ; Выход за допустимые пределы
ERR_TIMEOUT         equ 0x04  ; Таймаут операции
ERR_BUSY            equ 0x05  ; Устройство занято
ERR_AGAIN           equ 0x06  ; Повторить операцию

; Ошибки памяти
ERR_OUT_OF_MEMORY   equ 0x10  ; Недостаточно памяти
ERR_INVALID_ADDRESS equ 0x11  ; Неверный адрес
ERR_MEMORY_PROTECT  equ 0x12  ; Нарушение защиты памяти
ERR_MMU_FAULT       equ 0x13  ; Ошибка MMU

; Ошибки устройств
ERR_NO_DEVICE       equ 0x20  ; Устройство не найдено
ERR_DEVICE_BUSY     equ 0x21  ; Устройство занято
ERR_DEVICE_FAULT    equ 0x22  ; Сбой устройства
ERR_DEVICE_OFFLINE  equ 0x23  ; Устройство отключено

; Ошибки ввода-вывода
ERR_IO_ERROR        equ 0x30  ; Ошибка ввода-вывода
ERR_IO_TIMEOUT      equ 0x31  ; Таймаут ввода-вывода
ERR_IO_PROTOCOL     equ 0x32  ; Ошибка протокола

; ==============================================================================
; ОШИБКИ ФАЙЛОВОЙ СИСТЕМЫ (0x40-0x5F)
; ==============================================================================

ERR_FILE_NOT_FOUND    equ 0x40  ; Файл не найден
ERR_PATH_NOT_FOUND    equ 0x41  ; Путь не найден
ERR_ACCESS_DENIED     equ 0x42  ; Доступ запрещен
ERR_DISK_FULL         equ 0x43  ; Диск переполнен
ERR_DISK_ERROR        equ 0x44  ; Ошибка диска
ERR_NOT_A_DIRECTORY   equ 0x45  ; Не директория
ERR_IS_DIRECTORY      equ 0x46  ; Это директория
ERR_FILE_EXISTS       equ 0x47  ; Файл уже существует

; ==============================================================================
; СЕТЕВЫЕ ОШИБКИ (0x60-0x7F)
; ==============================================================================

ERR_NET_DOWN          equ 0x60  ; Сеть недоступна
ERR_NET_TIMEOUT       equ 0x61  ; Таймаут сети
ERR_CONNECTION_LOST   equ 0x62  ; Соединение потеряно
ERR_CONNECTION_REFUSED equ 0x63 ; Соединение отклонено

; ==============================================================================
; ПРИКЛАДНЫЕ ОШИБКИ (0x80-0xFF) - для пользовательских программ
; ==============================================================================

; Первая свободная для приложений
ERR_APP_BASE          equ 0x80

; ==============================================================================
; МАКРОСЫ ДЛЯ АССЕМБЛЕРА
; ==============================================================================

; Проверка Carry flag на успех/ошибку
; Использование: SUCCESS? после вызова функции
; Пример: call some_func \ jr c, @error_handler

; Установить успех (A=0, C=0)
MACRO SET_SUCCESS
    xor a           ; A=0
    and a           ; Clear Carry
ENDM

; Установить ошибку
; Использование: SET_ERROR ERR_NO_DEVICE
MACRO SET_ERROR error_code
    ld a, error_code
    scf             ; Set Carry Flag
ENDM

; Быстрая проверка на конкретную ошибку
; Использование: CHECK_ERROR ERR_NO_DEVICE
; После: Z установлен если ошибка равна проверяемой
MACRO CHECK_ERROR error_code
    cp error_code
ENDM

; Возврат успеха
MACRO RETURN_SUCCESS
    xor a
    ret
ENDM

; Возврат ошибки
; Использование: RETURN_ERROR ERR_TIMEOUT
MACRO RETURN_ERROR error_code
    ld a, error_code
    scf
    ret
ENDM

; Обработка ошибки с переходом
; Использование: 
;   call some_func
;   ERROR_HANDLE @error_label
MACRO ERROR_HANDLE label
    jr c, label
ENDM

; Цепочка вызовов с проверкой ошибок
; Использование:
;   CHAIN_CALLS func1, func2, func3, @error_label
MACRO CHAIN_CALLS args:vararg
    LOCAL chain_error, chain_done
    
    ; Перебираем все аргументы
    IF _NARG > 0
        REPT _NARG
            ; Вызываем функцию
            call {1}
            jr c, chain_error
            ; Сдвигаем аргументы
            SHIFT
        ENDR
    ENDIF
    
    jr chain_done
    
chain_error:
    ; Здесь можно добавить дополнительную обработку
    ; Например: ld (last_error), a
    ret
    
chain_done:
ENDM

; ==============================================================================
; СТАНДАРТНЫЕ ОБРАБОТЧИКИ ОШИБОК
; ==============================================================================

; Успешное завершение (публичная метка)
EXTERN _err_success

; Ошибка "не поддерживается" (публичная метка)
EXTERN _err_not_supported

; Ошибка "устройство не найдено" (публичная метка)
EXTERN _err_no_device

; Ошибка "таймаут" (публичная метка)
EXTERN _err_timeout

; Универсальный обработчик ошибки
; Вход: A = код ошибки
; Выход: A = тот же код, Carry=1
EXTERN _err_set

; ==============================================================================
; КЛАССИФИКАЦИЯ ОШИБОК (дополнительные макросы)
; ==============================================================================

; Проверить, является ли ошибка ошибкой памяти (0x10-0x1F)
MACRO IS_MEMORY_ERROR error_code
    ld a, error_code
    cp ERR_OUT_OF_MEMORY
    jr c, @not_memory
    cp ERR_MMU_FAULT + 1
    jr c, @is_memory
@not_memory:
ENDM

; Проверить, является ли ошибка ошибкой устройства (0x20-0x2F)
MACRO IS_DEVICE_ERROR error_code
    ld a, error_code
    cp ERR_NO_DEVICE
    jr c, @not_device
    cp ERR_DEVICE_OFFLINE + 1
    jr c, @is_device
@not_device:
ENDM

; Проверить, является ли ошибка ошибкой ввода-вывода (0x30-0x3F)
MACRO IS_IO_ERROR error_code
    ld a, error_code
    cp ERR_IO_ERROR
    jr c, @not_io
    cp ERR_IO_PROTOCOL + 1
    jr c, @is_io
@not_io:
ENDM

; Проверить, является ли ошибка ошибкой файловой системы (0x40-0x5F)
MACRO IS_FS_ERROR error_code
    ld a, error_code
    cp ERR_FILE_NOT_FOUND
    jr c, @not_fs
    cp ERR_FILE_EXISTS + 1
    jr c, @is_fs
@not_fs:
ENDM

; ==============================================================================
; КОНВЕРТАЦИЯ ОШИБОК (для отладки)
; ==============================================================================

; Получить текстовое описание ошибки
; Вход: A = код ошибки
; Выход: HL = указатель на строку с описанием
MACRO GET_ERROR_STRING
    ld hl, error_strings
    add a, a        ; ×2 (каждая строка - 2 байта указатель)
    ld e, a
    ld d, 0
    add hl, de
    ld e, (hl)
    inc hl
    ld d, (hl)
    ex de, hl
ENDM

; Таблица строк ошибок (опционально, занимает память)
ifdef INCLUDE_ERROR_STRINGS
error_strings:
    dw @str_success, @str_not_supported, @str_invalid_param, @str_out_of_range
    dw @str_timeout, @str_busy, @str_again, 0, 0, 0, 0, 0, 0, 0
    dw @str_out_of_memory, @str_invalid_address, @str_memory_protect, @str_mmu_fault
    ; @@@ и так далее для всех ошибок

@str_success:         db "Success", 0
@str_not_supported:   db "Not supported", 0
@str_invalid_param:   db "Invalid parameter", 0
@str_out_of_range:    db "Out of range", 0
@str_timeout:         db "Timeout", 0
@str_busy:            db "Device busy", 0
@str_again:           db "Try again", 0
@str_out_of_memory:   db "Out of memory", 0
@str_invalid_address: db "Invalid address", 0
@str_memory_protect:  db "Memory protection fault", 0
@str_mmu_fault:       db "MMU fault", 0
endif

; ==============================================================================
; ОБРАБОТКА ОШИБОК В ПРЕРЫВАНИЯХ
; ==============================================================================

; Сохранить контекст и код ошибки
; Использование в обработчике прерывания:
;   PUSH_ERROR_CONTEXT ERR_DEVICE_FAULT
MACRO PUSH_ERROR_CONTEXT error_code
    push af
    push bc
    push de
    push hl
    ld a, error_code
    ld (error_save_area), a
ENDM

; Восстановить контекст и вернуть ошибку
; Использование:
;   POP_ERROR_CONTEXT
MACRO POP_ERROR_CONTEXT
    ld a, (error_save_area)
    pop hl
    pop de
    pop bc
    pop af
    scf
ENDM

ifdef ERROR_SAVE_AREA
error_save_area: db 0
endif