#ifndef BIOS_DRIVERS_BASE_H
#define BIOS_DRIVERS_BASE_H

#include "errors.h"

/*
 * ОБЩИЕ ОПРЕДЕЛЕНИЯ ДРАЙВЕРОВ BIOS ALESTE
 */

// ==============================================================================
// ФЛАГИ ВОЗМОЖНОСТЕЙ ДРАЙВЕРОВ (API flags)
// ==============================================================================

#define API_HAS_IRQ           (1 << 0)  // Использует прерывания
#define API_STATIC_DEVICE     (1 << 1)  // Статическое устройство (встроенное)
#define API_USES_DMA          (1 << 2)  // Использует DMA
#define API_BANKED            (1 << 3)  // Работает с банковой памятью
#define API_POWER_SAVE        (1 << 4)  // Поддерживает энергосбережение
#define API_HOTPLUG           (1 << 5)  // Поддерживает горячее подключение
#define API_NEEDS_CONTEXT     (1 << 6)  // Требует контекст (для MMU)
#define API_THREAD_SAFE       (1 << 7)  // Потокобезопасный

// ==============================================================================
// СТРУКТУРА ИНФОРМАЦИИ О ДРАЙВЕРЕ
// ==============================================================================

#pragma pack(push, 1)
typedef struct {
    const char* name;        // Имя драйвера (ASCIIZ)
    uint8_t version_major;   // Старшая версия
    uint8_t version_minor;   // Младшая версия
    uint16_t api_flags;      // Флаги возможностей (см. выше)
    uint8_t irq_vector;      // Вектор прерывания (если API_HAS_IRQ)
    uint8_t reserved;        // Резерв
} DriverInfo;
#pragma pack(pop)

// ==============================================================================
// БАЗОВЫЕ МЕТОДЫ ДРАЙВЕРА (первые 5 методов в VTable)
// ==============================================================================

// Все драйверы должны реализовать эти методы:
// 1. detect    - проверка наличия устройства
// 2. install   - инициализация
// 3. get_info  - информация о драйвере
// 4. command   - универсальная команда (IOCTL)
// 5. uninstall - выгрузка драйвера

// ==============================================================================
// СИСТЕМНЫЕ КОМАНДЫ (все драйверы)
// ==============================================================================

// Эти команды должны поддерживать все драйверы через method_command()
#define CMD_RESET         0x00  // Сброс устройства
#define CMD_STATUS        0x01  // Получить статус
#define CMD_SUSPEND       0x02  // Приостановить работу
#define CMD_RESUME        0x03  // Возобновить работу
#define CMD_GET_CAPABILITIES 0x04 // Получить возможности
#define CMD_SET_POWER     0x05  // Установить режим питания

// ==============================================================================
// ДИАПАЗОНЫ КОМАНД ДЛЯ РАЗНЫХ ТИПОВ УСТРОЙСТВ
// ==============================================================================

#define CMD_VIDEO_BASE     0x10  // Видео
#define CMD_AUDIO_BASE     0x20  // Аудио
#define CMD_STORAGE_BASE   0x30  // Хранилище
#define CMD_INPUT_BASE     0x40  // Ввод
#define CMD_NETWORK_BASE   0x50  // Сеть
#define CMD_MMU_BASE       0x60  // MMU
#define CMD_SYSTEM_BASE    0x70  // Системные

// Проверка принадлежности команды диапазону
#define IS_VIDEO_CMD(cmd)    ((cmd) >= 0x10 && (cmd) < 0x20)
#define IS_AUDIO_CMD(cmd)    ((cmd) >= 0x20 && (cmd) < 0x30)
#define IS_STORAGE_CMD(cmd)  ((cmd) >= 0x30 && (cmd) < 0x40)
#define IS_INPUT_CMD(cmd)    ((cmd) >= 0x40 && (cmd) < 0x50)
#define IS_NETWORK_CMD(cmd)  ((cmd) >= 0x50 && (cmd) < 0x60)
#define IS_MMU_CMD(cmd)      ((cmd) >= 0x60 && (cmd) < 0x70)
#define IS_SYSTEM_CMD(cmd)   ((cmd) >= 0x70 && (cmd) < 0x80)

// ==============================================================================
// МАКРОСЫ ДЛЯ РАБОТЫ С ДРАЙВЕРАМИ
// ==============================================================================

// Размер базовой VTable (5 методов × 3 байта)
#define DRIVER_VTABLE_BASE_SIZE 15

// Максимальное количество специфичных методов
#define DRIVER_MAX_EXTRA_METHODS 10

// Полный размер VTable
#define DRIVER_VTABLE_FULL_SIZE (DRIVER_VTABLE_BASE_SIZE + \
                                 DRIVER_MAX_EXTRA_METHODS * 3)

// Проверка драйвера на наличие флага
#define DRIVER_HAS_FLAG(info, flag) ((info)->api_flags & (flag))

// Быстрая проверка статического устройства
#define IS_STATIC_DEVICE(info) DRIVER_HAS_FLAG(info, API_STATIC_DEVICE)

#endif // BIOS_DRIVERS_H