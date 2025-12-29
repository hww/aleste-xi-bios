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

#define SCREEN_WIDTH  (640)
#define SCREEN_HEIGHT  (400)
uint32_t screen_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];

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
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    screen_buffer[py * SCREEN_WIDTH + px] = color;
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
    window = SDL_CreateWindow("Z80 BIOS Debugger", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
                                SCREEN_WIDTH, SCREEN_HEIGHT);
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
    int w = 80;
    int h = SCREEN_HEIGHT;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int srcY = y / 2;
            int src = start + x + srcY * w;
            if (src >= 0xFFFFFF) break;

            uint8_t color = memory[src];   

            int dst = (x * 8) + (y * (8 * w));
            screen_buffer[dst+0] =  palette[(color & 0x80) ? 1 : 0];
            screen_buffer[dst+1] =  palette[(color & 0x40) ? 1 : 0];
            screen_buffer[dst+2] =  palette[(color & 0x20) ? 1 : 0];
            screen_buffer[dst+3] =  palette[(color & 0x10) ? 1 : 0];
            screen_buffer[dst+4] =  palette[(color & 0x08) ? 1 : 0];
            screen_buffer[dst+5] =  palette[(color & 0x04) ? 1 : 0];
            screen_buffer[dst+6] =  palette[(color & 0x02) ? 1 : 0];
            screen_buffer[dst+7] =  palette[(color & 0x01) ? 1 : 0];
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
    memset(screen_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
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
    while (line && y < SCREEN_HEIGHT - 10) {
        draw_text(x, y, line, 0xFFFFFFFF);
        y += 10;
        line = strtok(NULL, "\n");
    }
    
    // 5. Обновление SDL
    SDL_UpdateTexture(texture, NULL, screen_buffer, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}