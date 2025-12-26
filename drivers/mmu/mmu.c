/* Файл для совместимости с C компиляторами */
#include "mmu.h"

/* Эти функции реализованы в ассемблере в mmu.asm */
/* Здесь только обёртки для вызова */

uint8_t mmu_detect_c(void) {
    uint8_t result;
    __asm
        call _mmu_detect
        ld l, a
        ld a, l
    __endasm;
    return result;
}

void mmu_install_c(void) {
    __asm
        call _mmu_install
    __endasm;
}

void mmu_set_context_c(const uint8_t* ctx) {
    __asm
        ld hl, #_ctx
        call _mmu_set_context
    __endasm;
}

void mmu_map_page_c(uint8_t window, uint8_t page) {
    __asm
        ld a, 4(ix)    ; window
        ld b, 5(ix)    ; page  
        call _mmu_map_page
    __endasm;
}

void mmu_get_context_c(uint8_t* buf) {
    __asm
        ld hl, #_buf
        ld e, (hl)
        inc hl
        ld d, (hl)
        call _mmu_get_context
    __endasm;
}