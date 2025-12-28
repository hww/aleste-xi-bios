; ==============================================================================
; MMU MANAGER DRIVER - System Memory Allocator (Aleste LX)
; ==============================================================================

module mmu_mgr

include "../../inc/hardware/ports_aleste.inc"
include "../../inc/bios/drivers.inc"
include "../../inc/bios/errors.inc"

; ---- КОНСТАНТЫ ----
defc PAGE_COUNT     = 256
defc MAX_PAGE_IDX = PAGE_COUNT - 1
defc SYSTEM_PAGES   = 4      ; Страницы 0-3 зарезервированы ПЗУ/Системой
defc BITMAP_SIZE    = 32     ; 256 бит / 8

; ===================================================================
; ТАБЛИЦА ПЕРЕХОДОВ (VTable)
; ===================================================================
PUBLIC __mmu_mgr_vtable
PUBLIC __mmu_mgr_detect, __mmu_mgr_install, __mmu_mgr_get_info
PUBLIC __mmu_mgr_command, __mmu_mgr_uninstall
PUBLIC __mmu_mgr_alloc, __mmu_mgr_free, __mmu_mgr_stats

SECTION DRV_VTABLE
align 32

__mmu_mgr_vtable:
__mmu_mgr_detect:    jp detect        ; +0  (Обязательный)
__mmu_mgr_install:   jp install       ; +3  (Обязательный)
__mmu_mgr_get_info:  jp get_info      ; +6  (Обязательный)
__mmu_mgr_command:   jp command       ; +9  (Обязательный)
__mmu_mgr_uninstall: jp uninstall     ; +12 (Обязательный)
; Специфичные методы менеджера
__mmu_mgr_alloc:     jp alloc_pages   ; +15
__mmu_mgr_free:      jp free_pages    ; +18
__mmu_mgr_stats:     jp get_stats     ; +21

; ===================================================================
; ДАННЫЕ ДРАЙВЕРА
; ===================================================================
SECTION DRV_DATA

page_bitmap:   ds BITMAP_SIZE
free_count:    dw 0
alloc_count:   dw 0

info_block:
    dw @name
    db 1, 0          ; Версия 1.0
    dw API_STATIC_BIT
    db 0, 0          ; IRQ, резерв
@name:
    db "MMU Manager (Bitmap)", 0

; ===================================================================
; КОД ДРАЙВЕРА
; ===================================================================
SECTION DRV_CODE

; --- Обязательные методы ---

detect:
    ld a, 1          ; Менеджер памяти всегда присутствует (софтовый драйвер)
    and a            ; Clear carry
    ret

install:
    push hl
    push bc
    push de
    
    ; Очистка битмапа
    ld hl, page_bitmap
    ld de, page_bitmap + 1
    ld (hl), 0
    ld bc, BITMAP_SIZE - 1
    ldir
    
    ; Резервируем системные страницы
    ld b, SYSTEM_PAGES
    xor a            ; Начинаем с 0
@sys_loop:
    call set_occupied
    inc a
    djnz @sys_loop
    
    ; Сброс счетчиков
    ld hl, PAGE_COUNT - SYSTEM_PAGES
    ld (free_count), hl
    ld hl, 0
    ld (alloc_count), hl
    
    pop de
    pop bc
    pop hl
    xor a            ; Success
    ret

get_info:
    ld hl, info_block
    xor a
    ret

command:
    ; Здесь можно реализовать проверку целостности или дефрагментацию
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

uninstall:
    ; Менеджер памяти обычно не выгружается, но для канона сбрасываем всё
    jp install

; --- Специфичные методы ---

; In: A = кол-во страниц
; Out: A = первая страница, Carry = ошибка
; In: A = кол-во страниц
; Out: A = первая страница, Carry = ошибка (A=0xFF)
alloc_pages:
    push bc
    push de
    push hl
    
    ld c, a          ; С = сколько нужно
    ld hl, (free_count)
    cp l
    jr nc, @error    ; Если просим больше, чем есть всего
    
    ld a, SYSTEM_PAGES
