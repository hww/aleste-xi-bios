#ifndef MEMORY_ALESTE_H
#define MEMORY_ALESTE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================================================================
// MEMORY_ALESTE.H - Карта памяти Aleste LX (C версия)
// Соответствует спецификации MMU Architecture v2.0
// ==============================================================================

// ==============================================================================
// БАЗОВЫЕ КОНСТАНТЫ
// ==============================================================================

#define MEMORY_SIZE        0x01000000UL  // 16 МБ
#define PHYSICAL_RAM_SIZE  0x00100000UL  // 1 МБ ОЗУ

#define RAM_BASE           0x00000000UL
#define MMIO_BASE          0x00FF0000UL

// ==============================================================================
// ОКОННАЯ БАНКОВАЯ ПАМЯТЬ
// ==============================================================================

#define WB_MEMORY_BEGIN    0x000000UL
#define WB_MEMORY_END      0xFEFFFFUL
#define WB_MEMORY_SIZE     0xFF0000UL

#define WB_WINDOW_SIZE     0x00004000UL  // 16 КБ

#define WB_WINDOW0_BEGIN   0x000000UL
#define WB_WINDOW0_END     0x003FFFUL
#define WB_WINDOW0_DEFAULT 0

#define WB_WINDOW1_BEGIN   0x004000UL
#define WB_WINDOW1_END     0x007FFFUL
#define WB_WINDOW1_DEFAULT 1

#define WB_WINDOW2_BEGIN   0x008000UL
#define WB_WINDOW2_END     0x00BFFFUL
#define WB_WINDOW2_DEFAULT 2

#define WB_WINDOW3_BEGIN   0x00C000UL
#define WB_WINDOW3_END     0x00FFFFUL
#define WB_WINDOW3_DEFAULT 3

#define BANK_SIZE          0x00010000UL  // 64 КБ

// ==============================================================================
// MMIO ПРОСТРАНСТВО
// ==============================================================================

#define MMIO_BEGIN         0xFF0000UL
#define MMIO_END           0xFFFFFFUL
#define MMIO_SIZE          0x00010000UL

#define MMIO_LO_BEGIN      0xFF0000UL
#define MMIO_LO_END        0xFF3FFFUL
#define MMIO_LO_SIZE       0x00004000UL

#define MMIO_HI_BEGIN      0xFF4000UL
#define MMIO_HI_END        0xFFFFFFUL
#define MMIO_HI_SIZE       0x0000C000UL

// ==============================================================================
// УСТРОЙСТВА В MMIO_LO
// ==============================================================================

#define MMIO_PAGE0_BEGIN   0xFF0000UL

#define MMIO_PIC_BASE      0xFF0000UL
#define MMIO_PIC_SIZE      32

#define MMIO_IPC_BASE      0xFF0020UL
#define MMIO_IPC_SIZE      32

#define MMIO_TIMER_BASE    0xFF0040UL
#define MMIO_TIMER_SIZE    32

#define MMIO_RTC_BASE      0xFF0060UL
#define MMIO_RTC_SIZE      32

#define MMIO_MMU_EXT_BEGIN 0xFF0080UL
#define MMIO_MMU_EXT_END   0xFF00BFUL
#define MMIO_MMU_EXT_SIZE  64

#define MMIO_LEGACY_BEGIN  0xFF0100UL
#define MMIO_LEGACY_END    0xFF017FUL
#define MMIO_LEGACY_SIZE   128

#define MMIO_PALETTE_BASE  0xFF0100UL
#define MMIO_PALETTE_SIZE  32

#define MMIO_6845_BASE     0xFF0120UL
#define MMIO_6845_SIZE     32

// ==============================================================================
// MMIO_HI (LEGACY ЭМУЛЯЦИЯ)
// ==============================================================================

#define MMIO_GA_BASE       0xFF7F00UL
#define MMIO_GA_SIZE       256

#define MMIO_CRTC_INDEX    0xFFBC00UL
#define MMIO_CRTC_DATA     0xFFBD00UL

#define MMIO_UPPER_ROM     0xFFDF00UL

#define MMIO_PPI_PORTA     0xFFF400UL
#define MMIO_PPI_PORTB     0xFFF500UL
#define MMIO_PPI_PORTC     0xFFF600UL
#define MMIO_PPI_CTRL      0xFFF700UL

// ==============================================================================
// ПОРТЫ СИСТЕМНЫХ РЕГИСТРОВ
// ==============================================================================

#define PORT_GLOBAL_CTRL   0xF0
#define PORT_MMIO_PAGE     0xF1
#define PORT_SYSCALL       0xF2
#define PORT_CLOCK_CTRL    0xF3
#define PORT_SUPER_SLOT    0xF9
#define PORT_USER_SLOT     0xFB
#define PORT_BANK_0        0xFC
#define PORT_BANK_1        0xFD
#define PORT_BANK_2        0xFE
#define PORT_BANK_3        0xFF

