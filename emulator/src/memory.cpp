#include <cassert>
#include "memory.h"
#include "mmu_native.h"
#include "globals.h"
#include "ports.h"

// ============================================================
// Ports
// ============================================================

uint8_t mem_read(Z80EX_CONTEXT *cpu, uint16_t addr, int m1_state, void *user_data) {
    // m1_state будет 1, если CPU читает опкод, и 0 в остальных случаях
    if (g_mmu.is_native_mode())
    {
        return g_mmu.process_cycle(addr, ECycle::MRD, 0x00);
    } else {
        return memory[addr];
    }
}

void mem_write(Z80EX_CONTEXT *cpu, uint16_t addr, uint8_t value, void *user_data) {
    // Имитируем реальное ПЗУ: запись разрешена только выше области ROM
    if (g_mmu.is_native_mode())
    {
        g_mmu.process_cycle(addr, ECycle::MWR, value);
    } else {
        memory[addr] = value;
    }
}

// Функция для чтения байта (для дизассемблера)
uint8_t dasm_read_byte(uint16_t addr, void *user_data)
{
    uint8_t dummy_data  = 0;
    if (g_mmu.is_native_mode())
    {
        return g_mmu.process_cycle(addr, ECycle::MRD, dummy_data);
    } else {
        return memory[addr];
    }
    return 0xFF;
}
uint32_t dasm_translate_address(uint16_t addr, ECycle cycle)
{
    if (g_mmu.is_native_mode())
    {
        return g_mmu.translate_address(addr, cycle);
    } else {
        return addr;
    }
}