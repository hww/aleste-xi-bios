/* hw.c - acceso al hardware del Aleste LX para GPUDEMO.COM (ver hw.h)
 * Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx. */
#include <stdint.h>
#include "hw.h"

__sfr __at 0xF1 io_page;
__sfr __at 0xFA io_super_slot;
/* pagina 0: PIC */
__sfr __at 0x05 io_pic_pend;
__sfr __at 0x07 io_pic_clr;
/* pagina 2: paleta y CRTC */
__sfr __at 0x00 io_pal_idx;
__sfr __at 0x01 io_pal_lo;
__sfr __at 0x02 io_pal_hi;
__sfr __at 0x20 io_crtc_idx;
__sfr __at 0x21 io_crtc_dat;
/* pagina 3: LX-GPU */
__sfr __at 0x00 io_gpu_a0;
__sfr __at 0x01 io_gpu_a1;
__sfr __at 0x02 io_gpu_a2;
__sfr __at 0x03 io_gpu_cmd;
__sfr __at 0x04 io_gpu_status;

uint8_t port_in(uint8_t port) __z88dk_fastcall __naked {
    (void)port;
    __asm
        ld c, l
        in l, (c)
        ret
    __endasm;
}

/* IN r,(C) / INC C: las cuatro lecturas dentro de la misma copia */
uint32_t timer_us(void) __naked {
    __asm
        ld c, 0xF4
        in l, (c)
        inc c
        in h, (c)
        inc c
        in e, (c)
        inc c
        in d, (c)
        ret
    __endasm;
}

uint32_t phys(const void* p) {
    uint16_t a = (uint16_t)p;
    uint8_t win = (uint8_t)(a >> 14);
    uint8_t slot = (uint8_t)(io_super_slot >> (win * 2)) & 3;
    uint8_t bank = port_in((uint8_t)(0xE0 + slot * 4 + win));
    return ((uint32_t)slot << 22) | ((uint32_t)bank << 14) | (a & 0x3FFF);
}

void vblank_clear(void) {
    io_page = 0;
    io_pic_clr = 0x04;
}

uint8_t vblank_pending(void) {
    io_page = 0;
    return io_pic_pend & 0x04;
}

void vblank_wait(void) {
    io_page = 0;
    io_pic_clr = 0x04;
    while (!(io_pic_pend & 0x04)) ;
}

void crtc_set(uint8_t reg, uint8_t v) {
    io_page = 2;
    io_crtc_idx = reg;
    io_crtc_dat = v;
}

uint8_t crtc_get(uint8_t reg) {
    io_page = 2;
    io_crtc_idx = reg;
    return io_crtc_dat;
}

void pal_set(uint8_t idx, uint16_t rgb) {
    io_page = 2;
    io_pal_idx = idx;
    io_pal_lo = (uint8_t)rgb;
    io_pal_hi = (uint8_t)(rgb >> 8);
}

void gpu_start(uint32_t pk) {
    io_page = 3;
    io_gpu_a0 = (uint8_t)pk;
    io_gpu_a1 = (uint8_t)(pk >> 8);
    io_gpu_a2 = (uint8_t)(pk >> 16);
    io_gpu_cmd = 1;
}

uint8_t gpu_status(void) {
    io_page = 3;
    return io_gpu_status;
}

uint8_t gpu_wait(void) {
    uint8_t s;
    io_page = 3;
    do s = io_gpu_status; while (!(s & 0x80));
    return s;
}
