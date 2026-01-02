#include <stdint.h>
#include "drivers/mmu.h"

void main(void) {
    mmu_install();
    while(1) {
        // Главный цикл супервизора
        __asm
            halt
        __endasm;
    }
}

