/**
 * @file palette.c
 * @brief Color Palette Driver C Interface Implementation
 * 
 * Преобразует вызовы из C-стека в регистры Z80
 * и вызывает соответствующие ассемблерные функции.
 */

#include "drivers/palette.h"

// ============================================================================
// ВНЕШНИЕ СИМВОЛЫ ИЗ АССЕМБЛЕРА
// ============================================================================

/* Полиморфная VTable палитры (определена в palette.asm) */
extern void _palette_detect(void);      /* +0  */
extern void _palette_install(void);     /* +3  */
extern void _palette_get_info(void);    /* +6  */
extern void _palette_command(void);     /* +9  */
extern void _palette_uninstall(void);   /* +12 */
extern void _palette_set_color(void);   /* +15 */
extern void _palette_set_border(void);  /* +18 */
extern void _palette_load(void);        /* +21 */

/* Функции управления полиморфизмом */
extern void _palette_activate_driver(void);
extern void _palette_get_current_vtable(void);

/* VTable конкретных драйверов */
extern void _palette_12b_vtable(void);

// ============================================================================
// РЕАЛИЗАЦИЯ ОБЯЗАТЕЛЬНЫХ МЕТОДОВ
// ============================================================================

/* palette_detect - проверка наличия палитры */
bool palette_detect(void) __z88dk_fastcall  __naked {
    __asm
        ; fastcall: HL уже содержит пустой аргумент
         jp __palette_detect
        ; Возврат: A=1 если найдено, A=0 если нет
        ; Z88dk автоматически преобразует A в bool
    __endasm;
}

/* palette_install - инициализация палитры */
uint8_t palette_install(void) __z88dk_fastcall  __naked {
    __asm
         jp __palette_install
        ; Возврат: A=код ошибки (0 при успехе)
    __endasm;
}

/* palette_get_info - информация о драйвере */
void* palette_get_info(void) __z88dk_fastcall  __naked {
    __asm
         jp __palette_get_info
        ; Возврат: HL = адрес DriverInfo
    __endasm;
}

/* palette_command - универсальная команда */
uint8_t palette_command(uint8_t cmd, uint16_t param) __naked {
    cmd; param; /* Подавляем warnings о неиспользованных параметрах */
    __asm
        ld hl, #2        ; Пропускаем адрес возврата
        add hl, sp
        
        ld a, (hl)       ; A = cmd
        inc hl
        ld e, (hl)       ; E = param low
        inc hl
        ld d, (hl)       ; D = param high
        
        ld l, e          ; HL = param (16-bit)
        ld h, d
        
        ; A = cmd, HL = param
        call __palette_command
        
        ; Возвращаем результат в L (z88dk convention)
        ld l, a
        ret
    __endasm;
}

/* palette_uninstall - деинициализация */
uint8_t palette_uninstall(void) __z88dk_fastcall  __naked {
    __asm
         jp __palette_uninstall
        ; Возврат: A=код ошибки
    __endasm;
}

// ============================================================================
// РЕАЛИЗАЦИЯ СПЕЦИФИЧНЫХ МЕТОДОВ
// ============================================================================

/* palette_set_color_12b - установка цвета в 12-bit режиме */
uint8_t palette_set_color_12b(uint8_t index, palette_color_12b_t color) __naked {
    index; color;
    __asm
        ld hl, #2
        add hl, sp
        
        ; index уже в правильном формате (8-bit)
        ; color нужно разложить на BC
        ld a, (hl)       ; A = index
        inc hl
        ld c, (hl)       ; C = color low
        inc hl
        ld b, (hl)       ; B = color high
        
        ; Теперь: A = index, BC = color
        call __palette_set_color
        
        ld l, a          ; Возвращаем код ошибки в L
        ret
    __endasm;
}

/* palette_set_color_cpc - установка цвета в CPC режиме */
uint8_t palette_set_color_cpc(uint8_t index, palette_color_cpc_t color) __naked {
    index; color;
    __asm
        ; CPC цвет 6-битный, но палитра ожидает 12-битный
        ; Нужно преобразовать
        ld hl, #2
        add hl, sp
        
        ld a, (hl)       ; A = index
        inc hl
        ld e, (hl)       ; E = cpc_color (6-bit)
        
        ; Преобразуем CPC 6-bit -> 12-bit
        ; RRGGBB -> RRRRGGGGBBBB
        ld d, 0
        ld h, d
        
        ; Красная компонента (2 бита -> 4 бита)
        ld a, e
        and 0x30         ; Извлекаем RR
        rrca
        rrca
        rrca
        rrca             ; Сдвигаем в старшие биты 12-битного цвета
        ld b, a          ; Сохраняем в B
        
        ; Зеленая компонента (2 бита -> 4 бита)
        ld a, e
        and 0x0C         ; Извлекаем GG
        rrca
        rrca             ; Сдвигаем на место
        or b             ; Объединяем с красной
        ld b, a          ; Сохраняем в B
        
        ; Синяя компонента (2 бита -> 4 бита)
        ld a, e
        and 0x03         ; Извлекаем BB
        or b             ; Объединяем
        ld b, a          ; B теперь содержит 12-битный цвет
        
        ; Индекс уже в A, BC содержит цвет
        ; Но нужно восстановить индекс из стека
        ld hl, #2
        add hl, sp
        ld a, (hl)       ; A = индекс
        
        call __palette_set_color
        ld l, a
        ret
    __endasm;
}