#define PORT_LEGACY_RMR    0x7F00
#define PORT_LEGACY_MMR    0x7F00
#define PORT_LEGACY_UPPER_ROM 0xDF00
#define PORT_LEGACY_SYSCALL 0xF200

// ==============================================================================
// БИТЫ GLOBAL_CTRL
// ==============================================================================

#define GLOBAL_SUPERVISOR  0
#define GLOBAL_NATIVE      1
#define GLOBAL_TRAP_ENABLE 2
#define GLOBAL_RESERVED1   3
#define GLOBAL_MMIO_UNLOCK 4
#define GLOBAL_RESERVED2   5
#define GLOBAL_RESERVED3   6
#define GLOBAL_RESERVED4   7

#define GLOBAL_DEFAULT     ((1 << GLOBAL_SUPERVISOR) | (1 << GLOBAL_NATIVE))

// ==============================================================================
// СЛОТЫ ПАМЯТИ
// ==============================================================================

#define SLOT_0             0
#define SLOT_1             1
#define SLOT_2             2
#define SLOT_3             3

// ==============================================================================
// СИСТЕМНЫЕ ОБЛАСТИ ПАМЯТИ
// ==============================================================================

#define IVT_BEGIN          0x000000UL
#define IVT_END            0x0000FFUL
#define IVT_SIZE           0x00000100UL

#define SYS_STACK_BEGIN    0x00FF00UL
#define SYS_STACK_END      0x00FFFFUL
#define SYS_STACK_SIZE     0x00000100UL

#define SYS_VARS_BEGIN     0x000100UL
#define SYS_VARS_END       0x0003FFUL
#define SYS_VARS_SIZE      0x00000300UL

// ==============================================================================
// РАЗМЕЩЕНИЕ ПРОГРАММ
// ==============================================================================

#define PROGRAM_LOAD_BASE  0x004000UL
#define PROGRAM_LOAD_END   0x007FFFUL
#define PROGRAM_LOAD_SIZE  0x00004000UL

#define PROGRAM_DATA_BASE  0x008000UL
#define PROGRAM_DATA_END   0x00BFFFUL
#define PROGRAM_DATA_SIZE  0x00004000UL

#define PROGRAM_STACK_BASE 0x00C000UL
#define PROGRAM_STACK_TOP  0x00FFFFUL
#define PROGRAM_STACK_SIZE 0x00004000UL

// ==============================================================================
// РЕЖИМЫ ДОСТУПА
// ==============================================================================

#define MODE_SUPERVISOR_NATIVE ((1 << GLOBAL_SUPERVISOR) | (1 << GLOBAL_NATIVE))
#define MODE_USER_NATIVE     (1 << GLOBAL_NATIVE)
#define MODE_USER_LEGACY     0

// ==============================================================================
// СИСТЕМНЫЕ ВЫЗОВЫ
// ==============================================================================

#define SYS_MMIO_ACCESS_REQUEST 0xFE

// ==============================================================================
// ТИПЫ И СТРУКТУРЫ
// ==============================================================================

// Структура состояния маппера (64 байта)
#pragma pack(push, 1)
typedef struct {
    uint8_t supervisor_slot;
    uint8_t user_slot;
    uint8_t supervisor_banks[4];  // Для слота supervisor
    uint8_t user_banks[4];        // Для слота user
    uint8_t mmio_page;
    uint8_t reserved[53];         // Для будущего расширения
} MmuState;
#pragma pack(pop)

// ==============================================================================
// ИНЛАЙН-ФУНКЦИИ
// ==============================================================================

// Получить текущий режим
static inline uint8_t get_current_mode(void) {
    uint8_t mode;
    __asm
        in a, (PORT_GLOBAL_CTRL)
        ld l, a
    __endasm;
    return mode;
}

// Проверить, находимся ли мы в Supervisor режиме
static inline int is_supervisor_mode(void) {
    uint8_t mode = get_current_mode();
    return (mode & (1 << GLOBAL_SUPERVISOR)) != 0;
}

// Проверить, находимся ли мы в Native режиме
static inline int is_native_mode(void) {
    uint8_t mode = get_current_mode();
    return (mode & (1 << GLOBAL_NATIVE)) != 0;
}

// Проверить, разблокирован ли доступ к MMIO
static inline int is_mmio_unlocked(void) {
    if (is_supervisor_mode()) {
        return 1;  // В Supervisor всегда разрешено
    }
    uint8_t mode = get_current_mode();
    return (mode & (1 << GLOBAL_MMIO_UNLOCK)) != 0;
}

