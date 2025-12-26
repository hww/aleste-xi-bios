#ifndef PORTS_ALESTE_H
#define PORTS_ALESTE_H

#include <stdint.h>

// ==============================================================================
// PORTS_ALESTE.H - Специфичные порты Aleste (дополнение к CPC)
// ==============================================================================

// MMU порты (только в Aleste)
#define MMU_PORT_FC        0xFC    // Окно 0000-3FFF
#define MMU_PORT_FD        0xFD    // Окно 4000-7FFF  
#define MMU_PORT_FE        0xFE    // Окно 8000-BFFF
#define MMU_PORT_FF        0xFF    // Окно C000-FFFF

// Переключение банков памяти
static inline void mmu_select_bank(uint8_t window, uint8_t bank) {
    switch (window) {
        case 0: __asm out (MMU_PORT_FC), a __endasm; break;
        case 1: __asm out (MMU_PORT_FD), a __endasm; break;
        case 2: __asm out (MMU_PORT_FE), a __endasm; break;
        case 3: __asm out (MMU_PORT_FF), a __endasm; break;
    }
}

#endif // PORTS_ALESTE_H