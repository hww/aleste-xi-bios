// =============================================================================
// PALETTE.H - Color Palette Driver Interface (C)
// For Aleste LX BIOS
// =============================================================================

#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Основные функции палитры
// -----------------------------------------------------------------------------

/**
 * @brief Установить цвет в палитре
 * @param index Индекс цвета (0-255)
 * @param color Цвет в формате текущего режима
 * @return true если успешно
 */
extern bool palette_set_color(uint8_t index, uint16_t color);

/**
 * @brief Получить цвет из палитры
 * @param index Индекс цвета (0-255)
 * @return Цвет в формате текущего режима
 */
extern uint16_t palette_get_color(uint8_t index);

/**
 * @brief Установить цвет бордюра
 * @param color Цвет бордюра
 */
extern void palette_set_border(uint16_t color);

/**
 * @brief Массовая загрузка палитры
 * @param colors Массив цветов
 * @param count Количество цветов (0 = 256)
 * @param start_index Начальный индекс
 */
extern void palette_load(const uint16_t* colors, uint8_t count, uint8_t start_index);

/**
 * @brief Выполнить команду палитры
 * @param cmd Команда
 * @param param Параметр команды
 * @return Результат выполнения
 */
extern int palette_command(uint8_t cmd, uint16_t param);

#ifdef __cplusplus
}
#endif

#endif // PALETTE_H