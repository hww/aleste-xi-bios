#include "buffer_printer.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


// Инициализация
void bp_init(BufferPrinter *bp, char *buffer, int capacity)
{
    bp->buffer = buffer;
    bp->capacity = capacity;
    bp->position = 0;
    if (capacity > 0) {
        bp->buffer[0] = '\0';
    }
}

// Сброс позиции в начало
void bp_reset(BufferPrinter *bp)
{
    bp->position = 0;
    if (bp->capacity > 0) {
        bp->buffer[0] = '\0';
    }
}

// Печать без цвета
void bp_print(BufferPrinter *bp, const char *format, ...)
{
    if (!bp || !bp->buffer || bp->position >= bp->capacity) return;
    
    va_list args;
    va_start(args, format);
    int written = vsnprintf(bp->buffer + bp->position, 
                           bp->capacity - bp->position, 
                           format, args);
    va_end(args);
    
    if (written > 0 && bp->position + written < bp->capacity) {
        bp->position += written;
    }
}

// Печать с цветом (формат: {c:RRGGBB})
void bp_print_color(BufferPrinter *bp, const char *color_code, const char *format, ...)
{
    if (!bp || !bp->buffer || bp->position >= bp->capacity) return;
    
    // Добавляем цвет в формате {c:RRGGBB}
    if (color_code) {
        // ПРОВЕРЯЕМ, что код цвета - это ровно 6 символов HEX (RRGGBB)
        if (strlen(color_code) == 6) {
            // Проверяем, что все символы - hex цифры
            int valid = 1;
            for (int i = 0; i < 6; i++) {
                char c = color_code[i];
                if (!((c >= '0' && c <= '9') || 
                      (c >= 'A' && c <= 'F') || 
                      (c >= 'a' && c <= 'f'))) {
                    valid = 0;
                    break;
                }
            }
            
            if (valid) {
                // Нужно 10 символов: {c:RRGGBB}
                if (bp->position + 10 < bp->capacity) {
                    bp->buffer[bp->position] = '{';
                    bp->buffer[bp->position + 1] = 'c';
                    bp->buffer[bp->position + 2] = ':';
                    strncpy(bp->buffer + bp->position + 3, color_code, 6);
                    bp->position += 9;
                    bp->buffer[bp->position] = '}';
                    bp->position++;
                }
            }
        }
    }
    
    // Добавляем текст
    va_list args;
    va_start(args, format);
    int written = vsnprintf(bp->buffer + bp->position, 
                           bp->capacity - bp->position, 
                           format, args);
    va_end(args);
    
    if (written > 0 && bp->position + written < bp->capacity) {
        bp->position += written;
    }
    
    // Добавляем сброс цвета в формате {c}
    const char *reset = "{c}";
    int reset_len = strlen(reset);
    if (bp->position + reset_len < bp->capacity) {
        strcpy(bp->buffer + bp->position, reset);
        bp->position += reset_len;
    }
}

// Печать с переводом строки
void bp_println(BufferPrinter *bp, const char *format, ...)
{
    if (!bp || !bp->buffer || bp->position >= bp->capacity - 1) return;
    
    va_list args;
    va_start(args, format);
    int written = vsnprintf(bp->buffer + bp->position, 
                           bp->capacity - bp->position, 
                           format, args);
    va_end(args);
    
    if (written < 0) written = 0;

    if (bp->position + written + 1 < bp->capacity) {
        bp->position += written;
        bp->buffer[bp->position] = '\n';
        bp->position++;
        bp->buffer[bp->position] = '\0';
    }
}

// Печать с цветом и переводом строки
void bp_println_color(BufferPrinter *bp, const char *color_code, const char *format, ...)
{
    if (!bp || !bp->buffer || bp->position >= bp->capacity) return;
    
    char temp_buffer[256];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
    va_end(args);
    
    if (written <= 0) return;
    
    // Извлекаем цвет из формата {c:RRGGBB} или берем как есть если это RRGGBB
    char actual_color[7] = {0};
    int color_added = 0;
    
    if (color_code) {
        // Если цвет в формате {c:RRGGBB}
        if (strlen(color_code) >= 10 && 
            color_code[0] == '{' && 
            color_code[1] == 'c' && 
            color_code[2] == ':' && 
            color_code[9] == '}') {
            // Извлекаем RRGGBB
            strncpy(actual_color, color_code + 3, 6);
            color_added = 10; // {c:RRGGBB}
        }
        // Если цвет в формате RRGGBB (6 символов)
        else if (strlen(color_code) == 6) {
            strncpy(actual_color, color_code, 6);
            color_added = 10; // Будем добавлять как {c:RRGGBB}
        }
    }
    
    int reset_len = color_added ? 3 : 0; // {c}
    int total_len = color_added + written + reset_len + 1; // +1 для \n
    
    if (bp->position + total_len >= bp->capacity) return;
    
    // Добавляем цвет
    if (color_added && actual_color[0]) {
        bp->buffer[bp->position] = '{';
        bp->buffer[bp->position + 1] = 'c';
        bp->buffer[bp->position + 2] = ':';
        strncpy(bp->buffer + bp->position + 3, actual_color, 6);
        bp->position += 9;
        bp->buffer[bp->position] = '}';
        bp->position++;
    }
    
    // Добавляем текст
    strcpy(bp->buffer + bp->position, temp_buffer);
    bp->position += written;
    
    // Сбрасываем цвет
    if (color_added) {
        strcpy(bp->buffer + bp->position, "{c}");
        bp->position += 3;
    }
    
    // Добавляем перевод строки
    bp->buffer[bp->position] = '\n';
    bp->position++;
    bp->buffer[bp->position] = '\0';
}

// Получить текущий контент
const char* bp_get_content(BufferPrinter *bp)
{
    if (!bp || !bp->buffer) return "";
    return bp->buffer;
}

// Получить длину контента
int bp_get_length(BufferPrinter *bp)
{
    if (!bp) return 0;
    return bp->position;
}