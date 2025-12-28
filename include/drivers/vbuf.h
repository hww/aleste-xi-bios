/* ============================================================
 * vbuf.h - Интерфейс видеобуфера для C программ
 * ============================================================
 */

#ifndef VBUF_H
#define VBUF_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Структуры */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} vbuf_rect_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t bytes_per_line;
    uint16_t vram_size;
    uint8_t flags;
} vbuf_mode_info_t;

/* Обязательные методы драйвера */
bool vbuf_init(void);
const vbuf_mode_info_t* vbuf_get_mode_info(void);
bool vbuf_command(uint8_t cmd, void* param);

/* Управление буфером */
void vbuf_clear(uint8_t color);
void vbuf_clear_rect(const vbuf_rect_t* rect, uint8_t color);
void vbuf_scroll_up(uint8_t pixels);
void vbuf_scroll_down(uint8_t pixels);
void vbuf_set_clip_rect(const vbuf_rect_t* rect);
void vbuf_get_clip_rect(vbuf_rect_t* rect);
void vbuf_copy_rect(uint16_t src_x, uint16_t src_y, 
                    uint16_t dst_x, uint16_t dst_y,
                    uint16_t width, uint16_t height);

/* Примитивы рисования */
void* vbuf_get_addr(uint16_t x, uint16_t y, uint8_t* bitmask);
void vbuf_put_pixel_fast(uint16_t x, uint16_t y, uint8_t color);
bool vbuf_put_pixel(uint16_t x, uint16_t y, uint8_t color);
uint8_t vbuf_get_pixel(uint16_t x, uint16_t y);
void vbuf_draw_hline(uint16_t x, uint16_t y, uint16_t length, uint8_t color);
void vbuf_draw_vline(uint16_t x, uint16_t y, uint16_t length, uint8_t color);
void vbuf_draw_line(uint16_t x1, uint16_t y1, 
                    uint16_t x2, uint16_t y2, uint8_t color);
void vbuf_draw_rect(uint16_t x, uint16_t y, 
                    uint16_t width, uint16_t height, uint8_t color);
void vbuf_fill_rect(uint16_t x, uint16_t y, 
                    uint16_t width, uint16_t height, uint8_t color);

/* Текст и битмапы */
void vbuf_draw_char_at_addr(const uint8_t* font_char, void* vram_addr);
void vbuf_draw_char(const uint8_t* font, uint8_t char_code,
                    uint16_t x, uint16_t y);
void vbuf_draw_bitmap(const uint8_t* bitmap, uint16_t x, uint16_t y,
                      uint16_t width, uint16_t height);

/* Двойная буферизация */
void vbuf_swap_buffers(void);
void* vbuf_get_backbuf(void);
void vbuf_clear_backbuf(uint8_t color);

/* Системные функции */
bool vbuf_activate_driver(void* driver_vtable);
void* vbuf_get_current_vtable(void);

/* Константы */
#define VBUF_CMD_RESET             0x00
#define VBUF_CMD_ENABLE_DOUBLEBUF  0x01
#define VBUF_CMD_DISABLE_DOUBLEBUF 0x02
#define VBUF_CMD_SET_BANK          0x03

#define VBUF_FLAG_INTERLACED     (1 << 0)
#define VBUF_FLAG_DOUBLEBUF      (1 << 1)
#define VBUF_FLAG_LINEAR         (1 << 2)

#ifdef __cplusplus
}
#endif

#endif /* VBUF_H */