#ifndef PORTS_ALESTE_H
#define PORTS_ALESTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// PORTS_ALESTE.H - Порты и регистры Aleste LX (C версия)
// Соответствует правилам именования Aleste BIOS
// Версия 1.0
// ==============================================================================

// ==============================================================================
// ОБЩИЕ ПОРТЫ СИСТЕМЫ
// ==============================================================================

// ==================== MMU УПРАВЛЕНИЕ ====================

// Главные порты управления MMU
#define MMU_PORT_CTRL          0xF0U    // Регистр управления режимами
#define MMU_PORT_MMIO_PAGE     0xF1U    // Выбор страницы MMIO
#define MMU_PORT_SYSCALL       0xF2U    // Системный вызов
#define MMU_PORT_CLOCK_CTRL    0xF3U    // Управление частотой CPU
#define MMU_PORT_SUPER_SLOT    0xF9U    // Активный слот для Supervisor
#define MMU_PORT_USER_SLOT     0xFBU    // Активный слот для User
#define MMU_PORT_BANK_0        0xFCU    // Банк для окна 0 (0000-3FFF)
#define MMU_PORT_BANK_1        0xFDU    // Банк для окна 1 (4000-7FFF)
#define MMU_PORT_BANK_2        0xFEU    // Банк для окна 2 (8000-BFFF)
#define MMU_PORT_BANK_3        0xFFU    // Банк для окна 3 (C000-FFFF)

// ==================== ЗНАЧЕНИЯ ДЛЯ MMU_PORT_CTRL ====================

#define MMU_CTRL_SUPERVISOR_BIT 0x01U   // Бит 0: режим супервизора (1=вкл)
#define MMU_CTRL_NATIVE_BIT     0x02U   // Бит 1: нативный режим (1=вкл)
#define MMU_CTRL_TRAP_BIT       0x04U   // Бит 2: захват прерываний (1=вкл)
#define MMU_CTRL_USER_LOCK_BIT  0x10U   // Бит 4: блокировка MMIO в User режиме

// Предустановленные режимы MMU
#define MMU_CTRL_DEFAULT        (MMU_CTRL_SUPERVISOR_BIT | MMU_CTRL_NATIVE_BIT)

// ==================== ЗНАЧЕНИЯ ДЛЯ MMU_PORT_CLOCK_CTRL ====================

#define MMU_CLOCK_DIV_2         0x02U   // Делитель частоты 2
#define MMU_CLOCK_DIV_3         0x03U   // Делитель частоты 3
#define MMU_CLOCK_DIV_4         0x04U   // Делитель частоты 4 (6.25МГц при 25МГц)
#define MMU_CLOCK_DIV_5         0x05U   // Делитель частоты 5
#define MMU_CLOCK_DIV_6         0x06U   // Делитель частоты 6
#define MMU_CLOCK_DIV_7         0x07U   // Делитель частоты 7
#define MMU_CLOCK_DIV_8         0x08U   // Делитель частоты 8

#define MMU_CLOCK_CPC_BIT       0x10U   // Бит режима совместимости с CPC

// Маска для битов делителя
#define MMU_CLOCK_DIV_MASK      0x0FU   // Маска битов 0-3 (делитель)

// ==================== MMIO ОКНО ====================

#define MMIO_WINDOW_BEGIN       0x00U   // Начало окна MMIO (порт 0x00)
#define MMIO_WINDOW_END         0x7FU   // Конец окна MMIO (порт 0x7F)
#define MMIO_WINDOW_SIZE        0x80U   // Размер окна MMIO (128 портов)

// ==============================================================================
// УСТРОЙСТВА ЧЕРЕЗ MMIO (MMIO_PAGE значения)
// ==============================================================================

#define MMIO_PAGE_PIC           0x00U   // Страница контроллера прерываний
#define MMIO_PAGE_TIMER         0x00U   // Страница системного таймера
#define MMIO_PAGE_MMU_EXT       0x00U   // Страница расширенного MMU
#define MMIO_PAGE_PALETTE       0x02U   // Страница палитры цветов
#define MMIO_PAGE_CRTC          0x02U   // Страница CRTC контроллера
#define MMIO_PAGE_PSG           0x03U   // Страница PSG (AY-3-8910)
#define MMIO_PAGE_GA            0x04U   // Страница Gate Array
#define MMIO_PAGE_FDC           0x05U   // Страница контроллера дисковода
#define MMIO_PAGE_DMA           0x06U   // Страница DMA контроллера
#define MMIO_PAGE_SOUND         0x07U   // Страница звукового чипа
#define MMIO_PAGE_INPUT         0x08U   // Страница клавиатуры/мыши
#define MMIO_PAGE_RTC           0x09U   // Страница часов реального времени

