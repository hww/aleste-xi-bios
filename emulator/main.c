#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <SDL2/SDL.h>
#include "debugger.h"
#include "screen.h"
#include "memory.h"
#include "ports.h"


EScreenMode screenMode = EScreenMode_Linear16k1b;
uint32_t screenPosition = 0;


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <bios.bin>\n", argv[0]);
        return 1;
    }

    // 1. Загрузка бинарника
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open BIOS file");
        return 1;
    }
    fread(memory, 1, ROM_SIZE, f);
    fclose(f);

    // 2. Инициализация CPU
    Z80EX_CONTEXT *cpu = z80ex_create(
        mem_read,  NULL,  // 1, 2
        mem_write, NULL,  // 3, 4
        port_read, NULL,  // 5, 6
        port_write, NULL, // 7, 8
        int_read,  NULL   // 9, 10
    );

    printf("--- CPU Started ---\n");

    // 3. Главный цикл (пока без SDL2, просто гоним инструкции)
    // Внутри main вместо while(true):
    if (!init_sdl()) {
        printf("Can't init SDL\n");
        return -1;
    }
    
    printf("SDL Started\n");

    bool quit = false;
    SDL_Event e;
    bool is_running = false; // Начинаем в режиме паузы для контроля

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_SPACE: // Пауза/Пуск
                        is_running = !is_running;
                        printf(is_running ? "Running...\n" : "Paused.\n");
                        break;
                    case SDLK_s: // Шаг
                        if (!is_running) {
                            z80ex_step(cpu);
                            // Обновляем экран сразу после шага
                            update_screen(cpu, memory, screenPosition, screenMode);
                        }
                        break;
                    case SDLK_r: // Сброс
                        z80ex_reset(cpu);
                        break;
                }
            }
        }

        if (is_running) {
            long cycles_to_run = 66666; 
            while (cycles_to_run > 0) {
                cycles_to_run -= z80ex_step(cpu);
            }
        }

        update_screen(cpu, memory, screenPosition, screenMode);
    }

    z80ex_destroy(cpu);
    return 0;
}