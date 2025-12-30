#ifndef MMU_H
#define MMU_H

#include <stdint.h>

// Контекст (соответствует структуре в ассемблере)
typedef struct {
    uint8_t hash;
    uint8_t bank[4];
} mmu_ctx_t;

// --- Публичный Си-интерфейс ---

uint8_t  mmu_detect(void);
void     mmu_install(void);
void* mmu_get_info(void);
uint16_t mmu_command(uint8_t cmd, uint16_t param);
void     mmu_uninstall(void);

// Специфичные методы
void mmu_set_ctx(const mmu_ctx_t* ctx) __z88dk_fastcall __naked;
void mmu_get_ctx(mmu_ctx_t* ctx) __z88dk_fastcall __naked;
void mmu_map(uint8_t window, uint8_t page) __naked;

// Прямое управление прерываниями внутри MMU (для критических секций)
void mmu_lock(void);
void mmu_unlock(void);

#endif