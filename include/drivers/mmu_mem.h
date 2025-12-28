#ifndef MMU_MEM_H
#define MMU_MEM_H

#include <stdint.h>

// --- Точки входа VTable менеджера ---
extern void _mmu_mgr_detect;
extern void _mmu_mgr_install;
extern void _mmu_mgr_alloc;
extern void _mmu_mgr_free;
extern void _mmu_mgr_stats;

// --- Публичный Си-интерфейс ---

void     mmu_mem_init(void);    // вызывает install
uint8_t  mmu_alloc(uint8_t count) __z88dk_fastcall;
void     mmu_free(uint8_t start, uint8_t count);

// Расширенная статистика
uint16_t mmu_get_free_pages(void);
uint16_t mmu_get_alloc_count(void); // сколько раз вызывали alloc

#endif