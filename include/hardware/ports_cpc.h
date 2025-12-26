#ifndef PORTS_CPC_H
#define PORTS_CPC_H

#include <stdint.h>

// ==============================================================================
// PORTS_CPC.H - Порты ввода-вывода Amstrad CPC (для C программ)
// ==============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// БАЗОВЫЕ АДРЕСА ПОРТОВ
// ==============================================================================

#define GA_PORT_BASE          0x7F00U    // Gate Array
#define CRTC_INDEX_PORT_BASE  0xBC00U    // CRTC Index Register
#define CRTC_DATA_OUT_BASE    0xBD00U    // CRTC Data Write
#define CRTC_STATUS_BASE      0xBE00U    // CRTC Status Read
#define CRTC_DATA_IN_BASE     0xBF00U    // CRTC Data Read
#define UPPER_ROM_PORT_BASE   0xDF00U    // Upper ROM Bank Select
#define PRINTER_PORT_BASE     0xEF00U    // Printer Port
#define PPI_PORTA_BASE        0xF400U    // PPI Port A (PSG Data)
#define PPI_PORTB_BASE        0xF500U    // PPI Port B (Status)
#define PPI_PORTC_BASE        0xF600U    // PPI Port C (Control)
#define PPI_CTRL_BASE         0xF700U    // PPI Control Register
#define FDC_MOTOR_BASE        0xFA7EU    // Floppy Motor Control
#define FDC_STATUS_BASE       0xFB7EU    // FDC Status Register
#define FDC_DATA_BASE         0xFB7FU    // FDC Data Register

// ==============================================================================
// МАКРОСЫ ДЛЯ ГЕНЕРАЦИИ ПОРТОВ
// ==============================================================================

// Любое значение XX (0x00-0xFF) работает для декодируемых портов
#define GA_PORT(xx)          (GA_PORT_BASE | (xx))
#define CRTC_INDEX_PORT(xx)  (CRTC_INDEX_PORT_BASE | (xx))
#define CRTC_DATA_OUT(xx)    (CRTC_DATA_OUT_BASE | (xx))
#define CRTC_STATUS(xx)      (CRTC_STATUS_BASE | (xx))
#define CRTC_DATA_IN(xx)     (CRTC_DATA_IN_BASE | (xx))
#define UPPER_ROM_PORT(xx)   (UPPER_ROM_PORT_BASE | (xx))
#define PRINTER_PORT(xx)     (PRINTER_PORT_BASE | (xx))
#define PPI_PORTA(xx)        (PPI_PORTA_BASE | (xx))
#define PPI_PORTB(xx)        (PPI_PORTB_BASE | (xx))
#define PPI_PORTC(xx)        (PPI_PORTC_BASE | (xx))
#define PPI_CTRL(xx)         (PPI_CTRL_BASE | (xx))

// Дисковод использует фиксированные адреса
#define FDC_MOTOR_CONTROL    FDC_MOTOR_BASE
#define FDC_STATUS_REG       FDC_STATUS_BASE
#define FDC_DATA_REG         FDC_DATA_BASE

// ==============================================================================
// БИТОВЫЕ МАСКИ
// ==============================================================================

// Gate Array биты
#define GA_PENR_BIT         0x40      // Бит 6: разрешение прерываний

// PPI Port B (статус, чтение)
#define PPIB_VSYNC          (1 << 0)  // Вертикальная синхронизация
#define PPIB_PRINT_BUSY     (1 << 1)  // Принтер занят
#define PPIB_CASSETTE_IN    (1 << 2)  // Ввод с кассеты
#define PPIB_EXP            (1 << 3)  // Расширение (обычно 1)
#define PPIB_SOUND_ON       (1 << 4)  // Звук включен
#define PPIB_PRINT_DATA     (1 << 5)  // Данные принтера
#define PPIB_CASSETTE_OUT   (1 << 6)  // Вывод на кассету
#define PPIB_KEYBOARD_RESET (1 << 7)  // Сброс клавиатуры