// ==============================================================================
// ПАЛИТРА ЦВЕТОВ (при MMIO_PAGE = MMIO_PAGE_PALETTE)
// ==============================================================================

// Регистры палитры (смещения в MMIO_WINDOW)
#define PALETTE_REG_INDEX       0x00U   // Индекс цвета (0-15)
#define PALETTE_REG_DATA_LO     0x01U   // Младший байт цвета
#define PALETTE_REG_DATA_HI     0x02U   // Старший байт цвета
#define PALETTE_REG_BORDER_LO   0x03U   // Младший байт цвета бордюра
#define PALETTE_REG_BORDER_HI   0x04U   // Старший байт цвета бордюра
#define PALETTE_REG_CTRL        0x05U   // Регистр управления
#define PALETTE_REG_MODIFIER    0x06U   // Модификатор цвета

// Биты регистра управления палитрой (PALETTE_REG_CTRL)
#define PALETTE_CTRL_MODE_MASK  0x03U   // Маска режимов (биты 0-1)
#define PALETTE_CTRL_MODE_12BIT 0x00U   // 12-битный RGB (по умолчанию)
#define PALETTE_CTRL_MODE_CPC   0x01U   // Режим CPC (9-битный цвет)
#define PALETTE_CTRL_MODE_MSX   0x02U   // Режим MSX
#define PALETTE_CTRL_MODE_YJK   0x03U   // Режим YJK

#define PALETTE_CTRL_AUTO_INC_BIT   0x20U   // Автоинкремент при записи
#define PALETTE_CTRL_MOD_XOR_BIT    0x40U   // XOR модификатор
#define PALETTE_CTRL_MOD_ON_BIT     0x80U   // Включить модификатор

// ==============================================================================
// CRTC 6845 КОНТРОЛЛЕР (при MMIO_PAGE = MMIO_PAGE_CRTC)
// ==============================================================================

// Базовые регистры CRTC
#define CRTC_REG_ADDR           0x10U   // Регистр адреса (выбор регистра)
#define CRTC_REG_DATA           0x11U   // Регистр данных (чтение/запись)

// Стандартные регистры CRTC 6845
#define CRTC_REG_HTOTAL         0x00U   // R0: Всего символов по горизонтали
#define CRTC_REG_HDISPLAY       0x01U   // R1: Отображаемых символов
#define CRTC_REG_HSYNCPOS       0x02U   // R2: Позиция горизонтальной синхроимпульса
#define CRTC_REG_HSYNCWIDTH     0x03U   // R3: Ширина синхроимпульса
#define CRTC_REG_VTOTAL         0x04U   // R4: Всего строк
#define CRTC_REG_VADJUST        0x05U   // R5: Корректировка вертикального
#define CRTC_REG_VDISPLAY       0x06U   // R6: Отображаемых строк
#define CRTC_REG_VSYNCPOS       0x07U   // R7: Позиция вертикальной синхроимпульса
#define CRTC_REG_INTERLACE      0x08U   // R8: Чересстрочность
#define CRTC_REG_MAXSCAN        0x09U   // R9: Максимальный адрес строки
#define CRTC_REG_CURSTART       0x0AU   // R10: Начало курсора
#define CRTC_REG_CUREND         0x0BU   // R11: Конец курсора
#define CRTC_REG_STARTH         0x0CU   // R12: Старший байт начального адреса
#define CRTC_REG_STARTL         0x0DU   // R13: Младший байт начального адреса
#define CRTC_REG_CURH           0x0EU   // R14: Старший байт адреса курсора
#define CRTC_REG_CURL           0x0FU   // R15: Младший байт адреса курсора

// Расширенные регистры CRTC (Aleste LX)
#define CRTC_REG_VIDEO_CTRL     0x12U   // R18: Управление видео
#define CRTC_REG_HIGH_ADDRESS   0x13U   // R19: Старшие биты адреса (A16-A23)
#define CRTC_REG_ADDR_MODE      0x14U   // R20: Режим адресации
#define CRTC_REG_PIXEL_CTRL     0x15U   // R21: Управление пиксельными часами

// ==================== CRTC_REG_VIDEO_CTRL значения ====================

