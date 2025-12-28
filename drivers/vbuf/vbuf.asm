;; ============================================================
;; vbuf.asm - Полиморфный драйвер видеобуфера
;; ============================================================

module vbuf

include "../../inc/bios/undefined_method.inc"

; ---- Секция: Полиморфная таблица (системный слот) ----
section vbuf_vtable
align 32

; Метка начала таблицы (для копирования из драйверов)
PUBLIC __vbuf_vtable_start
; Системные методы (0xFD60-0xFD7F)
PUBLIC __vbuf_init
PUBLIC __vbuf_get_mode_info
PUBLIC __vbuf_command

; Управление буфером
PUBLIC __vbuf_clear
PUBLIC __vbuf_clear_rect
PUBLIC __vbuf_scroll_up
PUBLIC __vbuf_scroll_down
PUBLIC __vbuf_set_clip_rect
PUBLIC __vbuf_get_clip_rect
PUBLIC __vbuf_copy_rect

; ---- Примитивы рисования ----
PUBLIC __vbuf_get_addr
PUBLIC __vbuf_put_pixel_fast
PUBLIC __vbuf_put_pixel
PUBLIC __vbuf_get_pixel
PUBLIC __vbuf_draw_hline
PUBLIC __vbuf_draw_vline
PUBLIC __vbuf_draw_line
PUBLIC __vbuf_draw_rect
PUBLIC __vbuf_fill_rect

; Работа с текстом и битмапами
PUBLIC __vbuf_draw_char_at_addr
PUBLIC __vbuf_draw_char
PUBLIC __vbuf_draw_bitmap

; Двойная буферизация
PUBLIC __vbuf_swap_buffers
PUBLIC __vbuf_get_backbuf
PUBLIC __vbuf_clear_backbuf
__vbuf_vtable_start:

; Системные методы (0xFD60-0xFD7F)
__vbuf_init:         jp _undefined_method    ; +0  Инициализация
__vbuf_get_mode_info:jp _undefined_method    ; +3  Информация о режиме
__vbuf_command:      jp _undefined_method    ; +6  Команда драйвера

; Управление буфером
__vbuf_clear:        jp _undefined_method    ; +9  Очистить буфер
__vbuf_clear_rect:   jp _undefined_method    ; +12 Очистить прямоугольник
__vbuf_scroll_up:    jp _undefined_method    ; +15 Прокрутка вверх
__vbuf_scroll_down:  jp _undefined_method    ; +18 Прокрутка вниз
__vbuf_set_clip_rect:jp _undefined_method    ; +21 Установить отсечение
__vbuf_get_clip_rect:jp _undefined_method    ; +24 Получить отсечение
__vbuf_copy_rect:    jp _undefined_method    ; +27 Копировать область

; ---- Примитивы рисования ----
align 32

__vbuf_get_addr:     jp _undefined_method    ; +0  Адрес VRAM
__vbuf_put_pixel_fast:jp _undefined_method   ; +3  Пиксель (быстро)
__vbuf_put_pixel:    jp _undefined_method    ; +6  Пиксель (с отсечением)
__vbuf_get_pixel:    jp _undefined_method    ; +9  Получить пиксель
__vbuf_draw_hline:   jp _undefined_method    ; +12 Горизонтальная линия
__vbuf_draw_vline:   jp _undefined_method    ; +15 Вертикальная линия
__vbuf_draw_line:    jp _undefined_method    ; +18 Линия Брезенхэма
__vbuf_draw_rect:    jp _undefined_method    ; +21 Контур прямоугольника
__vbuf_fill_rect:    jp _undefined_method    ; +24 Заливка прямоугольника

; Работа с текстом и битмапами
__vbuf_draw_char_at_addr: jp _undefined_method ; +27 Символ по адресу
__vbuf_draw_char:    jp _undefined_method    ; +30 Символ по координатам
__vbuf_draw_bitmap:  jp _undefined_method    ; +33 Битмап

; Двойная буферизация
__vbuf_swap_buffers: jp _undefined_method    ; +36 Показать задний буфер
__vbuf_get_backbuf:  jp _undefined_method    ; +39 Адрес заднего буфера
__vbuf_clear_backbuf:jp _undefined_method    ; +42 Очистить задний буфер

; Метка конца таблицы
PUBLIC __vbuf_vtable_end
__vbuf_vtable_end:

; ---- Секция: Системные функции ----
section vbuf_system

; Вход: HL = адрес VTable конкретного драйвера
; Выход: A = код ошибки, CF=1 при ошибке
PUBLIC __vbuf_activate_driver
__vbuf_activate_driver:
    ld de, __vbuf_vtable_start
    ld (current_driver), hl      ; сохраняем адрес исходной VTable
    ld bc, __vbuf_vtable_end - __vbuf_vtable_start
    ldir                         ; копируем VTable в системный слот
    xor a                        ; успех
    ret

; Получить текущую активную VTable
; Выход: HL = адрес текущей VTable
PUBLIC __vbuf_get_current_vtable
__vbuf_get_current_vtable:
    ld hl, (current_driver)
    ret

section vbuf_data
current_driver: dw 0x0000