#include <stdint.h>
#include "drivers/mmu.h"

void main(void) {
    while(1) {
        // Главный цикл супервизора
        __asm
            halt
        __endasm;
    }
}

