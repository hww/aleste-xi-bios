#include "ports.h"
#include <stdint.h>
#include <stdio.h>


// Инициализация при объявлении
mc6845_t g_crct = {
    .screenMode = EScreenMode_Linear16k1b,
    .screenPosition = 0
};

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
    return 0xFF;
}

// Запись в порт (наш виртуальный UART)
void port_write(Z80EX_CONTEXT *cpu, uint16_t port, uint8_t value, void *user_data) {
    if ((port & 0xFF) == UART_PORT) {
        putchar(value);
        fflush(stdout); // Чтобы текст появлялся сразу
    }
}