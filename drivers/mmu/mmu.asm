; ==============================================================================
; MMU DRIVER (HAL LAYER) - Aleste LX
; ==============================================================================

include "../../inc/hardware/ports_aleste.inc"
include "../../inc/bios/drivers.inc"
include "../../inc/bios/errors.inc"

defc MMU_CMD_RESET      = 0x00
defc MMU_CMD_GET_TOTAL  = 0x10
defc MMU_CMD_GET_FREE   = 0x11
defc MMU_CMD_RESERVE    = 0x12
defc MMU_CMD_RELEASE    = 0x13

; ===================================================================
; ЗАВИСИМОСТИ: нет, это базовый драйвер железа
; ===================================================================
DEPENDS_ON:
    db 0

; ===================================================================
; ТАБЛИЦА ПЕРЕХОДОВ (15 байт + специфичные методы)
; ===================================================================
PUBLIC __mmu_vtable              ;
PUBLIC __mmu_detect, __mmu_install, __mmu_get_info, __mmu_command, __mmu_uninstall
PUBLIC __mmu_set_context, __mmu_get_context, __mmu_map_page, __mmu_lock, __mmu_unlock

SECTION DRV_VTABLE
align 32

__mmu_vtable:
__mmu_detect:      jp mmu_detect       ; +0  - проверка наличия
__mmu_install:     jp mmu_install      ; +3  - инициализация
__mmu_get_info:    jp mmu_get_info     ; +6  - информация
__mmu_command:     jp mmu_command      ; +9  - универсальные команды
__mmu_uninstall:   jp mmu_uninstall    ; +12 - выгрузка
; Специфичные методы MMU
__mmu_set_context: jp mmu_set_context  ; +15 - установка контекста
__mmu_get_context: jp mmu_get_context  ; +18 - чтение контекста
__mmu_map_page:    jp mmu_map_page     ; +21 - отображение страницы
__mmu_lock:        jp mmu_lock
__mmu_unlock:      jp mmu_unlock
__mmu_vtable_end:

; ===================================================================
; ДАННЫЕ ДРАЙВЕРА
; ===================================================================
SECTION DR__DATA

; Текущее состояние
mmu_current_hash:   db 0xFF        ; 0xFF = неинициализировано
mmu_lock_count:     db 0           ; Счётчик блокировок

; Дефолтный контекст (банки 0,1,2,3)
mmu_default_ctx:
    db 0x00          ; hash_id = 0 для линейной карты
    db 0, 1, 2, 3    ; страницы в окна FC,FD,FE,FF

; Статистика использования
mmu_stats:
    dw 0             ; вызовов set_context
    dw 0             ; реальных переключений
    dw 0             ; кэшированных вызовов

; Информация о драйвере
mmu_info:
    dw @name
    db 2, 0          ; Версия 2@0
    dw API_STATIC_BIT | API_BANKED_BIT
    db 0             ; IRQ не используем
    db 0             ; резерв
@name:
    db "ALESTE LX MMU v2.0", 0

; ===================================================================
; КОД ДРАЙВЕРА
; ===================================================================
SECTION DRV_CODE

; -------------------------------------------------------------------
; mmu_detect - проверка наличия MMU
; Out: A=1 если найден, A=0 если нет, Carry=1 при ошибке
; -------------------------------------------------------------------
mmu_detect:
    ; Сохраняем текущую страницу в окне C000-FFFF
    in a, (MMU_PORT_BANK_3)
    push af
    
    ; Пробуем записать уникальный паттерн
    ld a, 0xA5
    out (MMU_PORT_BANK_3), a
    
    ; Проверяем, что записалось
    in a, (MMU_PORT_BANK_3)
    cp 0xA5
    jr nz, @not_found
    
    ; Восстанавливаем оригинальную страницу
    pop af
    out (MMU_PORT_BANK_3), a
    
    ; Успех
    ld a, 1
    and a        ; сброс carry
    ret

@not_found:
    pop af       ; балансируем стек
    xor a        ; A=0, установлен ZF
    scf          ; carry=1 (ошибка)
    ret

; -------------------------------------------------------------------
; mmu_install - инициализация MMU
; -------------------------------------------------------------------
mmu_install:
    ; Устанавливаем дефолтную конфигурацию
    ld hl, mmu_default_ctx
    call mmu_set_context
    
    ; Сбрасываем статистику
    ld hl, 0
    ld (mmu_stats), hl
    ld (mmu_stats+2), hl
    ld (mmu_stats+4), hl
    
    xor a        ; успех
    ret

