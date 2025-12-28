;; ============================================================
;; vbuf_16k1b@asm - Драйвер видеобуфера 640x200 1bpp
;; ============================================================

module vbuf_16k1b

include "../../inc/drivers/vbuf.inc"
include "../../inc/bios/errors.inc"

; ---- Константы драйвера ----
VRAM_BASE      equ 0x8000     ; Базовый адрес VRAM
MMU_SLOT2_CTL  equ 0x7F       ; Порт управления MMU слота 2
VRAM_PAGE      equ 0x10       ; Номер страницы VRAM
SCREEN_WIDTH   equ 640
SCREEN_HEIGHT  equ 200
BYTES_PER_LINE equ 80         ; 640/8
VRAM_SIZE      equ 16000      ; 80 * 200


; ---- Секция: Таблицы конкретных драйверов ----
section vbuf_driver_tables

; Таблица драйвера 16K 1bpp (используется в vbuf_16k1b.asm)
PUBLIC _drv_16k1b_vtable
_drv_16k1b_vtable:
    jp _drv_16k1b_init
    jp _drv_16k1b_get_mode_info
    jp _drv_16k1b_command
    jp _drv_16k1b_clear
    jp _drv_16k1b_clear_rect
    jp _drv_16k1b_scroll_up
    jp _drv_16k1b_scroll_down
    jp _drv_16k1b_set_clip_rect
    jp _drv_16k1b_get_clip_rect
    jp _drv_16k1b_copy_rect
    
    ; Примитивы рисования
    jp _drv_16k1b_get_addr
    jp _drv_16k1b_put_pixel_fast
    jp _drv_16k1b_put_pixel
    jp _drv_16k1b_get_pixel
    jp _drv_16k1b_draw_hline
    jp _drv_16k1b_draw_vline
    jp _drv_16k1b_draw_line
    jp _drv_16k1b_draw_rect
    jp _drv_16k1b_fill_rect
    
    ; Текст и битмапы
    jp _drv_16k1b_draw_char_8x8
    jp _drv_16k1b_draw_bitmap
    
    ; Двойная буферизация
    jp _drv_16k1b_swap_buffers
    jp _drv_16k1b_get_backbuf
    jp _drv_16k1b_clear_backbuf

; ---- Секция: Данные драйвера ----
section vbuf_16k1b_data

; Информация о режиме
_drv_16k1b_mode_info:
    dw SCREEN_WIDTH      ; @width
    dw SCREEN_HEIGHT     ; @height
    db 1                 ; @bpp (1 бит на пиксель)
    db BYTES_PER_LINE    ; @bytes_per_line
    dw VRAM_SIZE         ; @vram_size
    db 0                 ; @flags (по умолчанию)

; Область отсечения (по умолчанию - весь экран)
_drv_16k1b_clip_rect:
    dw 0                 ; @x
    dw 0                 ; @y
    dw SCREEN_WIDTH      ; @w
    dw SCREEN_HEIGHT     ; @h

; ---- Секция: Код драйвера ----
section vbuf_16k1b_code

; --- Вспомогательная: Установить банк VRAM ---
; Сохраняет: AF, BC
_drv_16k1b_set_vram_bank:
    ld a, VRAM_PAGE
    out (MMU_SLOT2_CTL), a
    ret

; --- Системные методы ---

; Инициализация драйвера
_drv_16k1b_init:
    call _drv_16k1b_set_vram_bank
    xor a
    ret

; Получить информацию о режиме
_drv_16k1b_get_mode_info:
    ld hl, _drv_16k1b_mode_info
    ret

; Команда драйвера
_drv_16k1b_command:
    cp VBUF_CMD_RESET
    jr z, @reset
    cp VBUF_CMD_SET_BANK
    jr z, @set_bank
    
    ; Неподдерживаемая команда
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

@reset:
    call _drv_16k1b_init
    xor a
    ret

@set_bank:
    ; param = номер банка
    ld a, l
    out (MMU_SLOT2_CTL), a
    xor a
    ret

; --- Управление буфером ---

; Очистить весь буфер
; Вход: A = паттерн заполнения
_drv_16k1b_clear:
    call _drv_16k1b_set_vram_bank
    ld hl, VRAM_BASE
    ld (hl), a
    ld d, h
    ld e, l
    inc de
    ld bc, VRAM_SIZE - 1
    ldir
    ret

