#ifndef CRTC_H
#define CRTC_H

#include <stdint.h>

#define BIOS_CRTC_SLOT 0xFD20

// Команды IOCTL
#define CRTC_CMD_WRITE 0
#define CRTC_CMD_READ  1
#define CRTC_CMD_VSYNC 2

// 1. Установка режима
// Используем fastcall, чтобы 'mode' попал сразу в L (а затем в A в драйвере)
#define crtc_set_mode(mode) \
    ((void (*)(uint8_t) __z88dk_fastcall) (BIOS_CRTC_SLOT + 12))(mode)

// 2. Установка адреса отображения
// Мы передаем uint16_t, где (High = Slot, Low = Page/Offset)
// Это идеально ложится в регистровую пару HL через fastcall
static inline void crtc_set_view_raw(uint16_t slot_page) __z88dk_fastcall {
    __asm
        jp BIOS_CRTC_SLOT + 15
    __endasm;
}

// Красивая обертка для удобства
#define crtc_set_view(slot, page) \
    crtc_set_view_raw(((uint16_t)(slot) << 8) | (uint8_t)(page))

// 3. IOCTL (Чтение/Запись/Синхронизация)
// Здесь параметры передаются через стек (стандарт SDCC), 
// поэтому в драйвере crtc_ioctl должен будет забрать их оттуда или 
// мы напишем небольшую прослойку.
uint8_t crtc_ioctl(uint8_t cmd, uint8_t reg, uint8_t val) {
    cmd; reg; val; // Чтобы компилятор не ругался
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)  ; cmd
        inc hl
        ld b, (hl)  ; reg
        inc hl
        ld c, (hl)  ; val
        call BIOS_CRTC_SLOT + 9
        ld l, a     ; возвращаем результат в L
    __endasm;
}

// Удобный макрос для ожидания луча
#define crtc_wait_vsync() crtc_ioctl(CRTC_CMD_VSYNC, 0, 0)

enum {
    CPC_MODE2 = 0, CPC_MODE1, CPC_MODE0,
    LX_16K_1B,     LX_16K_2B,  LX_16K_4B,
    LX_32K_2B,     LX_32K_4B,  LX_32K_8B,
    LX_64K_4B,     LX_64K_8B
};

#endif