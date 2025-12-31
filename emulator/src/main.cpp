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
#include "render/minibuffer.h"
#include "render/ibuffer.h"

// Глобальные переменные
Debugger debugger;
Z80EX_CONTEXT *cpu = NULL;


void print_help(void) {
    ibuffer.printf("\n=== Z80 DEBUGGER COMMANDS ===\n");
    ibuffer.printf("  r        - Run (continuous execution)\n");
    ibuffer.printf("  t        - Trace (execute with pauses)\n");
    ibuffer.printf("  p        - Pause execution\n");
    ibuffer.printf("  s        - Step (execute one instruction)\n");
    ibuffer.printf("  b <addr> - Set breakpoint at address\n");
    ibuffer.printf("  d <addr> - Remove breakpoint at address\n");
    ibuffer.printf("  m <addr> [len] - Dump memory\n");
    ibuffer.printf("  dis <start> <end> - Disassemble memory range\n");
    ibuffer.printf("  reg      - Show registers\n");
    ibuffer.printf("  stat     - Show debugger status\n");
    ibuffer.printf("  h        - Show this help\n");
    ibuffer.printf("  q        - Quit\n");
    ibuffer.printf("\nMINIBUFFER KEYS:\n");
    ibuffer.printf("  Alt+X    - Toggle minibuffer\n");
    ibuffer.printf("  ESC      - Close minibuffer\n");
    ibuffer.printf("  ENTER    - Execute command\n");
    ibuffer.printf("  INS      - Toggle insert/overwrite mode\n");
    ibuffer.printf("\nDEBUGGER HOTKEYS:\n");
    ibuffer.printf("  SPACE    - Toggle run/pause\n");
    ibuffer.printf("  S        - Single step\n");
    ibuffer.printf("  R        - Run\n");
    ibuffer.printf("  T        - Trace mode\n");
    ibuffer.printf("  ESC      - Quit\n");
    ibuffer.printf("================================\n");
}

