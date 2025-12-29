#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <SDL2/SDL.h>

#include "debugger.h"
#include "ports.h"
#include "text_colors.h"

// Функции для отображения дебаггера
void render_debugger_info(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory);

void draw_text(int x, int y, const char* str, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color);
bool init_sdl();
void set_window_title(const char* title);
void update_screen(Z80EX_CONTEXT *cpu, uint8_t* memory, uint32_t start, EScreenMode mode, Debugger *dbg);

#endif //_SCREEN_H_