; Очистить прямоугольник
; Вход: HL = указатель на vbuf_rect_t
;       A = цвет
_drv_16k1b_clear_rect:
    call _drv_16k1b_set_vram_bank
    ; Реализация заливки прямоугольника
    ; @@@ (пропускаем для краткости) @@@
    ret

; Прокрутка вверх
; Вход: A = количество строк
_drv_16k1b_scroll_up:
    call _drv_16k1b_set_vram_bank
    ld b, a
    ld a, BYTES_PER_LINE
    call @multiply_ab  ; DE = A * B (смещение в байтах)
    
    ld hl, VRAM_BASE
    add hl, de         ; HL = начало области копирования
    ld de, VRAM_BASE   ; DE = начало назначения
    ld bc, VRAM_SIZE
    sbc hl, de         ; BC = размер для копирования
    ldir
    
    ; Очистка освободившейся области
    ex de, hl          ; HL = начало области очистки
    ld (hl), 0
    push hl
    pop de
    inc de
    ld bc, VRAM_SIZE
    sbc hl, de
    ex de, hl
    ldir
    
    ret

@multiply_ab:
    ; A * B -> DE
    ld d, 0
    ld e, b
    ld b, a
    xor a
    ld c, a
@mult_loop:
    add hl, hl
    rla
    jr nc, @mult_no_add
    add hl, de
    adc a, c
@mult_no_add:
    djnz @mult_loop
    ld d, a
    ld e, h
    ret

; --- Примитивы рисования ---

; Получить адрес VRAM
; Вход: BC = X, DE = Y
; Выход: HL = адрес, A = маска бита
_drv_16k1b_get_addr:
    ; HL = Y * 80
    ld l, e
    ld h, d
    add hl, hl  ; *2
    add hl, hl  ; *4
    add hl, hl  ; *8
    push hl
    add hl, hl  ; *16
    add hl, hl  ; *32
    pop de
    add hl, de  ; *40
    add hl, hl  ; *80
    
    ; Добавить X / 8
    ld a, c
    srl b \ rra  ; BC/2
    srl b \ rra  ; BC/4
    srl b \ rra  ; BC/8
    ld e, a
    ld d, 0
    add hl, de
    
    ; Битовая маска
    ld a, c
    and 7
    ld b, a
    ld a, 0x80
    jr z, @mask_done
@mask_shift:
    rrca
    djnz @mask_shift
@mask_done:
    
    ; Добавить базовый адрес
    ld de, VRAM_BASE
    add hl, de
    ret

; Нарисовать пиксель (быстро, без проверок)
; Вход: BC = X, DE = Y, A = цвет (0/1)
_drv_16k1b_put_pixel_fast:
    push af
    call _drv_16k1b_set_vram_bank
    call _drv_16k1b_get_addr  ; HL = адрес, A = маска бита
    ld c, a
    pop af
    or a
    ld a, c
    jr z, @clear_pixel
    
    ; Установить бит
    or (hl)
    ld (hl), a
    ret
    
@clear_pixel:
    ; Сбросить бит
    cpl
    and (hl)
    ld (hl), a
    ret

; Нарисовать пиксель (с проверкой отсечения)
; Вход: BC = X, DE = Y, A = цвет
; Выход: A = 1 если нарисован, 0 если за пределами отсечения
_drv_16k1b_put_pixel:
    ; Проверка отсечения
    push hl
    ld hl, (_drv_16k1b_clip_rect)
    ld a, c
    cp l
    jr c, @outside
    cp h
    jr nc, @outside
    ; @@@ проверка Y @@@
    pop hl
    
    ; Если внутри - рисуем
    call _drv_16k1b_put_pixel_fast
    ld a, 1
    ret
    
@outside:
    pop hl
    xor a
    ret

; --- Остальные методы (заглушки для примера) ---

_drv_16k1b_get_pixel:
_drv_16k1b_draw_hline:
_drv_16k1b_draw_vline:
_drv_16k1b_draw_line:
_drv_16k1b_draw_rect:
_drv_16k1b_fill_rect:
_drv_16k1b_draw_char_8x8:
_drv_16k1b_draw_bitmap:
_drv_16k1b_swap_buffers:
_drv_16k1b_get_backbuf:
_drv_16k1b_clear_backbuf:
    ; Реализация этих методов
    ret

_drv_16k1b_scroll_down:
_drv_16k1b_set_clip_rect:
_drv_16k1b_get_clip_rect:
_drv_16k1b_copy_rect:
    ; Реализация этих методов
    ret