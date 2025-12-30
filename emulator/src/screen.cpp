#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <z80ex_dasm.h>
#include <SDL2/SDL.h>
#include <string.h>


#include "screen.h"
#include "memory.h"
#include "debugger.h"
#include "palette.h"
#include "ports.h"
#include "minibuffer.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;

#define SDL_SCREEN_WIDTH  (640)
#define SDL_SCREEN_HEIGHT  (400)
uint32_t screen_buffer[SDL_SCREEN_WIDTH * SDL_SCREEN_HEIGHT];

#include "font.h"

// Вспомогательная функция для конвертации HEX строки в цвет
uint32_t hex_to_color(const char* hex_str) {
    if (!hex_str || strlen(hex_str) != 6) return 0;
    
    uint32_t color = 0;
    for (int i = 0; i < 6; i += 2) {
        char hex[3] = {hex_str[i], hex_str[i+1], '\0'};
        uint8_t value = (uint8_t)strtol(hex, NULL, 16);
        
        if (i == 0) color |= ((uint32_t)value << 16); // Red
        else if (i == 2) color |= ((uint32_t)value << 8);  // Green
        else color |= value; // Blue
    }
    return color;
}
void draw_char(int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 127) return;

    int idx = 8 * (c-32);

    for (int i = 0; i < 8; i++) {
        unsigned char line = font8x8[idx+ i];
        for (int j = 0; j < 8; j++) {
            if (line & (0x80 >> j)) {
                int px = x + j;
                int py = y + i;
                if (px >= 0 && px < SDL_SCREEN_WIDTH && py >= 0 && py < SDL_SCREEN_HEIGHT) {
                    screen_buffer[py * SDL_SCREEN_WIDTH + px] = color;
                }
            }
        }
    }
}

void draw_text(int x, int y, const char* str, uint32_t default_color) {

    int current_x = x;
    int current_y = y;
    uint32_t current_color = default_color;
    
    while (*str && *str != '\0') {
        // Обработка цветовых кодов в формате {c:RRGGBB}
        if (str[0] == '{' && str[1] == 'c') {
            // Проверяем формат {c} - сброс цвета
            if (str[2] == '}') {
         
                current_color = default_color;
                str += 3; // Пропускаем "{c}"
                continue;
            }
            // Проверяем формат {c:RRGGBB} - установка цвета
            else if (str[2] == ':' && strlen(str) >= 10 && str[9] == '}') {

                
                // Извлекаем HEX код цвета (6 символов)
                char hex_color[7] = {0};
                strncpy(hex_color, str + 3, 6); // Пропускаем "{c:"
                hex_color[6] = '\0';
                
                // Конвертируем HEX в uint32_t цвет (формат 0xRRGGBB)
                uint32_t color = 0;
                for (int i = 0; i < 6; i += 2) {
                    char hex[3] = {hex_color[i], hex_color[i+1], '\0'};
                    uint8_t value = (uint8_t)strtol(hex, NULL, 16);
                    
                    if (i == 0) color |= ((uint32_t)value << 16); // Red
                    else if (i == 2) color |= ((uint32_t)value << 8);  // Green
                    else color |= value; // Blue
                }
                
                current_color = color;

                str += 10; // Пропускаем "{c:RRGGBB}" (10 символов)
                continue;
            }
        }
        
        // Обработка перевода строки
        if (*str == '\n') {

            current_x = x;
            current_y += 16; // высота строки
            str++;
            continue;
        }
        
        // Игнорируем другие управляющие символы
        if (*str == '{') {
            // Пропускаем любые другие фигурные скобки (если есть другие коды)
            str++;
            continue;
        }
        
        // Рисуем символ
        draw_char(current_x, current_y, *str, current_color);
        current_x += 8;
        str++;
    }

}
// ============================================================
// SDL
// ============================================================

bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    
    // Размер окна (можно масштабировать)
    int window_width  = SDL_SCREEN_WIDTH  * 2;
    int window_height = SDL_SCREEN_HEIGHT * 2;
    
    window = SDL_CreateWindow("Z80 BIOS Debugger", 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED, 
                              window_width, window_height, 
                              SDL_WINDOW_SHOWN);
    if (!window) return false;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    
    // Важно: размер текстуры должен совпадать с размером экрана в пикселях!
    texture = SDL_CreateTexture(renderer, 
                               SDL_PIXELFORMAT_ARGB8888, 
                               SDL_TEXTUREACCESS_STREAMING, 
                               SDL_SCREEN_WIDTH,     // 640 или больше
                               SDL_SCREEN_HEIGHT);   // Достаточно для всех строк
    if (!texture) return false;
    
    return true;
}
void set_window_title(const char* title)
{
   SDL_SetWindowTitle(window, title);
}

