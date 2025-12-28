#ifndef ALESTE_VBUF_H
#define ALESTE_VBUF_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Типы данных ==================== */

/**
 * Информация о видеорежиме
 */
typedef struct {
    uint16_t width;          // Ширина в пикселях
    uint16_t height;         // Высота в пикселях
    uint8_t  bpp;            // Битов на пиксель (1, 2, 4, 8)
    uint8_t  bytes_per_line; // Байт на строку
    uint16_t vram_size;      // Размер VRAM в байтах
    uint8_t  flags;          // Флаги режима
} vbuf_mode_info_t;

/**
 * Прямоугольник
 */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} vbuf_rect_t;

/* ==================== Константы ==================== */

// Флаги видеорежима
#define VBUF_FLAG_INTERLACED   (1 << 0)  // Чересстрочный режим
#define VBUF_FLAG_DOUBLEBUF    (1 << 1)  // Двойная буферизация
#define VBUF_FLAG_LINEAR       (1 << 2)  // Линейная адресация (не planar)

// Коды ошибок
#define VBUF_ERR_SUCCESS       0
#define VBUF_ERR_NOT_FOUND     1
#define VBUF_ERR_INVALID_MODE  2
#define VBUF_ERR_NO_MEMORY     3
#define VBUF_ERR_MMU_FAIL      4
#define VBUF_ERR_NOT_SUPPORTED 5

/* ==================== Системные методы ==================== */

/**
 * Выбор и инициализация драйвера видеобуфера
 * @param driver_id Идентификатор драйвера (см. VBUF_DRV_*)
 * @return Код ошибки (0 = успех)
 */
uint8_t vbuf_select_driver(uint8_t driver_id);

/**
 * Получить информацию о текущем режиме
 * @return Указатель на структуру с информацией
 */
const vbuf_mode_info_t* vbuf_get_mode_info(void);

/**
 * Выполнить команду драйвера
 * @param cmd Команда (см. VBUF_CMD_*)
 * @param param Дополнительный параметр
 * @return Результат выполнения
 */
uint8_t vbuf_command(uint8_t cmd, uint16_t param);

/* ==================== Управление буфером ==================== */

/**
 * Очистить весь буфер
 * @param color Цвет заполнения (палитровый индекс или паттерн)
 */
void vbuf_clear(uint8_t color);

/**
 * Очистить прямоугольную область
 * @param rect Прямоугольник
 * @param color Цвет заполнения
 */
void vbuf_clear_rect(const vbuf_rect_t* rect, uint8_t color);

/**
 * Прокрутить буфер вверх
 * @param lines Количество строк для прокрутки
 */
void vbuf_scroll_up(uint8_t lines);

/**
 * Прокрутить буфер вниз
 * @param lines Количество строк для прокрутки
 */
void vbuf_scroll_down(uint8_t lines);

/**
 * Установить область отсечения (clipping)
 * @param rect Прямоугольник или NULL для сброса
 */
void vbuf_set_clip_rect(const vbuf_rect_t* rect);

/**
 * Получить текущую область отсечения
 * @param rect Указатель для заполнения
 */
void vbuf_get_clip_rect(vbuf_rect_t* rect);

/**
 * Копировать прямоугольную область внутри буфера
 * @param src Исходный прямоугольник
 * @param dst_x Целевая X координата
 * @param dst_y Целевая Y координата
 */
void vbuf_copy_rect(const vbuf_rect_t* src, uint16_t dst_x, uint16_t dst_y);

/* ==================== Работа с пикселями ==================== */

/**
 * Получить адрес в VRAM для координат
 * @param x Координата X
 * @param y Координата Y
 * @return Адрес в формате: (bank << 16) | offset
 */
uint32_t vbuf_get_addr(uint16_t x, uint16_t y);

/**
 * Нарисовать пиксель (без проверки отсечения)
 * @param x Координата X
 * @param y Координата Y
 * @param color Цвет
 */
void vbuf_put_pixel_fast(uint16_t x, uint16_t y, uint8_t color);

/**
 * Нарисовать пиксель (с проверкой отсечения)
 * @param x Координата X
 * @param y Координата Y
 * @param color Цвет
 * @return true если пиксель нарисован
 */
bool vbuf_put_pixel(uint16_t x, uint16_t y, uint8_t color);