// Получить текущий активный слот
static inline uint8_t get_current_slot(void) {
    if (is_supervisor_mode()) {
        uint8_t slot;
        __asm
            in a, (PORT_SUPER_SLOT)
            ld l, a
        __endasm;
        return slot;
    } else {
        uint8_t slot;
        __asm
            in a, (PORT_USER_SLOT)
            ld l, a
        __endasm;
        return slot;
    }
}

// Переключить банк в окне
static inline void switch_bank(uint8_t window, uint8_t bank) {
    switch (window) {
        case 0:
            __asm
                ld a, %[bank]
                out (PORT_BANK_0), a
            __endasm;
            break;
        case 1:
            __asm
                ld a, %[bank]
                out (PORT_BANK_1), a
            __endasm;
            break;
        case 2:
            __asm
                ld a, %[bank]
                out (PORT_BANK_2), a
            __endasm;
            break;
        case 3:
            __asm
                ld a, %[bank]
                out (PORT_BANK_3), a
            __endasm;
            break;
    }
}

// Системный вызов
static inline void syscall(uint8_t code) {
    __asm
        ld a, %[code]
        out (PORT_SYSCALL), a
    __endasm;
}

// Legacy системный вызов
static inline void syscall_legacy(uint8_t code) {
    __asm
        ld a, %[code]
        out (PORT_LEGACY_SYSCALL), a
    __endasm;
}

// Войти в Supervisor Mode (только из Supervisor!)
static inline void enter_supervisor(uint8_t enable_trap) {
    uint8_t value = GLOBAL_DEFAULT;
    if (enable_trap) {
        value |= (1 << GLOBAL_TRAP_ENABLE);
    }
    __asm
        ld a, %[value]
        out (PORT_GLOBAL_CTRL), a
    __endasm;
}

// Выйти из Supervisor Mode
static inline void exit_supervisor(uint8_t native_mode) {
    uint8_t value = 0;
    if (native_mode) {
        value = (1 << GLOBAL_NATIVE);
    }
    __asm
        ld a, %[value]
        out (PORT_GLOBAL_CTRL), a
    __endasm;
}

// Установить страницу MMIO
static inline void set_mmio_page(uint8_t page) {
    __asm
        ld a, %[page]
        out (PORT_MMIO_PAGE), a
    __endasm;
}

// Записать в регистр MMIO
static inline void write_mmio(uint8_t page, uint8_t offset, uint8_t value) {
    set_mmio_page(page);
    __asm
        ld a, %[value]
        out (%[offset]), a
    __endasm;
}

// Прочитать из регистра MMIO
static inline uint8_t read_mmio(uint8_t page, uint8_t offset) {
    set_mmio_page(page);
    uint8_t value;
    __asm
        in a, (%[offset])
        ld l, a
    __endasm;
    return value;
}

// Запрос доступа к MMIO (системный вызов)
static inline uint8_t request_mmio_access(void) {
    uint8_t result;
    __asm
        ; Подготовка аргументов
        ld a, SYS_MMIO_ACCESS_REQUEST
        ld bc, 0x0001  ; флаг запроса на MMIO
        
        ; Вызов syscall через обертку
        out (PORT_SYSCALL), a
        
        ; Результат в A
        ld l, a
    __endasm;
    return result;
}

// ==============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ==============================================================================

// Проверить, находится ли адрес в оконной памяти
static inline int is_window_memory(uint32_t addr) {
    return (addr >= WB_MEMORY_BEGIN && addr <= WB_MEMORY_END);
}

// Проверить, находится ли адрес в MMIO пространстве
static inline int is_mmio_space(uint32_t addr) {
    return (addr >= MMIO_BEGIN && addr <= MMIO_END);
}

// Проверить, находится ли адрес в MMIO_LO
static inline int is_mmio_lo(uint32_t addr) {
    return (addr >= MMIO_LO_BEGIN && addr <= MMIO_LO_END);
}

// Проверить, находится ли адрес в MMIO_HI
static inline int is_mmio_hi(uint32_t addr) {
    return (addr >= MMIO_HI_BEGIN && addr <= MMIO_HI_END);
}

// Получить номер банка для логического адреса
static inline uint8_t get_bank_for_address(uint32_t logical_addr) {
    // Банк зависит от активного слота и окна
    uint8_t slot = get_current_slot();
    uint8_t window = (logical_addr >> 14) & 0x03;  // Определяем окно по адресу
    
    // Здесь должна быть логика получения текущего банка для slot/window
    // Это зависит от внутреннего состояния MMU
    return 0;  // Заглушка
}

#ifdef __cplusplus
}
#endif

#endif // MEMORY_ALESTE_H