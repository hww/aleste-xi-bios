#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <z80ex.h>

#include "screen.h"
#include "memory.h"
#include "ports.h"
#include "debugger.h"
#include "mmu_native.h"

// Глобальные переменные
Debugger debugger;
Z80EX_CONTEXT *cpu = NULL;

// ============ КОМАНДНЫЙ ИНТЕРФЕЙС ============

void print_help(void)
{
    printf("\n=== Z80 DEBUGGER COMMANDS ===\n");
    printf("  r        - Run (continuous execution)\n");
    printf("  t        - Trace (execute with pauses)\n");
    printf("  p        - Pause execution\n");
    printf("  s        - Step (execute one instruction)\n");
    printf("  b <addr> - Set breakpoint at address\n");
    printf("  d <addr> - Remove breakpoint at address\n");
    printf("  m <addr> [len] - Dump memory\n");
    printf("  dis <start> <end> - Disassemble memory range\n");
    printf("  reg      - Show registers\n");
    printf("  stat     - Show debugger status\n");
    printf("  h        - Show this help\n");
    printf("  q        - Quit\n");
    printf("\nHOTKEYS:\n");
    printf("  SPACE    - Toggle run/pause\n");
    printf("  S        - Single step\n");
    printf("  R        - Run\n");
    printf("  T        - Trace mode\n");
    printf("  ESC      - Quit\n");
    printf("================================\n");
}

