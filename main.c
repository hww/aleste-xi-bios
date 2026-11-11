#include <stdint.h>
#include "drivers/mmu.h"
#include "drivers/crtc.h"
#include "drivers/vbuf.h"
#include "drivers/palette.h"

void main(void) {
    mmu_install();
    crtc_install();

    vbuf_activate_driver(&_drv_16k1b_vtable);
    vbuf_init();
    vbuf_clear(0x00);
    palette_activate_12bit();
    palette_install();
    palette_set_color_12b(0, 0x0026);   /* fondo: azul oscuro */
    palette_set_color_12b(1, 0x0FFF);   /* tinta: blanco */
    {
        static const char msg[] = "ALESTE LX BIOS - MiSTer";
        uint16_t x = 8;
        const char* p;
        for (p = msg; *p; p++, x += 8)
            vbuf_draw_char(&_scr_font_data, *p, x, 8);
    }
    while(1) {
        // Главный цикл супервизора
        __asm
            halt
        __endasm;
    }
}

