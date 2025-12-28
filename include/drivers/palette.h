/**
 * @file palette.h
 * @brief Color Palette Driver Interface (C API)
 * @version 1.0
 * 
 * Полиморфный драйвер палитры для Aleste LX BIOS.
 * Прямые вызовы через фиксированные адреса с преобразованием
 * из C-аргументов в регистры Z80.
 */

#ifndef __PALETTE_H__
#define __PALETTE_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// БАЗОВЫЕ ТИПЫ
// ============================================================================

/** 12-битный цвет: RRRGGGGG BBBBBBBB */
typedef uint16_t palette_color_12b_t;

/** Цвет CPC (6-битный: RRGGBB) */
typedef uint8_t palette_color_cpc_t;

/** Цвет MSX (9-битный) */
typedef uint16_t palette_color_msx_t;

/** Режимы палитры */
typedef enum {
    PAL_MODE_12BIT = 0x00,  /**< 12-bit RGB (4-4-4) */
    PAL_MODE_15BIT = 0x01,  /**< 15-bit RGB (5-5-5) */
    PAL_MODE_CPC   = 0x02,  /**< CPC compatible (6-bit) */
    PAL_MODE_MSX   = 0x03,  /**< MSX compatible (9-bit) */
    PAL_MODE_YJK   = 0x04,  /**< YJK mode */
    PAL_MODE_AUTO  = 0xFF   /**< Автовыбор по аппаратуре */
} palette_mode_t;

/** Тип модификатора цвета */
typedef enum {
    PAL_MOD_OR  = 0,        /**< Логическое OR с цветом */
    PAL_MOD_XOR = 1         /**< Логическое XOR с цветом */
} palette_modifier_t;

// ============================================================================
// ОСНОВНОЙ API (базовые функции)
// ============================================================================

/**
 * @brief Проверить наличие палитры
 * @return true если палитра обнаружена, false в противном случае
 */
bool palette_detect(void) __z88dk_fastcall __naked;

/**
 * @brief Инициализировать палитру
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_install(void) __z88dk_fastcall __naked;

/**
 * @brief Получить информацию о драйвере
 * @return Указатель на DriverInfo
 */
void* palette_get_info(void) __z88dk_fastcall __naked;

/**
 * @brief Выполнить команду драйвера
 * @param cmd Код команды
 * @param param Параметр команды
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_command(uint8_t cmd, uint16_t param) __naked;

/**
 * @brief Деинициализировать палитру
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_uninstall(void) __z88dk_fastcall __naked;

// ============================================================================
// СПЕЦИФИЧНЫЕ МЕТОДЫ ПАЛИТРЫ
// ============================================================================

/**
 * @brief Установить цвет в палитре (12-bit режим)
 * @param index Индекс цвета (0-255)
 * @param color Цвет в формате 12-bit RGB
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_set_color_12b(uint8_t index, palette_color_12b_t color) __naked;

/**
 * @brief Установить цвет в палитре (CPC режим)
 * @param index Индекс цвета (0-31)
 * @param color Цвет в формате CPC (6-bit)
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_set_color_cpc(uint8_t index, palette_color_cpc_t color) __naked;

/**
 * @brief Установить цвет бордюра
 * @param color Цвет бордюра
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_set_border(palette_color_12b_t color) __z88dk_fastcall __naked;

/**
 * @brief Загрузить массив цветов в палитру
 * @param start_index Начальный индекс
 * @param colors Указатель на массив цветов
 * @param count Количество цветов (0 = 256)
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_load_colors(uint8_t start_index, const void* colors, uint8_t count) __naked;

// ============================================================================
// ДОПОЛНИТЕЛЬНЫЙ API (команды)
// ============================================================================

/**
 * @brief Установить режим палитры
 * @param mode Режим работы палитры
 * @return 0 при успехе, код ошибки при неудаче
 */
static inline uint8_t palette_set_mode(palette_mode_t mode) {
    return palette_command(0x10, (uint16_t)mode);
}

/**
 * @brief Установить модификатор цвета
 * @param type Тип модификатора
 * @param value Значение модификатора
 * @return 0 при успехе, код ошибки при неудаче
 */
static inline uint8_t palette_set_modifier(palette_modifier_t type, uint8_t value) {
    uint16_t param = ((uint16_t)value << 8) | (uint16_t)type;
    return palette_command(0x11, param);
}

