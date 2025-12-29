#include "memory.h"

unsigned char memory[RAM_SIZE];

// ============================================================
// Ports
// ============================================================

// 1. Исправленная функция чтения памяти
uint8_t mem_read(Z80EX_CONTEXT *cpu, uint16_t addr, int m1_state, void *user_data) {
    // m1_state будет 1, если CPU читает опкод, и 0 в остальных случаях
    return memory[addr];
}

void mem_write(Z80EX_CONTEXT *cpu, uint16_t addr, uint8_t value, void *user_data) {
    // Имитируем реальное ПЗУ: запись разрешена только выше области ROM
    if (addr >= ROM_SIZE) {
        memory[addr] = value;
    }
}


// Обертка для дизассемблера (без m1_state)
uint8_t dasm_read_byte(uint16_t addr, void *user_data) {
    // В user_data мы обычно передаем указатель на память или контекст, 
    // но так как memory у нас глобальная, можем брать напрямую.
    return memory[addr]; 
}