void handle_command(const char *cmd)
{
    if (!cmd) return;
    
    char command[64];
    uint16_t arg1 = 0, arg2 = 0;
    
    if (sscanf(cmd, "%s %hx %hx", command, &arg1, &arg2) < 1) {
        return;
    }
    
    if (strcmp(command, "r") == 0) {
        debugger_run(&debugger);
        printf("Running... (press SPACE to pause)\n");
    }
    else if (strcmp(command, "t") == 0) {
        debugger_trace(&debugger);
        printf("Tracing...\n");
    }
    else if (strcmp(command, "p") == 0) {
        debugger_pause(&debugger);
        printf("Paused.\n");
    }
    else if (strcmp(command, "s") == 0) {
        debugger_step(&debugger);
        printf("Stepping...\n");
    }
    else if (strcmp(command, "b") == 0) {
        if (arg1 != 0) {
            if (debugger_add_breakpoint(&debugger, arg1, "User breakpoint")) {
                printf("Breakpoint set at %04X\n", arg1);
            } else {
                printf("Failed to set breakpoint\n");
            }
        } else {
            printf("Usage: b <address>\n");
        }
    }
    else if (strcmp(command, "d") == 0) {
        if (arg1 != 0) {
            if (debugger_remove_breakpoint(&debugger, arg1)) {
                printf("Breakpoint removed at %04X\n", arg1);
            } else {
                printf("Breakpoint not found at %04X\n", arg1);
            }
        } else {
            printf("Usage: d <address>\n");
        }
    }
    else if (strcmp(command, "lb") == 0) {
        char buffer[512];
        debugger_list_breakpoints(&debugger, buffer, sizeof(buffer));
        printf("%s", buffer);
    }
    else if (strcmp(command, "m") == 0) {
        if (arg1 != 0) {
            uint16_t len = (arg2 != 0) ? arg2 : 256;
            char buffer[2048];
            debugger_memory_dump(memory, arg1, len, buffer, sizeof(buffer));
            printf("%s", buffer);
        } else {
            printf("Usage: m <address> [length]\n");
        }
    }
    else if (strcmp(command, "dis") == 0) {
        if (arg1 != 0) {
            uint16_t end = (arg2 != 0) ? arg2 : (arg1 + 128);
            char buffer[2048];
            debugger_disassemble_range(memory, arg1, end, buffer, sizeof(buffer));
            printf("%s", buffer);
        } else {
            printf("Usage: dis <start> <end>\n");
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
        printf("%s\n", buffer);
    }
    else if (strcmp(command, "h") == 0) {
        print_help();
    }
    else if (strcmp(command, "q") == 0) {
        printf("Quitting...\n");
        // Установим флаг для выхода из основного цикла
    }
    else {
        printf("Unknown command: %s\n", command);
        print_help();
    }
}

// ============ ГЛАВНАЯ ФУНКЦИЯ ============

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <bios.bin>\n", argv[0]);
        return 1;
    }
    
    // Инициализация дебаггера
    debugger_init(&debugger);
    ports_reset();
    
    printf("Loading file %s...\n", argv[1]);

    // Загрузка BIOS
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Failed to open BIOS file");
        return 1;
    }
    fread(memory, 1, 0x10000, f);
    fclose(f);
    
    // Инициализация CPU
    cpu = z80ex_create(mem_read, NULL, mem_write, NULL,
                       port_read, NULL, port_write, NULL,
                       int_read, NULL);
    
    // Инициализация SDL
    if (!init_sdl()) {
        printf("Failed to initialize SDL\n");
        return -1;
    }
    
    printf("Z80 Emulator with Debugger started\n");
    printf("Loaded BIOS from %s\n", argv[1]);
    print_help();
    
    // Главный цикл
    bool quit = false;
    SDL_Event e;
    
    // Включим ввод текста для команд
    SDL_StartTextInput();
    
    // Буфер для команд
    char command_buffer[256] = {0};
    int command_pos = 0;
    
    // Для вывода командной строки
    bool show_command_line = false;
    
    while (!quit) {
        // Обработка SDL событий
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_KEYDOWN) {
                // Горячие клавиши для дебаггера
                switch (e.key.keysym.sym) {
                    case SDLK_r: // Run
                        debugger_run(&debugger);
                        printf("Running...\n");
                        break;
                    case SDLK_t: // Trace
                        debugger_trace(&debugger);
                        printf("Tracing...\n");
                        break;
                    case SDLK_SPACE: // Pause/Run toggle
                        if (debugger.mode == DEBUG_MODE_PAUSE) {
                            debugger_run(&debugger);
                            printf("Running...\n");
                        } else {
                            debugger_pause(&debugger);
                            printf("Paused.\n");
                        }
                        break;
                    case SDLK_s: // Step
                        debugger_step(&debugger);
                        printf("Stepping...\n");
                        break;
                    case SDLK_ESCAPE: // Выход
                        quit = true;
                        break;
                    case SDLK_RETURN: // Выполнить команду
                        if (command_pos > 0) {
                            printf("> %s\n", command_buffer);
                            handle_command(command_buffer);
                            command_pos = 0;
                            command_buffer[0] = '\0';
                        }
                        break;
                    case SDLK_BACKSPACE: // Удалить символ
                        if (command_pos > 0) {
                            command_pos--;
                            command_buffer[command_pos] = '\0';
                        }
                        break;
                    case SDLK_SLASH: // Показать/скрыть командную строку
                        show_command_line = !show_command_line;
                        if (show_command_line) {
                            printf("Command line active. Type commands and press Enter.\n");
                        }
                        break;
                }
            }
            else if (e.type == SDL_TEXTINPUT && show_command_line) {
                // Ввод текста для команд
                if (command_pos < sizeof(command_buffer) - 1) {
                    command_buffer[command_pos++] = e.text.text[0];
                    command_buffer[command_pos] = '\0';
                }
            }
        }
        
        // Обновление дебаггера (выполнение инструкций)
        if (debugger_update(&debugger, cpu, memory)) {
            // Если что-то выполнили, обновим экран
            update_screen(cpu, memory, g_crct.screenPosition, g_crct.screenMode, &debugger);
        } else if (debugger.mode == DEBUG_MODE_PAUSE) {
            // Если в паузе, тоже обновляем экран
            update_screen(cpu, memory, g_crct.screenPosition, g_crct.screenMode, &debugger);
        }
        
        // Небольшая задержка
        SDL_Delay(1);
    }
    
    // Очистка
    z80ex_destroy(cpu);
    SDL_StopTextInput();
    SDL_Quit();
    
    return 0;
}