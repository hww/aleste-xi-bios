
#include <stdint.h>
#include <stdio.h>
#include "ports.h"
#include "mmu_native.h"
#include "globals.h"
#include "palette.h"


void ports_reset()
{
    g_mmu.reset();
    g_palette.reset();
}

// ============================================================
// Ints
// ============================================================

// 2. Добавляем заглушку для чтения вектора прерывания
uint8_t int_read(Z80EX_CONTEXT *cpu, void *user_data) {
    return 0xFF; // Стандартное поведение для пустой шины данных
}

// ============================================================
// Ports
// ============================================================

// Чтение порта (пока пусто)
uint8_t port_read(Z80EX_CONTEXT *cpu, uint16_t port, void *user_data) {
    if (g_mmu.check_port(port, ECycle::IORD)) {
        g_mmu.process_cycle(port, ECycle::IORD, 0x00);
    } else if (g_palette.check_port(port, ECycle::IORD)) {
        g_palette.process_cycle(port, ECycle::IORD, 0x00);
    }
    return 0xFF;
}

// Запись в порт (наш виртуальный UART)
void port_write(Z80EX_CONTEXT *cpu, uint16_t port, uint8_t value, void *user_data) {
    if (g_mmu.check_port(port, ECycle::IOWR)) {
        g_mmu.process_cycle(port, ECycle::IOWR, value);
    } else if (g_palette.check_port(port, ECycle::IOWR)) {
        g_palette.process_cycle(port, ECycle::IOWR, value);
    }
    //if ((port & 0xFF) == UART_PORT) {
    //    putchar(value);
    //    fflush(stdout); // Чтобы текст появлялся сразу
    //}
}