const uint16_t palette[] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF   
};

void update_screen_16k1b(Z80EX_CONTEXT *cpu, uint8_t* memory, uint32_t start) {
    const int MODE_WIDTH = 640;
    const int MODE_HEIGHT = 200;
    const int BYTES_PER_LINE = MODE_WIDTH / 8;
    
    // Проверяем, что масштаб целый
    if (SDL_SCREEN_HEIGHT % MODE_HEIGHT != 0) {
        printf("Warning: vertical scale is not integer!\n");
    }
    
    int vertical_scale = SDL_SCREEN_HEIGHT / MODE_HEIGHT;
    
    for (int mode_y = 0; mode_y < MODE_HEIGHT; mode_y++) {
        for (int x_byte = 0; x_byte < BYTES_PER_LINE; x_byte++) {
            int src = start + x_byte + (mode_y * BYTES_PER_LINE);
            if (src >= 0xFFFFFF) break;
            
            uint8_t color = memory[src];

            // Копируем строку vertical_scale раз
            for (int scale_y = 0; scale_y < vertical_scale; scale_y++) {
                int screen_y = mode_y * vertical_scale + scale_y;
                if (screen_y >= SDL_SCREEN_HEIGHT) break;
                
                int screen_x = x_byte * 8;
                int dst_base = screen_y * SDL_SCREEN_WIDTH + screen_x;
                
                if (dst_base + 7 >= SDL_SCREEN_WIDTH * SDL_SCREEN_HEIGHT) continue;
                
                screen_buffer[dst_base + 0] = g_palette.translate_color_argb32((color >> 7) & 1);
                screen_buffer[dst_base + 1] = g_palette.translate_color_argb32((color >> 6) & 1);
                screen_buffer[dst_base + 2] = g_palette.translate_color_argb32((color >> 5) & 1);
                screen_buffer[dst_base + 3] = g_palette.translate_color_argb32((color >> 4) & 1);
                screen_buffer[dst_base + 4] = g_palette.translate_color_argb32((color >> 3) & 1);
                screen_buffer[dst_base + 5] = g_palette.translate_color_argb32((color >> 2) & 1);
                screen_buffer[dst_base + 6] = g_palette.translate_color_argb32((color >> 1) & 1);
                screen_buffer[dst_base + 7] = g_palette.translate_color_argb32(color & 1);
            }
        }
    }
}
// Глобальный буфер для текста
static char debug_display_buffer[2048];


