; ============================================================================
; MMU EXTENDED FUNCTIONS MODULE
; Для приложений и менеджера памяти
; ============================================================================
module mmu_extended

include "../../inc/hardware/ports_aleste.inc"
include "../../inc/bios/drivers.inc"

; ---- ДАННЫЕ ----
SECTION MMU_EXT_DATA

; Карта свободных страниц (256 бит = 32 байта)
mmu_page_bitmap:   ds 32

; Статистика аллокаций
mmu_alloc_count:   dw 0
mmu_free_count:    dw 256

; ---- КОНСТАНТЫ ----
MMU_PAGE_COUNT     equ 256
MMU_SYSTEM_PAGES   equ 4      ; Страницы 0-3 системные
MMU_BITMAP_SIZE    equ 32     ; 256 бит / 8 бит в байте

; ==================== ФУНКЦИИ МЕНЕДЖЕРА ПАМЯТИ ====================

SECTION MMU_EXT_CODE

; ----------------------------------------------------------------------------
; mmu_mem_init - Инициализация менеджера памяти
; ----------------------------------------------------------------------------
PUBLIC _mmu_mem_init
_mmu_mem_init:
    push hl
    push bc
    push de
    
    ; Очищаем битовую карту
    ld hl, mmu_page_bitmap
    ld de, mmu_page_bitmap + 1
    ld (hl), 0
    ld bc, MMU_BITMAP_SIZE - 1
    ldir
    
    ; Помечаем системные страницы как занятые
    ld hl, mmu_page_bitmap
    ld b, MMU_SYSTEM_PAGES
@mark_system:
    call set_page_occupied
    inc a
    djnz @mark_system
    
    ; Инициализируем счётчики
    ld hl, MMU_PAGE_COUNT - MMU_SYSTEM_PAGES
    ld (mmu_free_count), hl
    ld hl, 0
    ld (mmu_alloc_count), hl
    
    pop de
    pop bc
    pop hl
    ret

; ----------------------------------------------------------------------------
; mmu_alloc_pages - Выделить последовательные страницы
; In: A = количество страниц (1-256)
; Out: A = первая страница (или 0xFF при ошибке), Carry при ошибке
; ----------------------------------------------------------------------------
PUBLIC _mmu_alloc_pages
_mmu_alloc_pages:
    push bc
    push de
    push hl
    
    ld c, a  ; C = требуемое количество
    
    ; Проверяем, есть ли достаточно страниц
    ld hl, (mmu_free_count)
    ld a, c
    cp l
    jr c, @enough
    jr @error
    
@enough:
    ; Ищем последовательные свободные страницы
    ld a, MMU_SYSTEM_PAGES  ; Начинаем с первой несистемной
@search_loop:
    push af
    call check_pages_free
    pop af
    jr nc, @found
    
    ; Следующая страница
    inc a
    cp MMU_PAGE_COUNT
    jr c, @search_loop
    
@error:
    ld a, 0xFF
    scf
    jr @exit

@found:
    ; Помещаем страницы как занятые
    ld b, c
    ld d, a  ; Сохраняем первую страницу в D
@mark_loop:
    call set_page_occupied
    inc a
    djnz @mark_loop
    
    ; Обновляем счётчики
    ld hl, (mmu_free_count)
    ld a, c
    ld c, l
    sub c
    ld l, a
    ld (mmu_free_count), hl
    
    ld hl, (mmu_alloc_count)
    inc hl
    ld (mmu_alloc_count), hl
    
    ; Возвращаем первую страницу
    ld a, d
    and a  ; сброс carry
    
@exit:
    pop hl
    pop de
    pop bc
    ret

; ----------------------------------------------------------------------------
; mmu_free_pages - Освободить страницы
; In: A = первая страница, B = количество страниц
; ----------------------------------------------------------------------------
PUBLIC _mmu_free_pages
_mmu_free_pages:
    push af
    push bc
    push hl
    
    ld c, a  ; C = первая страница
    
@free_loop:
    ld a, c
    call set_page_free
    inc c
    djnz @free_loop
    
    ; Обновляем счётчики
    ld hl, (mmu_free_count)
    ld a, b
    add a, l
    ld l, a
    ld (mmu_free_count), hl
    
    ld hl, (mmu_alloc_count)
    dec hl
    ld (mmu_alloc_count), hl
    
    pop hl
    pop bc
    pop af
    ret

; ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

; Проверить, свободны ли N страниц начиная с A
; In: A = первая страница, C = количество
; Out: Carry = 0 если свободны, Carry = 1 если занята хотя бы одна
check_pages_free:
    push af
    push bc
    push hl
    
    ld b, a  ; B = текущая страница
    ld d, c  ; D = счётчик
    
@check_loop:
    ld a, b
    call is_page_free
    jr c, @occupied
    
    inc b
    dec d
    jr nz, @check_loop
    
    ; Все свободны
    and a  ; сброс carry
    jr @exit

@occupied:
    scf     ; установка carry
    
@exit:
    pop hl
    pop bc
    pop af
    ret

; Установить страницу A как занятую
set_page_occupied:
    push af
    push bc
    push hl
    
    call get_bit_position  ; HL = byte, B = bit mask
    ld a, (hl)
    or b
    ld (hl), a
    
    pop hl
    pop bc
    pop af
    ret

; Установить страницу A как свободную
set_page_free:
    push af
    push bc
    push hl
    
    call get_bit_position  ; HL = byte, B = bit mask
    ld a, b
    cpl
    ld b, a
    ld a, (hl)
    and b
    ld (hl), a
    
    pop hl
    pop bc
    pop af
    ret

; Проверить, свободна ли страница A
; Out: Carry = 0 если свободна, Carry = 1 если занята
is_page_free:
    push af
    push bc
    push hl
    
    call get_bit_position  ; HL = byte, B = bit mask
    ld a, (hl)
    and b
    jr z, @free
    
    ; Занята
    scf
    jr @exit

@free:
    and a  ; сброс carry
    
@exit:
    pop hl
    pop bc
    pop af
    ret

; Получить позицию бита для страницы A
; In: A = номер страницы
; Out: HL = адрес байта в битовой карте, B = маска бита
get_bit_position:
    push af
    
    ; Вычисляем байт: page / 8
    rrca
    rrca
    rrca
    and 0x1F  ; 0-31
    ld l, a
    ld h, high(mmu_page_bitmap)
    
    ; Вычисляем бит: page % 8
    pop af
    push af
    and 7
    ld b, 1
@shift_loop:
    or a
    jr z, @done
    sla b
    dec a
    jr @shift_loop
@done:
    pop af
    ret

; ----------------------------------------------------------------------------
; Статистика
; ----------------------------------------------------------------------------
PUBLIC _mmu_get_stats
_mmu_get_stats:
    ; HL -> количество свободных страниц
    ld hl, (mmu_free_count)
    ret

PUBLIC _mmu_get_alloc_count
_mmu_get_alloc_count:
    ; HL -> количество выделений
    ld hl, (mmu_alloc_count)
    ret
    
    
