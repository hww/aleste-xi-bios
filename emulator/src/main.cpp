#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <z80ex.h>
#include <SDL2/SDL.h>
#include <string.h>

#include "screen.h"
#include "memory.h"
#include "debugger.h"
#include "ports.h"
#include "globals.h"
#include "cli.h"
#include "utils/minibuffer.h"
#include "utils/ibuffer.h"
#include "commands.h"



int main(int argc, char *argv[]) {
    if (argc < 2) {
        ibuffer.printf("Usage: %s <bios.bin>\n", argv[0]);
        return 1;
    }
    
    // Инициализация
    ports_reset();

    printf("Loading file %s...\n", argv[1]);
    
    // Загрузка BIOS
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open BIOS file");
        return 1;
    }
    
    size_t bytes_read = fread(&memory[0xC00000], 1, 0x10000, f);
    fclose(f);
    
    if (bytes_read == 0) {
        ibuffer.printf("Failed to read BIOS file\n");
        return 1;
    }
    
    ibuffer.printf("Loaded %zu bytes into memory\n", bytes_read);
    
    // Инициализация CPU
    cpu = z80ex_create(mem_read, NULL, mem_write, NULL,
                       port_read, NULL, port_write, NULL,
                       int_read, NULL);
    
    if (!cpu) {
        ibuffer.printf("Failed to create Z80 CPU context\n");
        return 1;
    }
    
    // Инициализация SDL
    if (!init_sdl()) {
        ibuffer.printf("Failed to initialize SDL\n");
        return 1;
    }
    
    // Инициализация команд дебаггера
    init_debugger_commands(&debugger, cpu, memory);
    
    // Установка начального состояния CPU
    z80ex_set_reg(cpu, regPC, 0x0000);
    
    ibuffer.printf("Z80 Emulator with Debugger started\n");
    ibuffer.printf("Loaded BIOS from %s (%zu bytes)\n", argv[1], bytes_read);
    ibuffer.printf("CPU initialized, PC = 0x%04X\n", z80ex_get_reg(cpu, regPC));
    
    // Показываем справку по командам
    cli.execute("help");

    // Главный цикл
    bool quit = false;
    SDL_Event e;
    
    uint32_t last_update = SDL_GetTicks();
    uint32_t update_interval = 1000 / 60; // 60 FPS
    
    while (!quit) {
        // Обработка событий
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            
            bool alt_pressed = (SDL_GetModState() & (KMOD_LALT | KMOD_RALT)) != 0;
            bool ctrl_pressed = (SDL_GetModState() & KMOD_CTRL) != 0;
            
            if (minibuffer.is_visible()) {
                // Обработка событий мини-буфера
                if (minibuffer.handle_event(e)) {
                    auto cmds = minibuffer.get_text();
                    const char *cmd = cmds.c_str();
                    cli.execute(cmd);
                    minibuffer.hide();
                    minibuffer.clear();
                }
            } else {
                // Обработка горячих клавиш дебаггера
                if (e.type == SDL_KEYDOWN) {
                    switch (e.key.keysym.sym) {
                        case SDLK_x:
                            if (alt_pressed) {
                                minibuffer.show(">> ", "");
                                //ibuffer.printf("Minibuffer activated (Alt+X)\n");
                            }
                            break;
                            
                        case SDLK_SLASH:
                            if (ctrl_pressed) {
                                minibuffer.show(">> ", "help");
                            }
                            break;
                            
                        case SDLK_r:
                            if (!ctrl_pressed) {
                                debugger.run();
                                ibuffer.printf("Debugger: Run mode\n");
                            }
                            break;
                            
                        case SDLK_s:
                            if (!ctrl_pressed) {
                                debugger.step();
                                ibuffer.printf("Debugger: Single step\n");
                            }
                            break;
                            
                        case SDLK_t:
                            debugger.trace();
                            ibuffer.printf("Debugger: Trace mode\n");
                            break;
                            
                        case SDLK_SPACE:
                            if (debugger.mode() == DebugMode::RUN) {
                                debugger.pause();
                                ibuffer.printf("Debugger: Paused\n");
                            } else {
                                debugger.run();
                                ibuffer.printf("Debugger: Running\n");
                            }
                            break;
                            
                        case SDLK_b:
                            if (ctrl_pressed) {
                                minibuffer.show(">> ", "b ");
                            }
                            break;
                            
                        case SDLK_d:
                            if (ctrl_pressed) {
                                minibuffer.show(">> ", "dis ");
                            }
                            break;
                            
                        case SDLK_m:
                            if (ctrl_pressed) {
                                minibuffer.show(">> ", "m ");
                            }
                            break;
                            
                        case SDLK_g:
                            if (ctrl_pressed) {
                                minibuffer.show(">> ", "reg");
                            }
                            break;
                            
                        case SDLK_ESCAPE:
                            if (debugger.mode() == DebugMode::PAUSE) {
                                quit = true;
                            } else {
                                debugger.pause();
                            }
                            break;
                            
                        case SDLK_h:
                            cli.execute("help");
                            break;

                        case SDLK_PAGEUP:
                            ibuffer.page_up();
                            break;
                        case SDLK_PAGEDOWN:
                            ibuffer.page_down();
                            break;
                        case SDLK_HOME:
                            ibuffer.scroll_to_top();
                            break;
                        case SDLK_END:
                            ibuffer.scroll_to_end();
                            break;
                        case SDLK_UP:
                            ibuffer.scroll_up();
                            break;
                        case SDLK_DOWN:
                            ibuffer.scroll_down();
                            break;                                                                               
                    }
                }
            }
        }
        
        // Обновление дебаггера (выполнение инструкций)
        debugger.update(cpu, memory);
        
        // Обновление экрана с фиксированной частотой
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_update >= update_interval) {
            last_update = current_time;
            
            // Обновление заголовка окна
            char window_title[256];
            snprintf(window_title, sizeof(window_title), 
                    "Z80 Debugger - PC: 0x%04X - Mode: %s", 
                    z80ex_get_reg(cpu, regPC),
                    Debugger::mode_to_str(debugger.mode()));
            set_window_title(window_title);
            
            // Обновление экрана
            update_screen(cpu, memory, g_crct.screenPosition, g_crct.screenMode);
        }
        
        // Небольшая задержка для снижения нагрузки на CPU
        SDL_Delay(1);
    }
    
    // Очистка
    ibuffer.printf("Cleaning up...\n");
    z80ex_destroy(cpu);
    
    // Не очищаем SDL ресурсы здесь - они объявлены в screen.c
    // Функция cleanup должна быть в screen.c
    // Просто вызываем SDL_Quit
    SDL_Quit();
    
    ibuffer.printf("Goodbye!\n");
    return 0;
}