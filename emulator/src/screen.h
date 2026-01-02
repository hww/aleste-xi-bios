#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <SDL2/SDL.h>

#include "debugger.h"
#include "ports.h"
#include "forward_decl.h"
#include "globals.h"
#include "crtc.h"
#include "utils/text_colors.h"

// ============================================================
// Screen methods
// ============================================================

#define SDL_SCREEN_WIDTH  (640)
#define SDL_SCREEN_HEIGHT  (400)

bool init_sdl();
void set_window_title(const char* title);
void update_screen(Z80EX_CONTEXT *cpu, uint8_t* memory, uint32_t start, EScreenMode mode);

// ============================================================
// Text Renderer
// ============================================================

enum class EStyle : uint8_t {
    Normal = 0,
    Inverse = 1,
    InverseBlink = 2
};

struct StyledChart {
    char value;
    EStyle style;
};

void draw_text(int x, int y, const char* str, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color, EStyle style = EStyle::Normal);

// ============================================================
// MiniBuffer
// ============================================================

#define MINIBBUF_MARGIN_X 8
#define MINIBBUF_MARGIN_Y 4
#define MINIBBUF_H 16
#define MINIBBUF_Y  SDL_SCREEN_HEIGHT - MINIBBUF_H;
#define MINIBBUF_BORDER_COLOR 0xFFFFFF     
#define MINIBBUF_BG_COLOR 0x80000000
#define MINIBBUF_TEXT_COLOR 0xFFFFFFFF  // Белый текст

// ============================================================
// Interactive Result Buffer
// ============================================================

// Константы для IBuffer рендеринга
#define IBUF_W (SDL_SCREEN_WIDTH - 20)
#define IBUF_H 164
#define IBUF_X 8
#define IBUF_Y SDL_SCREEN_HEIGHT - 22 - IBUF_H
#define IBUF_MARGIN 8
#define IBUF_BG_COLOR 0x80000000  // Полупрозрачный черный (50% альфа)
#define IBUF_TEXT_COLOR 0xFFFFFFFF  // Белый текст
#define IBUF_BORDER_COLOR 0xFFFFFF  // Серый бордюр
#define IBUF_BG_COLOR 0x80000000

// Функция рендеринга IBuffer
void render_ibuffer(IBuffer& buffer, int x = IBUF_X, int y = IBUF_Y, 
                    int width = IBUF_W, int height = IBUF_H, 
                    int margin = IBUF_MARGIN);

// ============================================================
// Debugger Buffer
// ============================================================

#define DEBUGGER_X 8
#define DEBUGGER_Y 8
#define DEBUGGER_LINE_HEIGHT 8
#define DEBUGGER_WIDTH 480
#define DEBUGGER_HEIGHT 180
#define DEBUGGER_MARGIN  8
#define DEBUGGER_TEXT_COLOR 0xFFFFFF
#define DEBUGGER_HDR_TEXT_COLOR 0xFF00FF00
#define DEBUGGER_BG_COLOR 0x80000000
#define DEBUGGER_BORDER_COLOR 0xFFFFFF                    

// Функции для отображения дебаггера
void render_debugger_info(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory);


#endif //_SCREEN_H_