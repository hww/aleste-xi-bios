/* ============================================================
 * vbuf.c - C интерфейс для видеобуфера (конверсия аргументов)
 * ============================================================
 */

#include "drivers/vbuf.h"


/* Обязательные методы */
bool vbuf_init(void) {
    __asm
        call __vbuf_init
        ld l, a
    __endasm;
    return 0;
}

const vbuf_mode_info_t* vbuf_get_mode_info(void) {
    __asm
        call __vbuf_get_mode_info
    __endasm;
    return 0;
}

bool vbuf_command(uint8_t cmd, void* param) {
    cmd; param;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; cmd
        inc hl
        ld e, (hl)  ; param L
        inc hl
        ld d, (hl)  ; param H
        call __vbuf_command
    __endasm;
    return 0;
}

/* Управление буфером */
void vbuf_clear(uint8_t color) {
    color;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; color
        call __vbuf_clear
    __endasm;
}

void vbuf_clear_rect(const vbuf_rect_t* rect, uint8_t color) {
    rect; color;
    __asm
        ld hl, #2
        add hl, sp
        ld e, (hl)  ; rect L
        inc hl
        ld d, (hl)  ; rect H
        inc hl
        ld a, (hl)  ; color
        ex de, hl    ; HL = rect
        call __vbuf_clear_rect
    __endasm;
}

void vbuf_scroll_up(uint8_t pixels) {
    pixels;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; pixels
        call __vbuf_scroll_up
    __endasm;
}

void vbuf_scroll_down(uint8_t pixels) {
    pixels;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; pixels
        call __vbuf_scroll_down
    __endasm;
}

/* Примитивы рисования */
void vbuf_put_pixel_fast(uint16_t x, uint16_t y, uint8_t color) {
    x; y; color;
    __asm
        ; x, y, color на стеке: [SP+2] = x.l, [SP+3] = x.h
        ; [SP+4] = y.l, [SP+5] = y.h, [SP+6] = color
        ld hl, #2
        add hl, sp
        ld c, (hl)  ; x.l
        inc hl
        ld b, (hl)  ; x.h
        inc hl
        ld e, (hl)  ; y.l
        inc hl
        ld d, (hl)  ; y.h
        inc hl
        ld a, (hl)  ; color
        call __vbuf_put_pixel_fast
    __endasm;
}

bool vbuf_put_pixel(uint16_t x, uint16_t y, uint8_t color) {
    x; y; color;
    __asm
//        ; Аналогично put_pixel_fast
         ld hl, #2
        add hl, sp
         ld c, (hl)  ; x.l
        inc hl
        ld b, (hl)  ; x.h
        inc hl
        ld e, (hl)  ; y.l
        inc hl
        ld d, (hl)  ; y.h
        inc hl
        ld a, (hl)  ; color
        call __vbuf_put_pixel
        ld l, a     ; результат в L
    __endasm;
    return 0;
}

///* Текст */
void vbuf_draw_char(const uint8_t* font, uint8_t char_code,
                   uint16_t x, uint16_t y) {
    font; char_code; x; y;
    __asm
        ; [SP+2] = font L, 
        ; [SP+3] = font H
        ; [SP+4] = char_code, [SP+6] = x.l, [SP+7] = x.h
        ; [SP+8] = y.l, [SP+9] = y.h
        ld hl, #2
        add hl, sp
        ld e, (hl)  ; font L
        inc hl
        ld d, (hl)  ; font H
        inc hl
        inc hl      ; пропускаем старший байт char_code
        ld a, (hl)  ; char_code
        inc hl
        ld c, (hl)  ; x.l
        inc hl
        ld b, (hl)  ; x.h
        inc hl
        ld l, (hl)  ; y.l (сохраняем в L)
        inc hl
        ld h, (hl)  ; y.h
        
        ; Теперь параметры:
        ; DE = font, A = char_code, BC = x, HL = y
        push hl     ; сохраняем y
        push bc     ; сохраняем x
        push de     ; сохраняем font
        
        ; Восстанавливаем в порядке для функции:
        pop hl      ; HL = font
        pop bc      ; BC = x
        pop de      ; DE = y
        ; A уже содержит char_code
        
        call __vbuf_draw_char
    __endasm;
}

/* Системные функции */
bool vbuf_activate_driver(void* driver_vtable) {
    driver_vtable;
    __asm
        ld hl, #2
        add hl, sp
        ld e, (hl)  ; driver_vtable L
        inc hl
        ld d, (hl)  ; driver_vtable H
        ex de, hl   ; HL = driver_vtable
        call __vbuf_activate_driver
        ld l, a     ; результат в L
    __endasm;
    return 0;
}

void* vbuf_get_current_vtable(void) {
    __asm
        call __vbuf_get_current_vtable
        ; результат уже в HL
    __endasm;
    return 0;
}