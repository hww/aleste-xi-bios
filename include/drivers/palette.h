#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

/* Базовый адрес вектора палитры (согласно ALIGN 32 в BIOS) */
#define PAL_VTABLE 0xFD40

/* IOCTL команды */
#define PAL_CMD_MODE     0
#define PAL_CMD_MODIFIER 1

/**
 * Установить один цвет
 * index: 0-255
 * color: 12 или 15-битное значение (RRRGGGBBB)
 */
static inline void pal_set_color(uint8_t index, uint16_t color) {
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)     ; index
        inc hl
        ld c, (hl)     ; color low
        inc hl
        ld b, (hl)     ; color high
        call PAL_VTABLE + 12
    __endasm;
    (void)index; (void)color;
}

/**
 * Установить цвет бордюра
 */
static inline void pal_set_border(uint16_t color) {
    __asm
        ld hl, #2
        add hl, sp
        ld c, (hl)     ; color low
        inc hl
        ld b, (hl)     ; color high
        call PAL_VTABLE + 15
    __endasm;
    (void)color;
}

/**
 * Массовая загрузка палитры (256 цветов)
 * buffer: указатель на массив из 256 uint16_t
 */
static inline void pal_load_all(const uint16_t* buffer) __z88dk_fastcall {
    __asm
        ld b, #0       ; Count = 256
        call PAL_VTABLE + 18
    __endasm;
    (void)buffer;
}

/**
 * Управление расширенными функциями (режимы, модификаторы)
 */
static inline void pal_ioctl(uint8_t cmd, uint8_t val) {
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)     ; cmd
        inc hl
        ld b, (hl)     ; val
        call PAL_VTABLE + 9
    __endasm;
    (void)cmd; (void)val;
}

#endif