// PPI Port C (управление, запись)
#define PPIC_PSG_BDIR       (1 << 0)  // PSG BDIR (направление)
#define PPIC_PSG_BC1        (1 << 1)  // PSG BC1 (выбор регистра)
#define PPIC_CASSETTE_MOTOR (1 << 2)  // Мотор кассеты
#define PPIC_KEYB_ROW0      (1 << 3)  // Строка клавиатуры 0
#define PPIC_KEYB_ROW1      (1 << 4)  // Строка клавиатуры 1
#define PPIC_KEYB_ROW2      (1 << 5)  // Строка клавиатуры 2
#define PPIC_KEYB_ROW3      (1 << 6)  // Строка клавиатуры 3
#define PPIC_KEYB_ROW4      (1 << 7)  // Строка клавиатуры 4

// ==============================================================================
; УПРАВЛЕНИЕ PSG (AY-3-8910/8912)
// ==============================================================================

// Состояния управления PSG (BDIR, BC1)
typedef enum {
    PSG_IDLE       = 0x00,  // 00: Неактивно
    PSG_ADDR_LATCH = 0x02,  // 01: Защелкивание адреса (BC1=1, BDIR=0)
    PSG_READ_DATA  = 0x01,  // 10: Чтение данных (BC1=0, BDIR=1)
    PSG_WRITE_DATA = 0x03   // 11: Запись данных (BC1=1, BDIR=1)
} PSGControlState;

// Регистры PSG
typedef enum {
    PSG_REG_CHA_FINE   = 0,   // Канал A тон (младший байт)
    PSG_REG_CHA_COARSE = 1,   // Канал A тон (старший байт)
    PSG_REG_CHB_FINE   = 2,   // Канал B тон (младший байт)
    PSG_REG_CHB_COARSE = 3,   // Канал B тон (старший байт)
    PSG_REG_CHC_FINE   = 4,   // Канал C тон (младший байт)
    PSG_REG_CHC_COARSE = 5,   // Канал C тон (старший байт)
    PSG_REG_NOISE      = 6,   // Генератор шума
    PSG_REG_MIXER      = 7,   ; Смеситель (включение/выключение)
    PSG_REG_CHA_VOL    = 8,   // Громкость канала A
    PSG_REG_CHB_VOL    = 9,   // Громкость канала B
    PSG_REG_CHC_VOL    = 10,  // Громкость канала C
    PSG_REG_ENV_FINE   = 11,  ; Огибающая частота (младший байт)
    PSG_REG_ENV_COARSE = 12,  ; Огибающая частота (старший байт)
    PSG_REG_ENV_SHAPE  = 13   ; Форма огибающей
} PSGRegister;

// ==============================================================================
// CRTC 6845 РЕГИСТРЫ
// ==============================================================================

typedef enum {
    CRTC_H_TOTAL      = 0,    // R0: Всего символов по горизонтали
    CRTC_H_DISPLAYED  = 1,    // R1: Отображаемых символов по горизонтали
    CRTC_H_SYNC_POS   = 2,    // R2: Позиция горизонтальной синхронизации
    CRTC_SYNC_WIDTH   = 3,    // R3: Ширина синхроимпульсов (H и V)
    CRTC_V_TOTAL      = 4,    // R4: Всего строк по вертикали
    CRTC_V_ADJUST     = 5,    // R5: Корректировка вертикального общего
    CRTC_V_DISPLAYED  = 6,    // R6: Отображаемых строк по вертикали
    CRTC_V_SYNC_POS   = 7,    // R7: Позиция вертикальной синхронизации
    CRTC_INTERLACE    = 8,    // R8: Чересстрочность и смещение
    CRTC_MAX_SCAN     = 9,    // R9: Максимальный адрес строки сканирования
    CRTC_CURSOR_START = 10,   // R10: Начало курсора
    CRTC_CURSOR_END   = 11,   // R11: Конец курсора
    CRTC_START_HI     = 12,   // R12: Стартовый адрес (старший байт)
    CRTC_START_LO     = 13,   // R13: Стартовый адрес (младший байт)
    CRTC_CURSOR_HI    = 14,   // R14: Адрес курсора (старший байт)
    CRTC_CURSOR_LO    = 15,   // R15: Адрес курсора (младший байт)
    CRTC_LIGHT_PEN_HI = 16,   // R16: Световое перо (старший байт)
    CRTC_LIGHT_PEN_LO = 17    // R17: Световое перо (младший байт)
} CRTCRegister;

// ==============================================================================
// ИНЛАЙН-ФУНКЦИИ ДЛЯ РАБОТЫ С ПОРТАМИ (C с inline-ассемблером)
// ==============================================================================

