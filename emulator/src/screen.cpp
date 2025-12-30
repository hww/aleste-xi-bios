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
    const char *original_str = str; // Сохраняем указатель на начало для отладки
    
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
    const int BYTES_PER_LINE = 80;
    
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
                
                screen_buffer[dst_base + 0] = palette[(color >> 7) & 1];
                screen_buffer[dst_base + 1] = palette[(color >> 6) & 1];
                screen_buffer[dst_base + 2] = palette[(color >> 5) & 1];
                screen_buffer[dst_base + 3] = palette[(color >> 4) & 1];
                screen_buffer[dst_base + 4] = palette[(color >> 3) & 1];
                screen_buffer[dst_base + 5] = palette[(color >> 2) & 1];
                screen_buffer[dst_base + 6] = palette[(color >> 1) & 1];
                screen_buffer[dst_base + 7] = palette[color & 1];
            }
        }
    }
}
// Глобальный буфер для текста
static char debug_display_buffer[2048];

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
    
    // 3. Генерация текста отладчика (используем новую функцию)
    debugger_render_to_buffer(dbg, cpu, memory, debug_display_buffer, sizeof(debug_display_buffer));
    
    // 4. Рендеринг текста на экран
    int x = 10, y = 10;
    char *line = strtok(debug_display_buffer, "\n");
    while (line && y < SDL_SCREEN_HEIGHT - 10) {
        draw_text(x, y, line, 0xFFFFFFFF);
        y += 10;
        line = strtok(NULL, "\n");
    }
    
    // 5. Обновление SDL
    SDL_UpdateTexture(texture, NULL, screen_buffer, SDL_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}