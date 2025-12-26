#ifndef AL_MMU_H
#define AL_MMU_H

#include <stdint.h>

// ==============================================================================
// САМЫЙ ПРОСТОЙ ВАРИАНТ - ТОЛЬКО МАКРОСЫ ДЛЯ АССЕМБЛЕРА
// ==============================================================================

// Эти адреса определяются при линковке
extern void mmu_detect(void);      // 0xFD10
extern void mmu_install(void);     // 0xFD13  
extern void mmu_get_info(void);    // 0xFD16
extern void mmu_command(void);     // 0xFD19
extern void mmu_uninstall(void);   // 0xFD1C
extern void mmu_set_context(void); // 0xFD1F
extern void mmu_get_context(void); // 0xFD22
extern void mmu_map_page(void);    // 0xFD25

// ==============================================================================
// МАКРОСЫ ДЛЯ АССЕМБЛЕРА (основное использование)
// ==============================================================================

/*
Пример использования в ассемблере:

    ; Проверить MMU
    call mmu_detect
    
    ; Инициализировать
    call mmu_install
    
    ; Установить контекст
    ld hl, my_context
    call mmu_set_context
    
    ; Отобразить банк
    ld a, 2        ; окно 2
    ld b, 0x20     ; банк 0x20
    call mmu_map_page
*/

// ==============================================================================
// ВЕРСИЯ ДЛЯ SDCC (если нужна совместимость)
// ==============================================================================

#ifdef __SDCC

// Прототипы функций для SDCC
uint8_t mmu_detect_c(void);
void mmu_install_c(void);
void mmu_set_context_c(const uint8_t* ctx);
void mmu_map_page_c(uint8_t window, uint8_t page);
void mmu_get_context_c(uint8_t* buf);

// Инлайн-обёртки
static inline uint8_t mmu_detect_wrap(void) {
    return mmu_detect_c();
}

static inline void mmu_install_wrap(void) {
    mmu_install_c();
}

static inline void mmu_set_context_wrap(const uint8_t* ctx) {
    mmu_set_context_c(ctx);
}

static inline void mmu_map_page_wrap(uint8_t window, uint8_t page) {
    mmu_map_page_c(window, page);
}

static inline void mmu_get_context_wrap(uint8_t* buf) {
    mmu_get_context_c(buf);
}

// Переопределяем имена для удобства
#define mmu_detect    mmu_detect_wrap
#define mmu_install   mmu_install_wrap
#define mmu_set_context mmu_set_context_wrap
#define mmu_map_page  mmu_map_page_wrap
#define mmu_get_context mmu_get_context_wrap

#endif // __SDCC

// ==============================================================================
// ВСПОМОГАТЕЛЬНЫЕ КОНСТАНТЫ
// ==============================================================================

// Порты MMU
#define MMU_PORT_FC  0xFC
#define MMU_PORT_FD  0xFD  
#define MMU_PORT_FE  0xFE
#define MMU_PORT_FF  0xFF

// Команды для mmu_command()
#define MMU_CMD_RESET      0x00
#define MMU_CMD_GET_INFO   0x01
#define MMU_CMD_GET_STATS  0x02

// Специальные хэши
#define MMU_HASH_FORCE     0xFF  // Принудительное переключение
#define MMU_HASH_DEFAULT   0x00  // Дефолтный контекст

#endif // AL_MMU_H