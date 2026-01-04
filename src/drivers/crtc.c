#include "drivers/crtc.h"

// Импортируем метки напрямую из таблицы переходов драйвера CRTC
// В ассемблере они должны быть объявлены как PUBLIC _v_crtc_...
extern void _crtc_command;
extern void _crtc_set_mode;
extern void _crtc_set_view;
extern void _crtc_install;

// 2. Установка адреса отображения
void crtc_install() __naked {
    __asm
        jp __crtc_install
    __endasm;
}

// 1. Установка режима
void crtc_set_mode(crtc_mode_t mode) __z88dk_fastcall __naked {
    mode; 
    __asm
        ld a, l          ; fastcall: mode уже в L, переносим в A для BIOS
        jp __crtc_set_mode
    __endasm;
}

// 2. Установка адреса отображения
void crtc_set_view(uint8_t slot, uint8_t page) __naked {
    slot; page;
    __asm
        ld hl, #2        
        add hl, sp
        ld a, (hl)       ; a = slot (H)
        inc hl
        ld l, (hl)       ; l = page (L)
        ld h, a          ; теперь HL содержит (slot:page)
        jp __crtc_set_view
    __endasm;
}

// 3. Универсальные команды
uint8_t crtc_command(uint8_t cmd, uint8_t reg, uint8_t val) {
    cmd; reg; val;
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; cmd
        inc hl
        ld b, (hl)  ; reg
        inc hl
        ld c, (hl)  ; val
        call __crtc_command
        ld l, a     ; результат для Си в регистре L
    __endasm;
    return 0; 
}