// Проверить состояние VSYNC
static inline uint8_t check_vsync(void) {
    uint8_t status;
    __asm
        in a, (PPI_PORTB_BASE)
        ld l, a
    __endasm;
    return status;
}

// Ожидание вертикальной синхронизации
static inline void wait_vsync(void) {
    __asm
    .wait_vsync_loop:
        in a, (PPI_PORTB_BASE)
        bit 0, a          ; PPIB_VSYNC_BIT
        jr nz, .wait_vsync_loop
    .wait_vsync_active:
        in a, (PPI_PORTB_BASE)
        bit 0, a
        jr z, .wait_vsync_active
    __endasm;
}

// Запись в регистр CRTC
static inline void write_crtc_reg(uint8_t reg, uint8_t value) {
    __asm
        ld a, 4(ix)       ; reg
        out (CRTC_INDEX_PORT_BASE), a
        ld a, 5(ix)       ; value
        out (CRTC_DATA_OUT_BASE), a
    __endasm;
}

// Запись в регистр PSG
static inline void write_psg_reg(uint8_t reg, uint8_t value) {
    __asm
        ; Защелкиваем адрес
        ld a, 4(ix)       ; reg
        out (PPI_PORTA_BASE), a
        ld a, PSG_ADDR_LATCH
        out (PPI_PORTC_BASE), a
        xor a
        out (PPI_PORTC_BASE), a
        
        ; Записываем значение
        ld a, 5(ix)       ; value
        out (PPI_PORTA_BASE), a
        ld a, PSG_WRITE_DATA
        out (PPI_PORTC_BASE), a
        xor a
        out (PPI_PORTC_BASE), a
    __endasm;
}

// Чтение из регистра PSG
static inline uint8_t read_psg_reg(uint8_t reg) {
    uint8_t value;
    __asm
        ; Защелкиваем адрес
        ld a, 4(ix)       ; reg
        out (PPI_PORTA_BASE), a
        ld a, PSG_ADDR_LATCH
        out (PPI_PORTC_BASE), a
        xor a
        out (PPI_PORTC_BASE), a
        
        ; Читаем значение
        ld a, PSG_READ_DATA
        out (PPI_PORTC_BASE), a
        in a, (PPI_PORTA_BASE)
        ld l, a            ; возвращаем значение
        xor a
        out (PPI_PORTC_BASE), a
    __endasm;
    return value;
}

// ==============================================================================
// FDC 765 КОМАНДЫ
// ==============================================================================

typedef enum {
    FDC_CMD_SPECIFY     = 0x03,   // Specify (установка параметров)
    FDC_CMD_SENSE_DRIVE = 0x04,   // Sense Drive Status
    FDC_CMD_WRITE_DATA  = 0x05,   // Write Data
    FDC_CMD_READ_DATA   = 0x06,   // Read Data
    FDC_CMD_RECALIBRATE = 0x07,   // Recalibrate
    FDC_CMD_SENSE_INT   = 0x08,   // Sense Interrupt Status
    FDC_CMD_WRITE_DEL   = 0x09,   // Write Deleted Data
    FDC_CMD_READ_ID     = 0x0A,   // Read Sector ID
    FDC_CMD_READ_DEL    = 0x0C,   // Read Deleted Data
    FDC_CMD_FORMAT      = 0x0D,   // Format Track
    FDC_CMD_SEEK        = 0x0F,   // Seek
    FDC_CMD_SCAN_EQUAL  = 0x11,   // Scan Equal
    FDC_CMD_SCAN_LOW    = 0x19,   // Scan Low or Equal
    FDC_CMD_SCAN_HIGH   = 0x1D    // Scan High or Equal
} FDCCommand;

// Статус FDC
#define FDC_ST_READY       0x80   // Готовность FDC
#define FDC_ST_NOT_READY   0x08   // Дисковод не готов
#define FDC_ST_WRITE_PROT  0x40   // Защита от записи
#define FDC_ST_SEEK_END    0x20   // Конец поиска
#define FDC_ST_CRC_ERROR   0x10   // Ошибка CRC
#define FDC_ST_TRACK_0     0x04   // Дорожка 0
#define FDC_ST_INDEX       0x02   // Сигнал INDEX
#define FDC_ST_BUSY        0x01   // Занят выполнением команды

#ifdef __cplusplus
}
#endif

#endif // PORTS_CPC_H