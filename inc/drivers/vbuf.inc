; ============================================================
; vbuf.inc - Интерфейс видеобуфера для ассемблера
; ============================================================

; Только extern объявления (никаких equ!)
extern __vbuf_init
extern __vbuf_get_mode_info
extern __vbuf_command
extern __vbuf_clear
extern __vbuf_clear_rect
extern __vbuf_scroll_up
extern __vbuf_scroll_down
extern __vbuf_set_clip_rect
extern __vbuf_get_clip_rect
extern __vbuf_copy_rect
extern __vbuf_get_addr
extern __vbuf_put_pixel_fast
extern __vbuf_put_pixel
extern __vbuf_get_pixel
extern __vbuf_draw_hline
extern __vbuf_draw_vline
extern __vbuf_draw_line
extern __vbuf_draw_rect
extern __vbuf_fill_rect
extern __vbuf_draw_char_at_addr
extern __vbuf_draw_char
extern __vbuf_draw_bitmap
extern __vbuf_swap_buffers
extern __vbuf_get_backbuf
extern __vbuf_clear_backbuf

; Системные функции
extern vbuf_activate_driver
extern vbuf_get_current_vtable

; Конкретные драйверы
extern _drv_16k1b_vtable

; Макросы для удобства
macro VBUF_INIT
    call __vbuf_init
endm

macro VBUF_CLEAR color
    ld a, color
    call __vbuf_clear
endm

macro VBUF_DRAW_PIXEL x, y, color
    ld bc, x
    ld de, y
    ld a, color
    call __vbuf_put_pixel
endm

macro VBUF_FAST_PIXEL x, y, color
    ld bc, x
    ld de, y
    ld a, color
    call __vbuf_put_pixel_fast
endm

macro VBUF_DRAW_CHAR font, char, x, y
    ld hl, font
    ld a, char
    ld bc, x
    ld de, y
    call __vbuf_draw_char
endm

macro VBUF_SCROLL_UP pixels
    ld a, pixels
    call __vbuf_scroll_up
endm

macro VBUF_SCROLL_DOWN pixels
    ld a, pixels
    call __vbuf_scroll_down
endm