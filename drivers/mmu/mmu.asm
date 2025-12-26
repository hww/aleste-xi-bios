; ==============================================================================
; MMU DRIVER (HAL LAYER) - Aleste LX
; ==============================================================================

include "../../inc/hardware/ports_aleste.inc"
include "../../inc/bios/drivers.inc"
include "../../inc/bios/errors.inc"

; ===================================================================
; ЗАВИСИМОСТИ: нет, это базовый драйвер железа
; ===================================================================
DEPENDS_ON:
    db 0

; ===================================================================
; ДИАПАЗОНЫ КОМАНД ДЛЯ MMU
; ===================================================================
CMD_MMU_GET_TOTAL   equ 0x10
CMD_MMU_GET_FREE    equ 0x11
CMD_MMU_RESERVE     equ 0x12
CMD_MMU_RELEASE     equ 0x13
CMD_MMU_GET_STATS   equ 0x14
CMD_MMU_SET_MODE    equ 0x15  ; LX/EX режимы

; ===================================================================
; ТАБЛИЦА ПЕРЕХОДОВ (15 байт + специфичные методы)
; ===================================================================
PUBLIC _mmu_vtable
PUBLIC _mmu_detect, _mmu_install, _mmu_get_info, _mmu_get_info, _mmu_command, _mmu_command, _mmu_uninstall
PUBLIC _mmu_set_context, _mmu_get_context, _mmu_map_page

SECTION DRV_VTABLE
align 32

_mmu_vtable:
_mmu_detect:      jp mmu_detect       ; +0  - проверка наличия
_mmu_install:     jp mmu_install      ; +3  - инициализация
_mmu_get_info:    jp mmu_get_info     ; +6  - информация
_mmu_command:     jp mmu_command      ; +9  - универсальные команды
_mmu_uninstall:   jp mmu_uninstall    ; +12 - выгрузка
; Специфичные методы MMU
_mmu_set_context: jp mmu_set_context  ; +15 - установка контекста
_mmu_get_context: jp mmu_get_context  ; +18 - чтение контекста
_mmu_map_page:    jp mmu_map_page     ; +21 - отображение страницы

; ===================================================================
; ДАННЫЕ ДРАЙВЕРА
; ===================================================================
SECTION DRV_DATA

; Текущее состояние
mmu_current_hash:   db 0xFF        ; 0xFF = неинициализировано
mmu_total_pages:    db 256         ; Всего страниц в системе
mmu_free_pages:     dw 0           ; Свободных страниц
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
    dw API_STATIC_DEVICE | API_USES_DMA
    db 0             ; IRQ не используем
    db 0             ; резерв
@name:
    db "ALESTE LX MMU v2@0", 0

; ===================================================================
; КОД ДРАЙВЕРА
; ===================================================================
SECTION DRV_CODE

; -------------------------------------------------------------------
; mmu_detect - проверка наличия MMU
; Out: A=1 если найден, A=0 если нет
; -------------------------------------------------------------------
mmu_detect:
    ; Сохраняем текущую страницу в окне C000-FFFF
    in a, (0xFF)
    push af
    
    ; Пробуем записать уникальный паттерн
    ld a, 0xA5
    out (0xFF), a
    
    ; Проверяем, что записалось
    in a, (0xFF)
    cp 0xA5
    jr nz, @not_found
    
    ; Восстанавливаем оригинальную страницу
    pop af
    out (0xFF), a
    
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
    
    ; Инициализируем менеджер памяти
    call mmu_init_memory
    
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
; In: A = код команды (см@ CMD_MMU_*)
; -------------------------------------------------------------------
mmu_command:
    cp CMD_RESET
    jp z, mmu_install
    
    cp CMD_MMU_GET_TOTAL
    jr z, @get_total
    
    cp CMD_MMU_GET_FREE
    jr z, @get_free
    
    cp CMD_MMU_RESERVE
    jr z, @reserve_pages
    
    cp CMD_MMU_RELEASE
    jr z, @release_pages
    
    cp CMD_MMU_GET_STATS
    jr z, @get_stats
    
    ; Неизвестная команда
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

@get_total:
    ; Возвращаем общее количество страниц
    ld a, (mmu_total_pages)
    xor a        ; A != 0, сброс carry
    ret

@get_free:
    ; Возвращаем количество свободных страниц
    ld hl, (mmu_free_pages)
    xor a        ; сброс carry
    ret

@get_stats:
    ; HL -> статистика
    ld hl, mmu_stats
    xor a
    ret

@reserve_pages:
    ; A = количество страниц для резервации
    ; Возвращает номер первой страницы в HL или ошибку
    ; TODO: реализовать аллокатор
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

@release_pages:
    ; HL = номер первой страницы, BC = количество
    ; TODO: реализовать аллокатор
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
    ld a, (hl) \ out (0xFC), a \ inc hl
    ld a, (hl) \ out (0xFD), a \ inc hl
    ld a, (hl) \ out (0xFE), a \ inc hl
    ld a, (hl) \ out (0xFF), a
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
    in a, (0xFC) \ ld (de), a \ inc de
    in a, (0xFD) \ ld (de), a \ inc de
    in a, (0xFE) \ ld (de), a \ inc de
    in a, (0xFF) \ ld (de), a
    
    xor a  ; успех
    ret

; -------------------------------------------------------------------
; mmu_map_page - отображение логической страницы в окно
; In: A = окно (0-3), B = физическая страница
; -------------------------------------------------------------------
mmu_map_page:
    ; Проверяем валидность параметров
    cp 4
    jr nc, @invalid_window
    
    ld c, a
    ld a, b
    cp (mmu_total_pages)
    jr nc, @invalid_page
    
    ; Вычисляем адрес порта: 0xFC + окно
    ld a, c
    add a, 0xFC
    ld c, a
    
    ; Обновляем кэш контекста
    ld hl, mmu_current_hash
    ld (hl), 0xFF  ; инвалидируем кэш
    
    ; Отображаем страницу
    out (c), b
    
    xor a  ; успех
    ret

@invalid_window:
@invalid_page:
    ld a, ERR_INVALID_PARAM
    scf
    ret

; -------------------------------------------------------------------
; mmu_init_memory - инициализация менеджера памяти
; -------------------------------------------------------------------
mmu_init_memory:
    ; Помечаем первые 4 страницы как занятые (система)
    ; TODO: полная реализация битовой карты свободных страниц
    ld hl, 256 - 4
    ld (mmu_free_pages), hl
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