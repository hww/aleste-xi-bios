#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <SDL2/SDL.h>

typedef enum {
    EScreenMode_Cpc_4b, 
    EScreenMode_Cpc_2b,
    EScreenMode_Cpc_1b,
    EScreenMode_Linear16k1b,
    EScreenMode_Linear16k2b,
    EScreenMode_Linear16k4b,
    EScreenMode_Linear32k2b,
    EScreenMode_Linear32k4b,
    EScreenMode_Linear32k8b,
    EScreenMode_Linear64k2b,
    EScreenMode_Linear64k4b,
    EScreenMode_Linear64k8b
} EScreenMode;

void draw_text(int x, int y, const char* str, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color);
bool init_sdl();
void set_window_title(const char* title);
void update_screen(Z80EX_CONTEXT *cpu, uint8_t* memory, uint32_t start, EScreenMode mode);

#endif //_SCREEN_H_