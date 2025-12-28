/**
 * @file mmu_aleste.h
 * @brief MMU driver for Aleste LX computer
 * @version 2.0
 */

#ifndef _MMU_H
#define _MMU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Базовые команды MMU
#define MMU_CMD_GET_TOTAL    0x10
#define MMU_CMD_GET_FREE     0x11
#define MMU_CMD_RESERVE      0x12
#define MMU_CMD_RELEASE      0x13

// Размер контекста
#define MMU_CTX_SIZE         5

// Функции драйвера
bool mmu_detect(void);
void mmu_install(void);
void mmu_set_context(const uint8_t* ctx);
void mmu_map_page(uint8_t window, uint8_t page);
void mmu_get_context(uint8_t* buf);

// Функции менеджера памяти (расширенные)
void mmu_mem_init(void);
uint8_t mmu_alloc_pages(uint8_t count);
void mmu_free_pages(uint8_t first_page, uint8_t count);
uint16_t mmu_get_free_count(void);

#ifdef __cplusplus
}
#endif

#endif /* _MMU_H */
