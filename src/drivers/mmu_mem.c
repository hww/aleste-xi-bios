#include "drivers/mmu_mem.h"

extern void _mmu_mgr_install;
extern void _mmu_mgr_alloc;
extern void _mmu_mgr_free;
extern void _mmu_mgr_stats;

void mmu_mem_init(void) { 
    __asm 
        call __mmu_mgr_install 
    __endasm; 
}



uint8_t mmu_alloc(uint8_t count) __z88dk_fastcall __naked {
    count;
    __asm
        ld a, l
        call __mmu_mgr_alloc
        ld l, a
        ret
    __endasm;
}

void mmu_free(uint8_t start, uint8_t count) {
    start; count;
    __asm
        ld hl, #2 
        add hl, sp
        ld a, (hl) 
        inc hl 
        ld b, (hl)
        call __mmu_mgr_free
    __endasm;
}

uint16_t mmu_get_free_pages(void) {
    __asm 
        call __mmu_mgr_stats 
    __endasm;
    return 0;
}