// В minibuffer.c или screen.c
void minibuffer_draw(Minibuffer *mb, SDL_Renderer *renderer, SDL_Window *window) {
    if (!mb->visible) return;
    
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);
    
    // 1. Вычисляем позицию мини-буфера (низ экрана)
    // Примечание: win_h - это размер окна в пикселях, не SDL_SCREEN_HEIGHT!
    // Нужно конвертировать координаты
    float scale_x = (float)SDL_SCREEN_WIDTH / win_w;
    float scale_y = (float)SDL_SCREEN_HEIGHT / win_h;
    
    int minibuf_screen_y = SDL_SCREEN_HEIGHT - 40; // Фиксированная позиция в screen_buffer
    
    // 2. Рисуем фон мини-буфера в screen_buffer
    for (int y = minibuf_screen_y; y < SDL_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SDL_SCREEN_WIDTH; x++) {
            screen_buffer[y * SDL_SCREEN_WIDTH + x] = 0xFF303040; // Темно-синий
        }
    }
    
    // 3. Рисуем рамку
    for (int x = 0; x < SDL_SCREEN_WIDTH; x++) {
        if (minibuf_screen_y > 0) {
            screen_buffer[minibuf_screen_y * SDL_SCREEN_WIDTH + x] = 0xFF505078; // Верхняя рамка
        }
        if (SDL_SCREEN_HEIGHT - 1 >= 0) {
            screen_buffer[(SDL_SCREEN_HEIGHT - 1) * SDL_SCREEN_WIDTH + x] = 0xFF505078; // Нижняя рамка
        }
    }
    for (int y = minibuf_screen_y; y < SDL_SCREEN_HEIGHT; y++) {
        if (y < SDL_SCREEN_HEIGHT) {
            screen_buffer[y * SDL_SCREEN_WIDTH] = 0xFF505078; // Левая рамка
            screen_buffer[y * SDL_SCREEN_WIDTH + (SDL_SCREEN_WIDTH - 1)] = 0xFF505078; // Правая рамка
        }
    }
    
    // 4. Подготавливаем текст с курсором
    char display_buffer[512];
    
    // Мигающий курсор
    static uint32_t last_blink_time = 0;
    static bool cursor_visible = true;
    uint32_t current_time = SDL_GetTicks();
    
    if (current_time - last_blink_time > 500) {
        cursor_visible = !cursor_visible;
        last_blink_time = current_time;
    }
    
    // Собираем строку с курсором
    if (cursor_visible) {
        if (mb->cursor_pos < mb->length) {
            // Курсор внутри текста
            snprintf(display_buffer, sizeof(display_buffer), 
                    "%s%.*s{c:FFC864}|{c}%.*s",
                    mb->prompt,
                    mb->cursor_pos, mb->buffer,
                    mb->length - mb->cursor_pos, mb->buffer + mb->cursor_pos);
        } else {
            // Курсор в конце
            snprintf(display_buffer, sizeof(display_buffer), 
                    "%s%s{c:FFC864}|",
                    mb->prompt, mb->buffer);
        }
    } else {
        // Курсор невидим (мигание)
        snprintf(display_buffer, sizeof(display_buffer), 
                "%s%s",
                mb->prompt, mb->buffer);
    }
    
    // 5. Рисуем текст мини-буфера
    draw_text(10, minibuf_screen_y + 10, display_buffer, 0xFFFFFFFF);
    
    // 6. Рисуем режим [INS]/[OVR] справа
    char mode_str[20];
    snprintf(mode_str, sizeof(mode_str), "[%s]", mb->overwrite_mode ? "OVR" : "INS");
    draw_text(SDL_SCREEN_WIDTH - 60, minibuf_screen_y + 10, mode_str, 0xFFFFFF00);
    
    // 7. Выводим в консоль для отладки
    printf("\rMinibuffer: %s%s", mb->prompt, mb->buffer);
    if (cursor_visible) {
        printf(" [CURSOR]");
    }
    fflush(stdout);
}

// Главная функция update_screen
void update_screen(Z80EX_CONTEXT *cpu, uint8_t* memory, uint32_t start, EScreenMode mode, Debugger *dbg)
{
    if (!cpu || !memory) return;
    
    // 1. Очистка экрана
    memset(screen_buffer, 0, SDL_SCREEN_WIDTH * SDL_SCREEN_HEIGHT * sizeof(uint32_t));
    
    // 2. Графика из памяти Z80
    switch (mode) {
        case EScreenMode_Linear16k1b:
            update_screen_16k1b(cpu, memory, start);
            break;
        default:
            break;
    }
    
    // 3. Генерация текста отладчика
    static char debug_display_buffer[2048];
    debugger_render_to_buffer(dbg, cpu, memory, debug_display_buffer, sizeof(debug_display_buffer));
    
    // 4. Рендеринг текста на экран (но оставляем место для мини-буфера!)
    int x = 10, y = 10;
    char buffer_copy[2048];
    strncpy(buffer_copy, debug_display_buffer, sizeof(buffer_copy));
    
    char *line = strtok(buffer_copy, "\n");
    while (line && y < SDL_SCREEN_HEIGHT - 50) { // Оставляем 50 пикселей для мини-буфера!
        draw_text(x, y, line, 0xFFFFFFFF);
        y += 10;
        line = strtok(NULL, "\n");
    }
    
    // 5. Отрисовка мини-буфера (он рисует прямо в screen_buffer)
    extern Minibuffer minibuf; // Объявляем глобальную переменную
    if (minibuf.visible) {
        minibuffer_draw(&minibuf, renderer, window);
    }
    
    // 6. Обновляем SDL текстуру из screen_buffer
    SDL_UpdateTexture(texture, NULL, screen_buffer, SDL_SCREEN_WIDTH * sizeof(uint32_t));
    
    // 7. Очищаем рендерер и рисуем текстуру
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}