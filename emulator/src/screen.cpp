#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <z80ex_dasm.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <sstream>  

#include "screen.h"
#include "memory.h"
#include "debugger.h"
#include "palette.h"
#include "ports.h"
#include "globals.h"
#include "utils/minibuffer.h"
#include "utils/ibuffer.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;

uint32_t screen_buffer[SDL_SCREEN_WIDTH * SDL_SCREEN_HEIGHT];

#include "font.h"

static bool blink_flag = true;
// В debugger_render.cpp добавьте:
uint32_t alpha_blend(uint32_t background, uint32_t foreground) {
    // Извлекаем компоненты фона
    uint8_t bg_a = (background >> 24) & 0xFF;
    uint8_t bg_r = (background >> 16) & 0xFF;
    uint8_t bg_g = (background >> 8) & 0xFF;
    uint8_t bg_b = background & 0xFF;
    
    // Извлекаем компоненты переднего плана
    uint8_t fg_a = (foreground >> 24) & 0xFF;
    uint8_t fg_r = (foreground >> 16) & 0xFF;
    uint8_t fg_g = (foreground >> 8) & 0xFF;
    uint8_t fg_b = foreground & 0xFF;
    
    // Если передний план полностью прозрачный
    if (fg_a == 0) return background;
    
    // Если передний план полностью непрозрачный
    if (fg_a == 255) return foreground;
    
    // Alpha blending формула
    float alpha = fg_a / 255.0f;
    float inv_alpha = 1.0f - alpha;
    
    uint8_t r = static_cast<uint8_t>(fg_r * alpha + bg_r * inv_alpha);
    uint8_t g = static_cast<uint8_t>(fg_g * alpha + bg_g * inv_alpha);
    uint8_t b = static_cast<uint8_t>(fg_b * alpha + bg_b * inv_alpha);
    
    // Результирующая альфа (можно использовать максимальную)
    uint8_t a = std::max(bg_a, fg_a);
    
    return (a << 24) | (r << 16) | (g << 8) | b;
}
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

