#ifndef _BUFFER_PRINTER_H_
#define _BUFFER_PRINTER_H_

#include <stdarg.h>

// Структура для печати в буфер
typedef struct {
    char *buffer;     // Указатель на буфер
    int capacity;     // Максимальный размер буфера
    int position;     // Текущая позиция в буфере
} BufferPrinter;

// Инициализация
void bp_init(BufferPrinter *bp, char *buffer, int capacity);

// Сброс позиции в начало
void bp_reset(BufferPrinter *bp);

// Печать без цвета
void bp_print(BufferPrinter *bp, const char *format, ...);

// Печать с цветом
void bp_print_color(BufferPrinter *bp, const char *color, const char *format, ...);

// Печать с переводом строки
void bp_println(BufferPrinter *bp, const char *format, ...);

// Печать с цветом и переводом строки
void bp_println_color(BufferPrinter *bp, const char *color, const char *format, ...);

// Получить текущий контент
const char* bp_get_content(BufferPrinter *bp);

// Получить длину контента
int bp_get_length(BufferPrinter *bp);

#endif