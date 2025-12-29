#include "drivers/mmu.h"


// --- Точки входа VTable (для линкера) ---
extern void _mmu_detect;
extern void _mmu_install;
extern void _mmu_get_info;
extern void _mmu_command;
extern void _mmu_uninstall;
extern void _mmu_set_context;
extern void _mmu_get_context;
extern void _mmu_map_page;
extern void _mmu_lock; 
extern void _mmu_unlock;


uint8_t mmu_detect(void) {
    __asm
        call __mmu_detect
        ld l, a
    __endasm;
    return 0; 
}

void mmu_install(void) { 
    __asm 
        call __mmu_install 
    __endasm; 
}

void* mmu_get_info(void) {
    __asm 
        call __mmu_get_info 
    __endasm;
    return 0; 
}

uint16_t mmu_command(uint8_t cmd, uint16_t param) {
    cmd; param;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; cmd
        inc hl
        ld e, (hl)  ; param L
        inc hl
        ld d, (hl)  ; param H
        call __mmu_command
    __endasm;
    return 0;
}

void mmu_set_ctx(const mmu_ctx_t* ctx) __z88dk_fastcall __naked {
    ctx; 
    __asm 
        jp __mmu_set_context 
    __endasm;
}

void mmu_get_ctx(mmu_ctx_t* ctx) __z88dk_fastcall __naked {
    ctx; 
    __asm 
        ex de, hl 
        jp __mmu_get_context 
    __endasm;
}

void mmu_map(uint8_t window, uint8_t page) __naked {
    window; page;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)
        inc hl
        ld b, (hl)
        jp __mmu_map_page
    __endasm;
}

void mmu_lock(void) {
    __asm
        call __mmu_lock
    __endasm;
}

void mmu_unlock(void) {
    __asm
        call __mmu_unlock
    __endasm;
}