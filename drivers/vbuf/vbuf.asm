;; ============================================================
;; vbuf.asm - Полиморфный драйвер видеобуфера
;; ============================================================

module vbuf

include "../../inc/drivers/vbuf.inc"
include "../../inc/bios/undefined_method.inc"

; ---- Секция: Полиморфная таблица (системный слот) ----
section vbuf_vtable
align 32

; Метка начала таблицы (для копирования из драйверов)
PUBLIC _vbuf_vtable_start
_vbuf_vtable_start:

; Системные методы (0xFD60-0xFD7F)
_vbuf_init:         jp _undefined_method    ; +0  Инициализация
_vbuf_get_mode_info:jp _undefined_method    ; +3  Информация о режиме
_vbuf_command:      jp _undefined_method    ; +6  Команда драйвера

; Управление буфером
_vbuf_clear:        jp _undefined_method    ; +9  Очистить буфер
_vbuf_clear_rect:   jp _undefined_method    ; +12 Очистить прямоугольник
_vbuf_scroll_up:    jp _undefined_method    ; +15 Прокрутка вверх
_vbuf_scroll_down:  jp _undefined_method    ; +18 Прокрутка вниз
_vbuf_set_clip_rect:jp _undefined_method    ; +21 Установить отсечение
_vbuf_get_clip_rect:jp _undefined_method    ; +24 Получить отсечение
_vbuf_copy_rect:    jp _undefined_method    ; +27 Копировать область

; ---- Примитивы рисования ----
align 32

_vbuf_get_addr:     jp _undefined_method    ; +0  Адрес VRAM
_vbuf_put_pixel_fast:jp _undefined_method   ; +3  Пиксель (быстро)
_vbuf_put_pixel:    jp _undefined_method    ; +6  Пиксель (с отсечением)
_vbuf_get_pixel:    jp _undefined_method    ; +9  Получить пиксель
_vbuf_draw_hline:   jp _undefined_method    ; +12 Горизонтальная линия
_vbuf_draw_vline:   jp _undefined_method    ; +15 Вертикальная линия
_vbuf_draw_line:    jp _undefined_method    ; +18 Линия Брезенхэма
_vbuf_draw_rect:    jp _undefined_method    ; +21 Контур прямоугольника
_vbuf_fill_rect:    jp _undefined_method    ; +24 Заливка прямоугольника

; Работа с текстом и битмапами
_vbuf_draw_char_8x8:jp _undefined_method    ; +27 Символ 8x8
_vbuf_draw_bitmap:  jp _undefined_method    ; +30 Битмап

; Двойная буферизация
_vbuf_swap_buffers: jp _undefined_method    ; +33 Показать задний буфер
_vbuf_get_backbuf:  jp _undefined_method    ; +36 Адрес заднего буфера
_vbuf_clear_backbuf:jp _undefined_method    ; +39 Очистить задний буфер

; Метка конца таблицы
PUBLIC _vbuf_vtable_end
_vbuf_vtable_end:

; ---- Секция: Системные функции ----
section vbuf_system

; Вход: A = ID драйвера
; Выход: A = код ошибки, CF=1 при ошибке
PUBLIC _vbuf_activate_driver
_vbuf_activate_driver:
    ld de, _vbuf_vtable_start
    ld (current_driver), de
    ld bc, _vbuf_vtable_end-_vbuf_vtable_start
    ldir
    ret

; Получить текущую активную VTable
; Выход: HL = адрес текущей VTable
PUBLIC _vbuf_get_current_vtable
_vbuf_get_current_vtable:
    ld hl, (current_driver)
    ret

SECTION DRV_DATA
current_driver: dw 0x0000