/**
 * @brief Включить/выключить автоинкремент
 * @param enable 1 - включить, 0 - выключить
 * @return 0 при успехе, код ошибки при неудаче
 */
static inline uint8_t palette_set_autoinc(uint8_t enable) {
    return palette_command(0x12, (uint16_t)enable);
}

/**
 * @brief Сохранить состояние палитры
 * @param buffer Буфер для сохранения (64 байта)
 * @return 0 при успехе, код ошибки при неудаче
 */
static inline uint8_t palette_save_state(void* buffer) {
    return palette_command(0x20, (uint16_t)buffer);
}

/**
 * @brief Восстановить состояние палитры
 * @param buffer Буфер с сохраненным состоянием
 * @return 0 при успехе, код ошибки при неудаче
 */
static inline uint8_t palette_restore_state(const void* buffer) {
    return palette_command(0x21, (uint16_t)buffer);
}

// ============================================================================
// УПРАВЛЕНИЕ ПОЛИМОРФИЗМОМ
// ============================================================================

/**
 * @brief Активировать 12-bit RGB драйвер палитры
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_activate_12bit(void) __z88dk_fastcall __naked;

/**
 * @brief Активировать CPC-совместимый драйвер палитры
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_activate_cpc(void) __z88dk_fastcall __naked;

/**
 * @brief Активировать MSX-совместимый драйвер палитры
 * @return 0 при успехе, код ошибки при неудаче
 */
uint8_t palette_activate_msx(void) __z88dk_fastcall __naked;

/**
 * @brief Получить текущую активную VTable
 * @return Адрес текущей VTable
 */
void* palette_get_current_vtable(void) __z88dk_fastcall __naked;

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/** Преобразовать 8-битный цвет в 12-битный (потеря точности) */
static inline palette_color_12b_t palette_8bit_to_12bit(uint8_t color) {
    // RRRGGGBB -> RRRRGGGGBBBB
    return ((color & 0xE0) << 4) | ((color & 0x1C) << 6) | ((color & 0x03) << 8);
}

/** Преобразовать 12-битный цвет в 8-битный (потеря точности) */
static inline uint8_t palette_12bit_to_8bit(palette_color_12b_t color) {
    // RRRRGGGGBBBB -> RRRGGGBB
    return ((color >> 4) & 0xE0) | ((color >> 6) & 0x1C) | ((color >> 8) & 0x03);
}

/** Создать 12-битный цвет из компонентов */
static inline palette_color_12b_t palette_rgb_12bit(uint8_t r, uint8_t g, uint8_t b) {
    // r,g,b в диапазоне 0-15
    return ((r & 0x0F) << 8) | ((g & 0x0F) << 4) | (b & 0x0F);
}

/** Создать CPC цвет из компонентов */
static inline palette_color_cpc_t palette_cpc_rgb(uint8_t r, uint8_t g, uint8_t b) {
    // r,g,b в диапазоне 0-3
    return ((r & 0x03) << 4) | ((g & 0x03) << 2) | (b & 0x03);
}

// ============================================================================
// КОНСТАНТЫ ДЛЯ С-ПРОГРАММ
// ============================================================================

/** Коды ошибок */
#define PAL_ERR_SUCCESS          0
#define PAL_ERR_NOT_SUPPORTED    1
#define PAL_ERR_INVALID_PARAM    2
#define PAL_ERR_TIMEOUT          4
#define PAL_ERR_NO_DEVICE        11

/** Команды драйвера */
#define PAL_CMD_SET_MODE         0x10
#define PAL_CMD_SET_MODIFIER     0x11
#define PAL_CMD_SET_AUTOINC      0x12
#define PAL_CMD_SAVE_STATE       0x20
#define PAL_CMD_RESTORE_STATE    0x21

/** Предопределенные цвета (12-bit) */
#define PAL_COLOR_BLACK      0x0000
#define PAL_COLOR_WHITE      0x0FFF
#define PAL_COLOR_RED        0x0F00
#define PAL_COLOR_GREEN      0x00F0
#define PAL_COLOR_BLUE       0x000F
#define PAL_COLOR_YELLOW     0x0FF0
#define PAL_COLOR_CYAN       0x00FF
#define PAL_COLOR_MAGENTA    0x0F0F
#define PAL_COLOR_GRAY       0x0888
#define PAL_COLOR_DARK_GRAY  0x0444
#define PAL_COLOR_LIGHT_GRAY 0x0CCC

#ifdef __cplusplus
}
#endif

#endif /* __PALETTE_H__ */