; -------------------------------------------------------------------
; mmu_get_info - возвращает информацию о драйвере
; Out: HL -> mmu_info
; -------------------------------------------------------------------
mmu_get_info:
    ld hl, mmu_info
    xor a        ; успех
    ret

; -------------------------------------------------------------------
; mmu_command - обработчик универсальных команд
; In: A = код команды (см. DRIVER_CMD_* и MMU_CMD_*)
; -------------------------------------------------------------------
mmu_command:
    cp MMU_CMD_RESET
    jp z, mmu_install
    
    cp MMU_CMD_GET_TOTAL
    jr z, @get_total
    
    cp MMU_CMD_GET_FREE
    jr z, @get_free
    
    ; Неизвестная команда
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

@get_total:
    ; Возвращаем общее количество страниц (256)
    ld hl, 256
    xor a        ; A=0, сброс carry
    ret

@get_free:
    ; TODO: реализовать, когда будет менеджер памяти
    ld hl, 0
    xor a
    ret

; -------------------------------------------------------------------
; mmu_uninstall - выгрузка драйвера
; -------------------------------------------------------------------
mmu_uninstall:
    ; Для MMU это просто сброс в дефолт
    call mmu_install
    xor a
    ret

; -------------------------------------------------------------------
; mmu_set_context - установка контекста памяти (ОСНОВНОЙ МЕТОД)
; In: HL -> [hash, page_FC, page_FD, page_FE, page_FF]
; -------------------------------------------------------------------
mmu_set_context:
    ; Обновляем статистику
    ld de, (mmu_stats)
    inc de
    ld (mmu_stats), de
    
    ; Проверяем кэш
    ld a, (hl)
    cp 0xFF
    jr z, @force_update  ; 0xFF = всегда обновлять
    
    ld b, a
    ld a, (mmu_current_hash)
    cp b
    jr nz, @do_update
    
    ; Контекст уже активен - увеличиваем счётчик кэшированных вызовов
    ld de, (mmu_stats+4)
    inc de
    ld (mmu_stats+4), de
    ret

@do_update:
    ; Сохраняем новый хэш
    ld a, b
    ld (mmu_current_hash), a
    
    ; Обновляем статистику реальных переключений
    ld de, (mmu_stats+2)
    inc de
    ld (mmu_stats+2), de

@force_update:
    ; Переключаем страницы (самая быстрая последовательность)
    inc hl
    ld a, (hl) \ out (MMU_PORT_BANK_0), a \ inc hl
    ld a, (hl) \ out (MMU_PORT_BANK_1), a \ inc hl
    ld a, (hl) \ out (MMU_PORT_BANK_2), a \ inc hl
    ld a, (hl) \ out (MMU_PORT_BANK_3), a
    ret

; -------------------------------------------------------------------
; mmu_get_context - чтение текущего контекста
; In: DE -> буфер для 5 байт
; -------------------------------------------------------------------
mmu_get_context:
    ; Хэш не читается из портов - используем кэшированный
    ld a, (mmu_current_hash)
    ld (de), a
    inc de
    
    ; Читаем текущие страницы из портов
    in a, (MMU_PORT_BANK_0) \ ld (de), a \ inc de
    in a, (MMU_PORT_BANK_1) \ ld (de), a \ inc de
    in a, (MMU_PORT_BANK_2) \ ld (de), a \ inc de
    in a, (MMU_PORT_BANK_3) \ ld (de), a
    
    xor a  ; успех
    ret

; -------------------------------------------------------------------
; mmu_map_page - отображение логической страницы в окно
; In: A = окно (0-3), B = физическая страница (0-255)
; -------------------------------------------------------------------
mmu_map_page:
    ; Проверяем номер окна
    cp 4
    jr nc, @invalid_window
    
    ; Вычисляем адрес порта: MMU_PORT_BANK_0 + окно
    add a, MMU_PORT_BANK_0
    ld c, a
    
    ; Обновляем кэш контекста
    ld hl, mmu_current_hash
    ld (hl), 0xFF  ; инвалидируем кэш
    
    ; Отображаем страницу
    out (c), b
    
    xor a  ; успех
    ret

@invalid_window:
    ld a, ERR_INVALID_PARAM
    scf
    ret

; -------------------------------------------------------------------
; mmu_lock/unlock - для критических секций
; -------------------------------------------------------------------
mmu_lock:
    di
    ld a, (mmu_lock_count)
    inc a
    ld (mmu_lock_count), a
    ret

mmu_unlock:
    ld a, (mmu_lock_count)
    dec a
    ld (mmu_lock_count), a
    jp nz, $+3  ; если не ноль, не включаем прерывания
    ei
    ret
    