// Биты на пиксель (биты 0-1)
#define CRTC_VIDEO_CTRL_BPP_MASK    0x03U
#define CRTC_VIDEO_CTRL_BPP_1BPP    0x00U   // 1 бит/пиксель (2 цвета)
#define CRTC_VIDEO_CTRL_BPP_2BPP    0x01U   // 2 бита/пиксель (4 цвета)
#define CRTC_VIDEO_CTRL_BPP_4BPP    0x02U   // 4 бита/пиксель (16 цветов)
#define CRTC_VIDEO_CTRL_BPP_8BPP    0x03U   // 8 бит/пиксель (256 цветов)

// Другие биты
#define CRTC_VIDEO_CTRL_LINEAR_BIT  0x10U   // Бит 4: линейные пиксели
                                            // 0 = CPC палитра, 1 = Линейный RGB

#define CRTC_VIDEO_CTRL_CPC_MODES_BIT 0x20U // Бит 5: режимы CPC
                                            // 0 = Внутренние BPP режимы
                                            // 1 = CPC GateArray графика

// Предустановленные видеорежимы
#define CRTC_VIDEO_MODE_CPC_0  (CRTC_VIDEO_CTRL_CPC_MODES_BIT | CRTC_VIDEO_CTRL_BPP_1BPP)
#define CRTC_VIDEO_MODE_CPC_1  (CRTC_VIDEO_CTRL_CPC_MODES_BIT | CRTC_VIDEO_CTRL_BPP_2BPP)
#define CRTC_VIDEO_MODE_CPC_2  (CRTC_VIDEO_CTRL_CPC_MODES_BIT | CRTC_VIDEO_CTRL_BPP_4BPP)

// ==================== CRTC_REG_ADDR_MODE значения ====================

// Базовые биты
#define CRTC_ADDR_MODE_LINEAR_BIT   0x01U   // Бит 0: линейный режим адресации
#define CRTC_ADDR_MODE_STEP_BIT     0x02U   // Бит 1: шаг адреса
#define CRTC_ADDR_MODE_RATE_BIT     0x04U   // Бит 2: частота адресации

// Режимы адресации (биты 5:4)
#define CRTC_ADDR_MODE_MASK         0x30U   // Маска режимов адресации
#define CRTC_ADDR_MODE_CPC_16K      0x00U   // CPC 16KB (стандартный CPC)
#define CRTC_ADDR_MODE_EX_32K       0x10U   // EX 32KB (расширенный)
#define CRTC_ADDR_MODE_LX_32K       0x20U   // LX 32KB (Aleste LX)
#define CRTC_ADDR_MODE_LX_64K       0x30U   // LX 64KB (Aleste LX расширенный)
#define CRTC_ADDR_MODE_LINEAR       0x40U   // Полностью линейный

// ==================== CRTC_REG_PIXEL_CTRL значения ====================

#define CRTC_PIXEL_CTRL_RATE_MASK   0x03U   // Маска скорости пикселей (биты 0-1)
#define CRTC_PIXEL_CTRL_RATE_2BYTES 0x00U   // 2 байта за 16 тактов
#define CRTC_PIXEL_CTRL_RATE_4BYTES 0x01U   // 4 байта за 16 тактов
#define CRTC_PIXEL_CTRL_RATE_8BYTES 0x02U   // 8 байт за 16 тактов
#define CRTC_PIXEL_CTRL_RATE_16BYTES 0x03U  // 16 байт за 16 тактов

// ==============================================================================
// LEGACY ПОРТЫ (CPC СОВМЕСТИМОСТЬ)
// ==============================================================================

// Эти порты работают в Legacy режиме
#define CPC_PORT_RMR            0x7F00U  // RMR регистр CPC
#define CPC_PORT_DFXX           0xDF00U  // Выбор верхнего ROM
#define CPC_PORT_SYSCALL        0xF200U  // Legacy системный вызов

// ==============================================================================
// СИСТЕМНЫЕ КОНСТАНТЫ
// ==============================================================================

// Максимальные значения
#define MMU_BANK_MAX            255U     // Максимальный номер банка (0-255)
#define MMU_MMIO_PAGE_MAX       127U     // Максимальный номер страницы MMIO (0-127)

// Размеры
#define MMU_WINDOW_SIZE         0x4000U  // Размер окна памяти (16К)
#define MMU_BANK_SIZE           0x10000U // Размер банка памяти (64К)

#ifdef __cplusplus
}
#endif

#endif // PORTS_ALESTE_H