const char* debugger_mode_to_str(int mode) {
    switch (mode) {
        case DEBUG_MODE_PAUSE: return "PAUSE";
        case DEBUG_MODE_RUN: return "RUN";
        case DEBUG_MODE_STEP: return "STEP";
        case DEBUG_MODE_TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

void handle_command(const char *cmd) {
    if (!cmd || strlen(cmd) == 0) return;
    
    ibuffer.printf("Executing command: %s\n", cmd);
    
    char command[64];
    uint16_t arg1 = 0, arg2 = 0;
    
    if (sscanf(cmd, "%s %hx %hx", command, &arg1, &arg2) < 1) {
        return;
    }
    
    if (strcmp(command, "r") == 0) {
        debugger_run(&debugger);
        ibuffer.printf("Running... (press SPACE to pause)\n");
    }
    else if (strcmp(command, "t") == 0) {
        debugger_trace(&debugger);
        ibuffer.printf("Tracing...\n");
    }
    else if (strcmp(command, "p") == 0) {
        debugger_pause(&debugger);
        ibuffer.printf("Paused.\n");
    }
    else if (strcmp(command, "s") == 0) {
        debugger_step(&debugger);
        ibuffer.printf("Stepping...\n");
    }
    else if (strcmp(command, "b") == 0) {
        if (arg1 != 0) {
            if (debugger_add_breakpoint(&debugger, arg1, "User breakpoint")) {
                ibuffer.printf("Breakpoint set at %04X\n", arg1);
            } else {
                ibuffer.printf("Failed to set breakpoint\n");
            }
        } else {
            ibuffer.printf("Usage: b <address>\n");
        }
    }
    else if (strcmp(command, "d") == 0) {
        if (arg1 != 0) {
            if (debugger_remove_breakpoint(&debugger, arg1)) {
                ibuffer.printf("Breakpoint removed at %04X\n", arg1);
            } else {
                ibuffer.printf("Breakpoint not found at %04X\n", arg1);
            }
        } else {
            ibuffer.printf("Usage: d <address>\n");
        }
    }
    else if (strcmp(command, "lb") == 0) {
        char buffer[512];
        debugger_list_breakpoints(&debugger, buffer, sizeof(buffer));
        ibuffer.printf("%s", buffer);
    }
    else if (strcmp(command, "m") == 0) {
        if (arg1 != 0) {
            uint16_t len = (arg2 != 0) ? arg2 : 256;
            char buffer[2048];
            debugger_memory_dump(memory, arg1, len, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        } else {
            ibuffer.printf("Usage: m <address> [length]\n");
        }
    }
    else if (strcmp(command, "dis") == 0) {
        if (arg1 != 0) {
            uint16_t end = (arg2 != 0) ? arg2 : (arg1 + 128);
            char buffer[2048];
            debugger_disassemble_range(memory, arg1, end, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        } else {
            ibuffer.printf("Usage: dis <start> <end>\n");
        }
    }
    else if (strcmp(command, "reg") == 0) {
        char buffer[256];
        debugger_print_registers(cpu, buffer, sizeof(buffer));
        printf("%s\n", buffer);
    }
    else if (strcmp(command, "stat") == 0) {
        char buffer[256];
        debugger_print_status(&debugger, cpu, buffer, sizeof(buffer));
        ibuffer.printf("%s\n", buffer);
    }
    else if (strcmp(command, "h") == 0) {
        print_help();
    }
    else if (strcmp(command, "q") == 0) {
        ibuffer.printf("Quitting...\n");
        SDL_Event quit_event;
        quit_event.type = SDL_QUIT;
        SDL_PushEvent(&quit_event);
    }
    else {
        cli.execute(command);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        ibuffer.printf("Usage: %s <bios.bin>\n", argv[0]);
        return 1;
    }
    
    // Инициализация
    debugger_init(&debugger);
    ports_reset();

    printf("Loading file %s...\n", argv[1]);
    
    // Загрузка BIOS
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open BIOS file");
        return 1;
    }
    
    size_t bytes_read = fread(memory, 1, 0x10000, f);
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
    
    // Установка начального состояния CPU
    // PC = 0x0000 для большинства Z80 систем
    z80ex_set_reg(cpu, regPC, 0x0000);
    
    ibuffer.printf("Z80 Emulator with Debugger started\n");
    ibuffer.printf("Loaded BIOS from %s (%zu bytes)\n", argv[1], bytes_read);
    ibuffer.printf("CPU initialized, PC = 0x%04X\n", z80ex_get_reg(cpu, regPC));
    print_help();
    


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
                    const char *cmd = minibuffer.get_text().c_str();
                    handle_command(cmd);
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
                                ibuffer.printf("Minibuffer activated (Alt+X)\n");
                            }
                            break;
                            
                        case SDLK_SLASH:
                            if (ctrl_pressed) {
                                minibuffer.show(">> ", "help");
                            }
                            break;
                            
                        case SDLK_r:
                            if (!ctrl_pressed) {
                                debugger_run(&debugger);
                                ibuffer.printf("Debugger: Run mode\n");
                            }
                            break;
                            
                        case SDLK_s:
                            if (!ctrl_pressed) {
                                debugger_step(&debugger);
                                ibuffer.printf("Debugger: Single step\n");
                            }
                            break;
                            
                        case SDLK_t:
                            debugger_trace(&debugger);
                            printf("Debugger: Trace mode\n");
                            break;
                            
                        case SDLK_SPACE:
                            if (debugger.mode == DEBUG_MODE_RUN) {
                                debugger_pause(&debugger);
                                ibuffer.printf("Debugger: Paused\n");
                            } else {
                                debugger_run(&debugger);
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
                            if (debugger.mode == DEBUG_MODE_PAUSE) {
                                quit = true;
                            } else {
                                debugger_pause(&debugger);
                            }
                            break;
                            
                        case SDLK_h:
                            print_help();
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
        debugger_update(&debugger, cpu, memory);
        
        // Обновление экрана с фиксированной частотой
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_update >= update_interval) {
            last_update = current_time;
            
            // Обновление заголовка окна
            char window_title[256];
            snprintf(window_title, sizeof(window_title), 
                    "Z80 Debugger - PC: 0x%04X - Mode: %s", 
                    z80ex_get_reg(cpu, regPC),
                    debugger_mode_to_str(debugger.mode));
            set_window_title(window_title);
            
            // Обновление экрана
            update_screen(cpu, memory, g_crct.screenPosition, g_crct.screenMode, &debugger);
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