/**
 * Получить цвет пикселя
 * @param x Координата X
 * @param y Координата Y
 * @return Цвет пикселя
 */
uint8_t vbuf_get_pixel(uint16_t x, uint16_t y);

/**
 * Нарисовать горизонтальную линию (быстрая)
 * @param x Начальная X
 * @param y Y координата
 * @param length Длина линии
 * @param color Цвет
 */
void vbuf_draw_hline(uint16_t x, uint16_t y, uint16_t length, uint8_t color);

/**
 * Нарисовать вертикальную линию (быстрая)
 * @param x X координата
 * @param y Начальная Y
 * @param length Длина линии
 * @param color Цвет
 */
void vbuf_draw_vline(uint16_t x, uint16_t y, uint16_t length, uint8_t color);

/**
 * Нарисовать линию (алгоритм Брезенхэма)
 * @param x0 Начальная X
 * @param y0 Начальная Y
 * @param x1 Конечная X
 * @param y1 Конечная Y
 * @param color Цвет
 */
void vbuf_draw_line(uint16_t x0, uint16_t y0, 
                    uint16_t x1, uint16_t y1, 
                    uint8_t color);

/**
 * Нарисовать прямоугольник (контур)
 * @param rect Прямоугольник
 * @param color Цвет границы
 */
void vbuf_draw_rect(const vbuf_rect_t* rect, uint8_t color);

/**
 * Заполнить прямоугольник
 * @param rect Прямоугольник
 * @param color Цвет заливки
 */
void vbuf_fill_rect(const vbuf_rect_t* rect, uint8_t color);

/* ==================== Работа с текстом и битмапами ==================== */

/**
 * Нарисовать 8x8 символ (1bpp)
 * @param x Координата X
 * @param y Координата Y
 * @param glyph_data Данные глифа (8 байт)
 * @param color Цвет символа (для 1bpp)
 */
void vbuf_draw_char_8x8(uint16_t x, uint16_t y, 
                       const uint8_t* glyph_data, 
                       uint8_t color);

/**
 * Нарисовать битмап (1bpp)
 * @param x Координата X
 * @param y Координата Y
 * @param width Ширина в пикселях (до 255)
 * @param height Высота в пикселях (до 255)
 * @param bitmap_data Данные битмапа
 * @param color Цвет (для 1bpp)
 * @param transparent Прозрачный цвет (только для многоцветных режимов)
 */
void vbuf_draw_bitmap(uint16_t x, uint16_t y,
                      uint8_t width, uint8_t height,
                      const uint8_t* bitmap_data,
                      uint8_t color,
                      uint8_t transparent);

/* ==================== Двойная буферизация ==================== */

/**
 * Показать задний буфер (если поддерживается)
 */
void vbuf_swap_buffers(void);

/**
 * Получить адрес заднего буфера
 * @return Адрес в формате: (bank << 16) | offset
 */
uint32_t vbuf_get_backbuffer_addr(void);

/**
 * Очистить задний буфер
 * @param color Цвет заполнения
 */
void vbuf_clear_backbuffer(uint8_t color);

/* ==================== Предопределенные драйверы ==================== */

// Команды для vbuf_command()
#define VBUF_CMD_RESET           0x00  // Сброс драйвера
#define VBUF_CMD_ENABLE_DOUBLEBUF 0x01  // Включить двойную буферизацию
#define VBUF_CMD_DISABLE_DOUBLEBUF 0x02 // Выключить двойную буферизацию
#define VBUF_CMD_SET_BANK        0x03  // Установить банк MMU (param: номер банка)

/* ==================== Макросы для удобства ==================== */

// Быстрая проверка ошибок
#define VBUF_CHECK(expr) do { \
    uint8_t __err = (expr); \
    if (__err != VBUF_ERR_SUCCESS) { \
        return __err; \
    } \
} while(0)

// Автоматический выбор драйвера с проверкой
#define VBUF_INIT(driver_id) do { \
    uint8_t __err = vbuf_select_driver(driver_id); \
    if (__err != VBUF_ERR_SUCCESS) { \
        /* Обработка ошибки */ \
    } \
} while(0)

#ifdef __cplusplus
}
#endif

#endif // ALESTE_VBUF_H