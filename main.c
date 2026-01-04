#include <stdint.h>
#include "drivers/mmu.h"
#include "drivers/crtc.h"
#include "drivers/vbuf.h"

void main(void) {
    mmu_install();
    crtc_install();

    vbuf_activate_driver(&_drv_16k1b_vtable);
    vbuf_init();
    vbuf_clear(0x00);
    vbuf_draw_char(&_scr_font_data, 'A', 0, 0);
    while(1) {
        // Главный цикл супервизора
        __asm
            halt
        __endasm;
    }
}