@find_loop:
    push af
    ; Проверка: не выйдет ли блок за пределы MAX_PAGE_IDX?
    add a, c
    dec a            ; A = последняя страница потенциального блока
    cp MAX_PAGE_IDX
    jr nc, @pop_error ; Если последняя страница >= 255 (или перебор), выходим
    
    pop af           ; Восстанавливаем A (начало блока) для проверки
    push af
    call check_free  ; Проверяем C страниц начиная с A
    pop af
    jr nc, @found    ; Если Carry=0, значит блок свободен
    
    inc a            ; Пробуем со следующей страницы
    ; Здесь тоже важна проверка, чтобы сам указатель 'a' не стал выше лимита
    cp MAX_PAGE_IDX
    jr c, @find_loop

@error:
    ld a, 0xFF
    scf              ; Устанавливаем Carry (ошибка)
    pop hl
    pop de
    pop bc
    ret

@pop_error:
    pop af           ; Снимаем AF со стека перед выходом
    jr @error

@found:
    ld d, a          ; D = стартовая страница найденного блока
    ld b, c          ; B = счетчик для цикла пометки
@mark_loop:
    call set_occupied
    inc a
    djnz @mark_loop
    
    ; Обновление статистики (free = free - C)
    ld hl, (free_count)
    ld a, l
    sub c
    ld l, a
    ld (free_count), hl
    
    ; Инкремент общего счетчика аллокаций
    ld hl, (alloc_count)
    inc hl
    ld (alloc_count), hl
    
    ld a, d          ; Возвращаем номер первой страницы
    or a             ; Сброс Carry (успех), если A не 0. Если A=0, Carry и так 0.
    pop hl
    pop de
    pop bc
    ret

; In: A = старт, B = кол-во
free_pages:
    push af
    push bc
    push hl
    ld c, a
@f_loop:
    ld a, c
    call set_free
    inc c
    djnz @f_loop
    
    ; Статистика
    ld hl, (free_count)
    ld a, b
    add a, l
    ld l, a
    ld (free_count), hl
    
    ld hl, (alloc_count)
    dec hl
    ld (alloc_count), hl
    
    pop hl
    pop bc
    pop af
    ret

get_stats:
    ld hl, (free_count)
    xor a
    ret

; ==================== ВНУТРЕННИЕ ХЕЛПЕРЫ (Без _) ====================

; Проверка C страниц начиная с A
check_free:
    push af
    push bc
    ld b, a
    ld d, c
@c_loop:
    ld a, b
    call is_free
    jr c, @occ       ; Если занята
    inc b
    dec d
    jr nz, @c_loop
    and a            ; Свободны
    pop bc
    pop af
    ret
@occ:
    scf              ; Заняты
    pop bc
    pop af
    ret

set_occupied:
    push hl 
    push bc 
    push af
    call get_ptr
    ld a, (hl)
    or b
    ld (hl), a
    pop af 
    pop bc 
    pop hl
    ret

set_free:
    push hl
    push bc
    push af
    call get_ptr
    ld a, b
    cpl
    ld b, a
    ld a, (hl)
    and b
    ld (hl), a
    pop af
    pop bc
    pop hl
    ret

is_free:
    push hl
    push bc
    call get_ptr
    ld a, (hl)
    and b
    jr z, @yes
    scf              ; Carry = 1 (занято)
    jr @end
@yes:
    and a            ; Carry = 0 (свободно)
@end:
    pop bc
    pop hl
    ret

get_ptr:
    push af
    rrca
    rrca 
    rrca
    and 0x1F
    ld l, a
    ld h, page_bitmap >> 8
    
    pop af
    push af
    and 7           ; A = номер бита (0-7)
    ld b, 1         ; Начальная маска
    jr z, @done     ; Если бит 0, сдвиги не нужны
    
@shift_loop:        ; Локальная метка в стиле z88dk
    sla b 
    dec a 
    jr nz, @shift_loop

@done:
    pop af
    ret