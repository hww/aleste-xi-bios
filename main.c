#include <stdint.h>
#include "drivers/mmu/mmu.h"

// Определяем адрес активного слота MMU в таблице прыжков
// Согласно нашему канону, это 0xFD00
#define MMU_VTABLE_ADDR 0xFD00

void main() {
    // 1. Инициализация системы драйверов
    // В реальности тут будет цикл по всем detect() и install()
    
    // 2. Пример использования MMU по канону
    // Создаем контекст для какого-то процесса или другого драйвера
    MmuContext system_map = {
        0x01, // Hash ID
        0x00, // Page 0
        0x01, // Page 1
        0x02, // Page 2
        0x03  // Page 3
    };

    // Вызываем метод установки контекста (смещение +12 в VTable)
    // Мы используем указатель на функцию по фиксированному адресу
    void (*mmu_set_context)(MmuContext*) = (void (*)(MmuContext*))(MMU_VTABLE_ADDR + 12);
    
    mmu_set_context(&system_map);

    while(1) {
        // Главный цикл супервизора
        __asm
            halt
        __endasm;
    }
}