void draw_char(int x, int y, char c, uint32_t color, EStyle style) {
    if (c < 32 || c > 127) return;

    int idx = 8 * (c-32);

    for (int i = 0; i < 8; i++) {
        unsigned char line = font8x8[idx+ i];
        
        switch (style)
        {
        case EStyle::Inverse:
            line =(0xFF-line);
            break;
        case EStyle::InverseBlink:
            line = blink_flag ? (0xFF-line) : line;
            break;
        default:
            break;
        }

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



void minibuffer_draw(Minibuffer &mb, SDL_Renderer *renderer, SDL_Window *window) {
    if (!mb.is_visible()) return;

    // 1. Вычисляем позицию мини-буфера (низ экрана)
    int minibuf_screen_y = MINIBBUF_Y; 
    
    // 2. Рисуем фон мини-буфера в screen_buffer
    for (int y = minibuf_screen_y; y < SDL_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SDL_SCREEN_WIDTH; x++) {
            screen_buffer[y * SDL_SCREEN_WIDTH + x] = alpha_blend(screen_buffer[y * SDL_SCREEN_WIDTH + x]  , MINIBBUF_BG_COLOR); // Темно-синий
        }
    }
    
    // 3. Рисуем рамку
    // Верхняя граница
    for (int x = 0; x < SDL_SCREEN_WIDTH; x++) {
        if (minibuf_screen_y > 0 && minibuf_screen_y < SDL_SCREEN_HEIGHT) {
            screen_buffer[minibuf_screen_y * SDL_SCREEN_WIDTH + x] = MINIBBUF_BORDER_COLOR;
        }
    }
    
    // Нижняя граница (последняя строка экрана)
    int bottom_line = SDL_SCREEN_HEIGHT - 1;
    if (bottom_line >= 0 && bottom_line < SDL_SCREEN_HEIGHT) {
        for (int x = 0; x < SDL_SCREEN_WIDTH; x++) {
            screen_buffer[bottom_line * SDL_SCREEN_WIDTH + x] = MINIBBUF_BORDER_COLOR;
        }
    }
    
    // Боковые границы
    for (int y = minibuf_screen_y; y < SDL_SCREEN_HEIGHT; y++) {
        if (y >= 0 && y < SDL_SCREEN_HEIGHT) {
            // Левая граница
            if (0 < SDL_SCREEN_WIDTH) {
                screen_buffer[y * SDL_SCREEN_WIDTH] = MINIBBUF_BORDER_COLOR;
            }
            // Правая граница
            if (SDL_SCREEN_WIDTH - 1 >= 0) {
                screen_buffer[y * SDL_SCREEN_WIDTH + (SDL_SCREEN_WIDTH - 1)] = MINIBBUF_BORDER_COLOR;
            }
        }
    }
    
    // 4. Подготавливаем текст с курсором
    char display_buffer[512];
    
    // Мигающий курсор (используем статическую переменную для мигания)
    static uint32_t last_blink_time = 0;
    static bool blink_flag = true;  // Локальная переменная для мигания
    
    uint32_t current_time = SDL_GetTicks();
    
    if (current_time - last_blink_time > 500) {
        blink_flag = !blink_flag;
        last_blink_time = current_time;
    }
    
    // Получаем данные из класса Minibuffer
    const std::string& buffer = mb.get_text();
    const std::string& prompt = mb.get_prompt();
    int cursor_pos = mb.get_cursor_pos();
    int length = buffer.length();
    
    // Собираем строку с курсором
    if (blink_flag) {
        if (cursor_pos < length) {
            // Курсор внутри текста
            snprintf(display_buffer, sizeof(display_buffer), 
                    "%s%.*s{c:FFC864}|{c}%.*s",
                    prompt.c_str(),
                    cursor_pos, buffer.c_str(),
                    length - cursor_pos, buffer.c_str() + cursor_pos);
        } else {
            // Курсор в конце
            snprintf(display_buffer, sizeof(display_buffer), 
                    "%s%s{c:FFC864}|",
                    prompt.c_str(), buffer.c_str());
        }
    } else {
        // Курсор невидим (мигание)
        if (cursor_pos < length) {
            // Курсор внутри текста
            snprintf(display_buffer, sizeof(display_buffer), 
                    "%s%.*s{c:FFC864}.{c}%.*s",
                    prompt.c_str(),
                    cursor_pos, buffer.c_str(),
                    length - cursor_pos, buffer.c_str() + cursor_pos);
        } else {
            // Курсор в конце
            snprintf(display_buffer, sizeof(display_buffer), 
                    "%s%s{c:FFC864}.",
                    prompt.c_str(), buffer.c_str());
        }
    }
    
    // 5. Рисуем текст мини-буфера
    draw_text(MINIBBUF_MARGIN_X, minibuf_screen_y + MINIBBUF_MARGIN_Y, display_buffer, MINIBBUF_TEXT_COLOR);
    
    // 6. Рисуем режим [INS]/[OVR] справа
    char mode_str[20];
    snprintf(mode_str, sizeof(mode_str), "[%s]", mb.is_overwrite_mode() ? "OVR" : "INS");
    draw_text(SDL_SCREEN_WIDTH - 60, minibuf_screen_y + MINIBBUF_MARGIN_Y, mode_str, MINIBBUF_TEXT_COLOR);
}


void render_ibuffer(IBuffer& buffer, int x, int y, int width, int height, int margin) {
    // 1. Проверяем границы
    if (x >= SDL_SCREEN_WIDTH || y >= SDL_SCREEN_HEIGHT || 
        x + width <= 0 || y + height <= 0) {
        return;
    }

    // 2. Рисуем фон
    for (int buf_y = y; buf_y < y + height && buf_y < SDL_SCREEN_HEIGHT; buf_y++) {
        if (buf_y >= 0) {
            for (int buf_x = x; buf_x < x + width && buf_x < SDL_SCREEN_WIDTH; buf_x++) {
                if (buf_x >= 0) {
                    screen_buffer[buf_y * SDL_SCREEN_WIDTH + buf_x] = alpha_blend( screen_buffer[buf_y * SDL_SCREEN_WIDTH + buf_x]  , DEBUGGER_BG_COLOR);
                }
            }
        }
    }
    
    // 3. Рисуем рамку
    // Верхняя и нижняя границы
    for (int buf_x = x; buf_x < x + width && buf_x < SDL_SCREEN_WIDTH; buf_x++) {
        if (buf_x >= 0) {
            if (y >= 0 && y < SDL_SCREEN_HEIGHT) {
                screen_buffer[y * SDL_SCREEN_WIDTH + buf_x] = IBUF_BORDER_COLOR;
            }
            if (y + height - 1 >= 0 && y + height - 1 < SDL_SCREEN_HEIGHT) {
                screen_buffer[(y + height - 1) * SDL_SCREEN_WIDTH + buf_x] = IBUF_BORDER_COLOR;
            }
        }
    }
    
    // Левая и правая границы
    for (int buf_y = y; buf_y < y + height && buf_y < SDL_SCREEN_HEIGHT; buf_y++) {
        if (buf_y >= 0) {
            if (x >= 0 && x < SDL_SCREEN_WIDTH) {
                screen_buffer[buf_y * SDL_SCREEN_WIDTH + x] = IBUF_BORDER_COLOR;
            }
            if (x + width - 1 >= 0 && x + width - 1 < SDL_SCREEN_WIDTH) {
                screen_buffer[buf_y * SDL_SCREEN_WIDTH + (x + width - 1)] = IBUF_BORDER_COLOR;
            }
        }
    }
    
    if (buffer.get_lines_num() == 0) return;
    
    // 4. Рассчитываем параметры отображения текста
    int line_height = 10;
    int text_area_height = height - 2 * margin;
    int max_display_lines = text_area_height / line_height;
    
    if (max_display_lines <= 0) return;
    
    // 5. Устанавливаем visible_lines в буфере в зависимости от доступного места
    buffer.set_visible_lines(max_display_lines);
    
    // 6. Получаем видимые строки из буфера
    std::string visible_text = buffer.get_visible_lines_text(); // Используем новый метод
    int display_start_line = buffer.get_display_start_line();
    
    // 7. Рисуем строки
    int current_y = y + margin;
    int text_x = x + margin;
    int max_chars = (width - 2 * margin - 50) / 8; // Оставляем место для номера строки
    
    std::stringstream ss(visible_text);
    std::string line;
    int displayed_lines = 0;
    
    while (std::getline(ss, line) && displayed_lines < max_display_lines) {
        // Номер строки (1-based)
        int line_number = display_start_line + displayed_lines + 1;
        
        // Обрезаем слишком длинные строки
        std::string display_line = line;
        if (display_line.length() > static_cast<size_t>(max_chars - 6)) {
            display_line = display_line.substr(0, max_chars - 6) + "...";
        }
        
 
        
        // Рисуем строку
        draw_text(text_x, current_y, display_line.c_str(), IBUF_TEXT_COLOR);
        
        current_y += line_height;
        displayed_lines++;
    }
    
    // 8. Рисуем scrollbar если нужно
    int total_lines = static_cast<int>(buffer.get_lines_num());
    bool show_scrollbar = (total_lines > max_display_lines);
    
    if (show_scrollbar) {
        int scrollbar_width = 6;
        int scrollbar_x = x + width - margin - scrollbar_width;
        int scrollbar_area_height = text_area_height;
        
        // Вычисляем размер и позицию ползунка
        float visible_ratio = static_cast<float>(max_display_lines) / total_lines;
        int thumb_height = static_cast<int>(scrollbar_area_height * visible_ratio);
        thumb_height = std::max(8, thumb_height);
        
        float scroll_ratio = 0.0f;
        if (total_lines > max_display_lines) {
            scroll_ratio = static_cast<float>(display_start_line) / 
                          (total_lines - max_display_lines);
        }
        
        int thumb_y = y + margin + static_cast<int>(scroll_ratio * 
                   (scrollbar_area_height - thumb_height));
        
        // Рисуем фон скроллбара
        for (int bar_y = y + margin; bar_y < y + margin + scrollbar_area_height && bar_y < y + height; bar_y++) {
            for (int bar_x = scrollbar_x; bar_x < scrollbar_x + scrollbar_width && bar_x < x + width; bar_x++) {
                if (bar_x >= 0 && bar_x < SDL_SCREEN_WIDTH && bar_y >= 0 && bar_y < SDL_SCREEN_HEIGHT) {
                    screen_buffer[bar_y * SDL_SCREEN_WIDTH + bar_x] = 0xFF404040;
                }
            }
        }
        
        // Рисуем ползунок
        for (int bar_y = thumb_y; bar_y < thumb_y + thumb_height && bar_y < y + height; bar_y++) {
            for (int bar_x = scrollbar_x; bar_x < scrollbar_x + scrollbar_width && bar_x < x + width; bar_x++) {
                if (bar_x >= 0 && bar_x < SDL_SCREEN_WIDTH && bar_y >= 0 && bar_y < SDL_SCREEN_HEIGHT) {
                    screen_buffer[bar_y * SDL_SCREEN_WIDTH + bar_x] = 0xFF808080;
                }
            }
        }
    }
    
    // 9. Рисуем заголовок с информацией
    char header[256];
    bool at_end = buffer.is_at_end();
    bool last_visible = buffer.is_last_line_visible();
    
    snprintf(header, sizeof(header), "Log: %d-%d/%d", 
             display_start_line + 1, 
             display_start_line + displayed_lines, 
             total_lines);
    
    // Добавляем информацию о режиме
    if (at_end) {
        strcat(header, " [AUTO]");
    } else if (last_visible) {
        strcat(header, " [TAIL]");
    }
    
    // Рисуем заголовок
    int header_y = y - 15;
    if (header_y >= 0 && header_y < SDL_SCREEN_HEIGHT) {
        uint32_t header_color = at_end ? 0xFF00FF00 : 0xFFFFFF00;
        draw_text(x + margin, header_y, header, header_color);
    }
}


void render_debugger_with_background(const char* debug_text, 
                                                    uint32_t* screen_buffer, 
                                                    int screen_width, 
                                                    int screen_height) {
    if (!debug_text || !screen_buffer) return;
    
    // 1. Проверяем границы
    if (DEBUGGER_X >= screen_width || DEBUGGER_Y >= screen_height || 
        DEBUGGER_X + DEBUGGER_WIDTH <= 0 || DEBUGGER_Y + DEBUGGER_HEIGHT <= 0) {
        return;
    }
    
    // 2. Рисуем фон
    for (int y = DEBUGGER_Y; y < DEBUGGER_Y + DEBUGGER_HEIGHT && y < screen_height; y++) {
        if (y >= 0) {
            for (int x = DEBUGGER_X; x < DEBUGGER_X + DEBUGGER_WIDTH && x < screen_width; x++) {
                if (x >= 0) {
                    screen_buffer[y * screen_width + x] = alpha_blend(screen_buffer[y * screen_width + x] , DEBUGGER_BG_COLOR);
                }
            }
        }
    }
    
    // 3. Рисуем рамку
    // Верхняя и нижняя границы
    for (int x = DEBUGGER_X; x < DEBUGGER_X + DEBUGGER_WIDTH && x < screen_width; x++) {
        if (x >= 0) {
            if (DEBUGGER_Y >= 0 && DEBUGGER_Y < screen_height) {
                screen_buffer[DEBUGGER_Y * screen_width + x] = DEBUGGER_BORDER_COLOR;
            }
            if (DEBUGGER_Y + DEBUGGER_HEIGHT - 1 >= 0 && 
                DEBUGGER_Y + DEBUGGER_HEIGHT - 1 < screen_height) {
                screen_buffer[(DEBUGGER_Y + DEBUGGER_HEIGHT - 1) * screen_width + x] = DEBUGGER_BORDER_COLOR;
            }
        }
    }
    
    // Левая и правая границы
    for (int y = DEBUGGER_Y; y < DEBUGGER_Y + DEBUGGER_HEIGHT && y < screen_height; y++) {
        if (y >= 0) {
            if (DEBUGGER_X >= 0 && DEBUGGER_X < screen_width) {
                screen_buffer[y * screen_width + DEBUGGER_X] = DEBUGGER_BORDER_COLOR;
            }
            if (DEBUGGER_X + DEBUGGER_WIDTH - 1 >= 0 && 
                DEBUGGER_X + DEBUGGER_WIDTH - 1 < screen_width) {
                screen_buffer[y * screen_width + (DEBUGGER_X + DEBUGGER_WIDTH - 1)] = DEBUGGER_BORDER_COLOR;
            }
        }
    }
    
    // 4. Рисуем текст отладчика с отступом
    int current_x = DEBUGGER_X + DEBUGGER_MARGIN;
    int current_y = DEBUGGER_Y + DEBUGGER_MARGIN;
    
    // Копируем текст для разбивки на строки
    char buffer_copy[2048];
    strncpy(buffer_copy, debug_text, sizeof(buffer_copy));
    buffer_copy[sizeof(buffer_copy) - 1] = '\0';
    
    // Разбиваем на строки и рисуем
    char* line = strtok(buffer_copy, "\n");
    while (line && current_y < DEBUGGER_Y + DEBUGGER_HEIGHT - DEBUGGER_MARGIN) {
        draw_text(current_x, current_y, line, DEBUGGER_TEXT_COLOR);
        current_y += DEBUGGER_LINE_HEIGHT;
        line = strtok(NULL, "\n");
    }
    
    // 5. Рисуем заголовок
    const char* header = "Debugger";
    int header_y = DEBUGGER_Y - 15;
    if (header_y >= 0 && header_y < screen_height) {
        draw_text(DEBUGGER_X + DEBUGGER_MARGIN, header_y, header, DEBUGGER_HDR_TEXT_COLOR ); 
    }
}
// Главная функция update_screen
void update_screen(Z80EX_CONTEXT *cpu, uint8_t* memory, uint32_t start, EScreenMode mode)
{
    if (!cpu || !memory) return;
    
    // 1. Очистка экрана
    memset(screen_buffer, 0, SDL_SCREEN_WIDTH * SDL_SCREEN_HEIGHT * sizeof(uint32_t));
    
    // 2. Графика из памяти Z80
    switch (mode) { 
        case EScreenMode::Linear16k1b:
            update_screen_16k1b(cpu, memory, start);
            break;
        default:
            break;
    }
    
    // 3. Генерация текста отладчика
    auto str = debugger.render_to_buffer(cpu, memory);
    
    // 4. Отрисовка отладчика с фоном и рамкой
    render_debugger_with_background(str.c_str(), 
                                                   screen_buffer, 
                                                   SDL_SCREEN_WIDTH, 
                                                   SDL_SCREEN_HEIGHT);
    
    // 5. Отрисовка мини-буфера
    if (minibuffer.is_visible()) {
        minibuffer_draw(minibuffer, renderer, window);
    }
    
    // 6. Отрисовка лог-буфера
    render_ibuffer(ibuffer, IBUF_X, IBUF_Y, IBUF_W, IBUF_H, IBUF_MARGIN);
  
    // 7. Обновляем SDL текстуру из screen_buffer
    SDL_UpdateTexture(texture, NULL, screen_buffer, SDL_SCREEN_WIDTH * sizeof(uint32_t));
    
    // 8. Очищаем рендерер и рисуем текстуру
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}