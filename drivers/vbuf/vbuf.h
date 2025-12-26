#ifndef VBUF_H
#define VBUF_H

#include <stdint.h>

// Базовые системные вызовы через прокси-таблицу (0xFD60)
extern void vbuf_select_driver(void* drv_vtable) __z88dk_fastcall;
extern void vbuf_clear(uint8_t pattern) __z88dk_fastcall;
extern void vbuf_scroll_up(void);

// Примитивы (0xFD80)
extern uint16_t vbuf_get_addr(uint16_t x, uint16_t y);
extern void vbuf_draw_char(uint16_t vram_addr, const uint8_t* font_ptr);

// Указатели на таблицы конкретных драйверов (для vbuf_select_driver)
extern void drv_16k1b_table; 
extern void drv_16k4b_table;

#endif