/* palette_set_border - установка цвета бордюра */
uint8_t palette_set_border(palette_color_12b_t color) __z88dk_fastcall  __naked {
    __asm
        ; fastcall: цвет уже в HL
        ld c, l
        ld b, h          ; BC = цвет
        
         jp __palette_set_border
        ; Возврат: A=код ошибки
    __endasm;
}

/* palette_load_colors - загрузка массива цветов */
uint8_t palette_load_colors(uint8_t start_index, const void* colors, uint8_t count) __naked {
    start_index; colors; count;
    __asm
        ld hl, #2
        add hl, sp
        
        ; Получаем start_index для _palette_load
        ld c, (hl)       ; C = start_index
        inc hl
        
        ; Получаем указатель на массив
        ld e, (hl)       ; E = colors low
        inc hl
        ld d, (hl)       ; D = colors high
        inc hl
        
        ; Получаем количество
        ld b, (hl)       ; B = count
        
        ; Теперь: C = start_index, DE = colors, B = count
        ; _palette_load ожидает: HL=colors, B=count, C=start_index
        ld h, d
        ld l, e          ; HL = colors
        
        call __palette_load
        
        ld l, a          ; Возвращаем код ошибки
        ret
    __endasm;
}

// ============================================================================
// УПРАВЛЕНИЕ ПОЛИМОРФИЗМОМ
// ============================================================================

/* palette_activate_12bit - активация 12-bit драйвера */
uint8_t palette_activate_12bit(void) __z88dk_fastcall  __naked {
    __asm
        ld hl, __palette_12b_vtable
        call __palette_activate_driver
        ; _palette_activate_driver возвращает код в A
        ld l, a
        ret
    __endasm;
}

/* palette_activate_cpc - активация CPC драйвера */
uint8_t palette_activate_cpc(void) __z88dk_fastcall  __naked {
    __asm
        ld hl, _palette_cpc_vtable
        call __palette_activate_driver
        ld l, a
        ret
    __endasm;
}

/* palette_activate_msx - активация MSX драйвера */
uint8_t palette_activate_msx(void) __z88dk_fastcall  __naked {
    __asm
        ld hl, _palette_msx_vtable
        call __palette_activate_driver
        ld l, a
        ret
    __endasm;
}

/* palette_get_current_vtable - получить текущую VTable */
void* palette_get_current_vtable(void) __z88dk_fastcall  __naked {
    __asm
         jp __palette_get_current_vtable
        ; Возврат: HL = адрес VTable
    __endasm;
}

// ============================================================================
// ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ C-ИНТЕРФЕЙСА
// ============================================================================

/**
 * @brief Установить цвет с проверкой режима
 * 
 * Автоматически выбирает правильную функцию в зависимости
 * от текущего активного драйвера.
 */
uint8_t palette_set_color_auto(uint8_t index, uint32_t color) {
    // TODO: определить текущий активный драйвер и преобразовать цвет
    // Это можно сделать через palette_get_current_vtable()
    // и сравнение с известными адресами VTable
    
    // Пока просто используем 12-bit
    return palette_set_color_12b(index, (palette_color_12b_t)color);
}

/**
 * @brief Заполнить палитру градиентом
 * 
 * @param start_index Начальный индекс
 * @param start_color Начальный цвет
 * @param end_color Конечный цвет
 * @param count Количество цветов в градиенте
 */
uint8_t palette_fill_gradient(uint8_t start_index, 
                              palette_color_12b_t start_color,
                              palette_color_12b_t end_color,
                              uint8_t count) {
    
    if (count == 0) return PAL_ERR_INVALID_PARAM;
    if (count == 1) {
        return palette_set_color_12b(start_index, start_color);
    }
    
    // Вычисляем шаги для каждой компоненты
    int16_t r1 = (start_color >> 8) & 0x0F;
    int16_t g1 = (start_color >> 4) & 0x0F;
    int16_t b1 = start_color & 0x0F;
    
    int16_t r2 = (end_color >> 8) & 0x0F;
    int16_t g2 = (end_color >> 4) & 0x0F;
    int16_t b2 = end_color & 0x0F;
    
    int16_t dr = r2 - r1;
    int16_t dg = g2 - g1;
    int16_t db = b2 - b1;
    
    // Создаем буфер для градиента
    palette_color_12b_t gradient[256];
    
    for (uint8_t i = 0; i < count; i++) {
        int16_t r = r1 + (dr * i) / (count - 1);
        int16_t g = g1 + (dg * i) / (count - 1);
        int16_t b = b1 + (db * i) / (count - 1);
        
        // Ограничиваем значения 0-15
        if (r < 0) r = 0;
        if (r > 15) r = 15;
        if (g < 0) g = 0;
        if (g > 15) g = 15;
        if (b < 0) b = 0;
        if (b > 15) b = 15;
        
        gradient[i] = palette_rgb_12bit(r, g, b);
    }
    
    return palette_load_colors(start_index, gradient, count);
}