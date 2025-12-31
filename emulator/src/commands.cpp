#include "commands.h"
#include "globals.h"
#include "utils/ibuffer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <SDL2/SDL.h>
#include <z80ex.h>  // Добавляем для Z80 констант

// === Вспомогательные функции ===

void print_registers(Z80EX_CONTEXT* cpu) {
    char buffer[512];
    debugger_print_registers(cpu, buffer, sizeof(buffer));
    ibuffer.printf("%s", buffer);
}

void print_help() {
    ibuffer.printf("\n=== Z80 DEBUGGER COMMANDS ===\n");
    ibuffer.printf("Execution:\n");
    ibuffer.printf("  r/run     - Run (continuous execution)\n");
    ibuffer.printf("  t/trace   - Trace (execute with pauses)\n");
    ibuffer.printf("  p/pause   - Pause execution\n");
    ibuffer.printf("  s/step    - Step (execute one instruction)\n");
    ibuffer.printf("  c/cont    - Continue execution\n");
    
    ibuffer.printf("\nBreakpoints:\n");
    ibuffer.printf("  b <addr>  - Set breakpoint at address\n");
    ibuffer.printf("  db <addr> - Remove breakpoint at address\n");
    ibuffer.printf("  lb        - List all breakpoints\n");
    ibuffer.printf("  bc        - Clear all breakpoints\n");
    
    ibuffer.printf("\nMemory:\n");
    ibuffer.printf("  m <addr> [len]    - Dump memory (default 256 bytes)\n");
    ibuffer.printf("  dis <start> <end> - Disassemble memory range\n");
    ibuffer.printf("  w <addr> <value>  - Write byte to memory\n");
    ibuffer.printf("  f <start> <end> <val> - Fill memory range with value\n");
    
    ibuffer.printf("\nCPU:\n");
    ibuffer.printf("  reg       - Show registers\n");
    ibuffer.printf("  flags     - Show flags\n");
    ibuffer.printf("  stack     - Show stack\n");
    ibuffer.printf("  pc <addr> - Set PC register\n");
    
    ibuffer.printf("\nSystem:\n");
    ibuffer.printf("  stat      - Show debugger status\n");
    ibuffer.printf("  help/?    - Show this help\n");
    ibuffer.printf("  q/quit    - Quit emulator\n");
    ibuffer.printf("  reset     - Reset CPU\n");
    
    ibuffer.printf("\n================================\n");
}

// === Регистрация команд дебаггера ===

void register_debugger_commands(CLI& cli, Debugger* debugger, Z80EX_CONTEXT* cpu, uint8_t* memory) {
    // Команды выполнения
    cli.register_command("r", "Run execution", 
        [debugger](const auto& args) {
            debugger_run(debugger);
            ibuffer.printf("Running... (press SPACE to pause)\n");
        });
    
    cli.register_command("run", "Run execution", 
        [debugger](const auto& args) {
            debugger_run(debugger);
            ibuffer.printf("Running... (press SPACE to pause)\n");
        });
    
    cli.register_command("t", "Trace execution", 
        [debugger](const auto& args) {
            debugger_trace(debugger);
            ibuffer.printf("Tracing...\n");
        });
    
    cli.register_command("trace", "Trace execution", 
        [debugger](const auto& args) {
            debugger_trace(debugger);
            ibuffer.printf("Tracing...\n");
        });
    
    cli.register_command("p", "Pause execution", 
        [debugger](const auto& args) {
            debugger_pause(debugger);
            ibuffer.printf("Paused.\n");
        });
    
    cli.register_command("pause", "Pause execution", 
        [debugger](const auto& args) {
            debugger_pause(debugger);
            ibuffer.printf("Paused.\n");
        });
    
    cli.register_command("s", "Single step", 
        [debugger](const auto& args) {
            debugger_step(debugger);
            ibuffer.printf("Stepped one instruction\n");
        });
    
    cli.register_command("step", "Single step", 
        [debugger](const auto& args) {
            debugger_step(debugger);
            ibuffer.printf("Stepped one instruction\n");
        });
    
    cli.register_command("c", "Continue execution", 
        [debugger](const auto& args) {
            debugger_run(debugger);
            ibuffer.printf("Continuing execution\n");
        });
    
    cli.register_command("cont", "Continue execution", 
        [debugger](const auto& args) {
            debugger_run(debugger);
            ibuffer.printf("Continuing execution\n");
        });
    
    // Команды точек останова
    cli.register_command("b", "Set breakpoint <address> [comment]", 
        [debugger](const auto& args) {
            if (args.empty()) {
                ibuffer.printf("Usage: b <address> [comment]\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0])) {
                ibuffer.printf("Error: address must be in hex\n");
                return;
            }
            
            uint16_t addr = CLIUtils::hex_to_int(args[0]);
            std::string comment = args.size() > 1 ? args[1] : "User breakpoint";
            
            if (debugger_add_breakpoint(debugger, addr, comment.c_str())) {
                ibuffer.printf("Breakpoint set at %04X: %s\n", addr, comment.c_str());
            } else {
                ibuffer.printf("Failed to set breakpoint at %04X\n", addr);
            }
        });
    
    cli.register_command("db", "Delete breakpoint <address>", 
        [debugger](const auto& args) {
            if (args.empty()) {
                ibuffer.printf("Usage: db <address>\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0])) {
                ibuffer.printf("Error: address must be in hex\n");
                return;
            }
            
            uint16_t addr = CLIUtils::hex_to_int(args[0]);
            
            if (debugger_remove_breakpoint(debugger, addr)) {
                ibuffer.printf("Breakpoint removed at %04X\n", addr);
            } else {
                ibuffer.printf("Breakpoint not found at %04X\n", addr);
            }
        });
    
    cli.register_command("lb", "List breakpoints", 
        [debugger](const auto& args) {
            char buffer[1024];
            debugger_list_breakpoints(debugger, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        });
    
    cli.register_command("bc", "Clear all breakpoints", 
        [debugger](const auto& args) {
            for (int i = 0; i < debugger->breakpoint_count; i++) {
                debugger->breakpoints[i].enabled = false;
            }
            debugger->breakpoint_count = 0;
            ibuffer.printf("All breakpoints cleared\n");
        });
    
    // Статус
    cli.register_command("stat", "Show debugger status", 
        [debugger, cpu](const auto& args) {
            char buffer[256];
            debugger_print_status(debugger, cpu, buffer, sizeof(buffer));
            ibuffer.printf("%s\n", buffer);
        });
}

void register_memory_commands(CLI& cli, uint8_t* memory) {
    // Дамп памяти
    cli.register_command("m", "Dump memory <address> [length]", 
        [memory](const auto& args) {
            if (args.empty()) {
                ibuffer.printf("Usage: m <address> [length]\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0])) {
                ibuffer.printf("Error: address must be in hex\n");
                return;
            }
            
            uint16_t addr = CLIUtils::hex_to_int(args[0]);
            uint16_t len = 256;  // По умолчанию
            
            if (args.size() > 1) {
                if (!CLIUtils::is_hex(args[1])) {
                    ibuffer.printf("Error: length must be in hex\n");
                    return;
                }
                len = CLIUtils::hex_to_int(args[1]);
            }
            
            char buffer[4096];
            debugger_memory_dump(memory, addr, len, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        });
    
    cli.register_command("dump", "Dump memory <address> [length]", 
        [memory](const auto& args) {
            if (args.empty()) {
                ibuffer.printf("Usage: dump <address> [length]\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0])) {
                ibuffer.printf("Error: address must be in hex\n");
                return;
            }
            
            uint16_t addr = CLIUtils::hex_to_int(args[0]);
            uint16_t len = 256;
            
            if (args.size() > 1) {
                if (!CLIUtils::is_hex(args[1])) {
                    ibuffer.printf("Error: length must be in hex\n");
                    return;
                }
                len = CLIUtils::hex_to_int(args[1]);
            }
            
            char buffer[4096];
            debugger_memory_dump(memory, addr, len, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        });
    
    // Дизассемблирование
    cli.register_command("dis", "Disassemble <start> <end>", 
        [memory](const auto& args) {
            if (args.size() < 2) {
                ibuffer.printf("Usage: dis <start> <end>\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0]) || !CLIUtils::is_hex(args[1])) {
                ibuffer.printf("Error: addresses must be in hex\n");
                return;
            }
            
            uint16_t start = CLIUtils::hex_to_int(args[0]);
            uint16_t end = CLIUtils::hex_to_int(args[1]);
            
            char buffer[4096];
            debugger_disassemble_range(memory, start, end, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        });
    
    cli.register_command("disasm", "Disassemble <start> <end>", 
        [memory](const auto& args) {
            if (args.size() < 2) {
                ibuffer.printf("Usage: disasm <start> <end>\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0]) || !CLIUtils::is_hex(args[1])) {
                ibuffer.printf("Error: addresses must be in hex\n");
                return;
            }
            
            uint16_t start = CLIUtils::hex_to_int(args[0]);
            uint16_t end = CLIUtils::hex_to_int(args[1]);
            
            char buffer[4096];
            debugger_disassemble_range(memory, start, end, buffer, sizeof(buffer));
            ibuffer.printf("%s", buffer);
        });
    
    // Запись в память
    cli.register_command("w", "Write byte to memory <address> <value>", 
        [memory](const auto& args) {
            if (args.size() < 2) {
                ibuffer.printf("Usage: w <address> <value>\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0]) || !CLIUtils::is_hex(args[1])) {
                ibuffer.printf("Error: address and value must be in hex\n");
                return;
            }
            
            uint16_t addr = CLIUtils::hex_to_int(args[0]);
            uint8_t value = CLIUtils::hex_to_int(args[1]);
            
            memory[addr] = value;
            ibuffer.printf("Memory[%04X] = %02X\n", addr, value);
        });
    
    cli.register_command("fill", "Fill memory range <start> <end> <value>", 
        [memory](const auto& args) {
            if (args.size() < 3) {
                ibuffer.printf("Usage: fill <start> <end> <value>\n");
                return;
            }
            
            if (!CLIUtils::is_hex(args[0]) || !CLIUtils::is_hex(args[1]) || !CLIUtils::is_hex(args[2])) {
                ibuffer.printf("Error: all parameters must be in hex\n");
                return;
            }
            
            uint16_t start = CLIUtils::hex_to_int(args[0]);
            uint16_t end = CLIUtils::hex_to_int(args[1]);
            uint8_t value = CLIUtils::hex_to_int(args[2]);
            
            if (start > end) {
                ibuffer.printf("Error: start address must be <= end address\n");
                return;
            }
            
            for (uint16_t addr = start; addr <= end; addr++) {
                memory[addr] = value;
            }
            
            ibuffer.printf("Filled memory %04X-%04X with %02X\n", start, end, value);
        });
}

void register_cpu_commands(CLI& cli, Z80EX_CONTEXT* cpu, uint8_t* memory) {  // Добавляем memory в параметры
    // Регистры
    cli.register_command("reg", "Show registers", 
        [cpu](const auto& args) {
            print_registers(cpu);
        });
    
    cli.register_command("registers", "Show registers", 
        [cpu](const auto& args) {
            print_registers(cpu);
        });
    
    // Флаги
    cli.register_command("flags", "Show flags", 
        [cpu](const auto& args) {
            // Используем правильные константы из z80ex
            // regF - это 6 для регистра F согласно документации z80ex
            uint8_t f = z80ex_get_reg(cpu, Z80_REG_T::regAF)>>8;  // regF
            
            ibuffer.printf("Flags: ");
            ibuffer.printf("S=%d ", (f & 0x80) ? 1 : 0);
            ibuffer.printf("Z=%d ", (f & 0x40) ? 1 : 0);
            ibuffer.printf("H=%d ", (f & 0x10) ? 1 : 0);
            ibuffer.printf("P/V=%d ", (f & 0x04) ? 1 : 0);
            ibuffer.printf("N=%d ", (f & 0x02) ? 1 : 0);
            ibuffer.printf("C=%d\n", (f & 0x01) ? 1 : 0);
        });
    
    // Стек
    cli.register_command("stack", "Show stack", 
        [cpu, memory](const auto& args) {
            uint16_t sp = z80ex_get_reg(cpu, Z80_REG_T::regSP);  // regSP
            
            ibuffer.printf("Stack pointer: SP=%04X\n", sp);
            ibuffer.printf("Stack contents:\n");
            
            for (int i = 0; i < 16; i += 2) {
                uint16_t addr = sp + i;
                if (addr >= 0xFFFF) break;
                
                uint16_t value = memory[addr] | (memory[addr + 1] << 8);
                ibuffer.printf("  %04X: %04X\n", addr, value);
            }
        });
    
    // Установка PC
    cli.register_command("pc", "Set PC register <address>", 
        [cpu](const auto& args) {
            if (args.empty()) {
                ibuffer.printf("Current PC: %04X\n", z80ex_get_reg(cpu, Z80_REG_T::regPC));  // regPC
                return;
            }
            
            if (!CLIUtils::is_hex(args[0])) {
                ibuffer.printf("Error: address must be in hex\n");
                return;
            }
            
            uint16_t addr = CLIUtils::hex_to_int(args[0]);
            z80ex_set_reg(cpu, Z80_REG_T::regPC, addr);  // regPC
            ibuffer.printf("PC set to %04X\n", addr);
        });
    
    // Сброс
    cli.register_command("reset", "Reset CPU", 
        [cpu](const auto& args) {
            z80ex_reset(cpu);
            ibuffer.printf("CPU reset\n");
        });
    
    // Выход
    cli.register_command("q", "Quit emulator", 
        [](const auto& args) {
            ibuffer.printf("Quitting...\n");
            SDL_Event quit_event;
            quit_event.type = SDL_QUIT;
            SDL_PushEvent(&quit_event);
        });
    
    cli.register_command("quit", "Quit emulator", 
        [](const auto& args) {
            ibuffer.printf("Quitting...\n");
            SDL_Event quit_event;
            quit_event.type = SDL_QUIT;
            SDL_PushEvent(&quit_event);
        });
    
    cli.register_command("exit", "Quit emulator", 
        [](const auto& args) {
            ibuffer.printf("Quitting...\n");
            SDL_Event quit_event;
            quit_event.type = SDL_QUIT;
            SDL_PushEvent(&quit_event);
        });
}

// === Инициализация всех команд ===

void init_debugger_commands(Debugger* debugger, Z80EX_CONTEXT* cpu, uint8_t* memory) {
    // Регистрируем команды помощи
    cli.register_command("help", "Show help", 
        [](const auto& args) { print_help(); });
    
    cli.register_command("?", "Show help", 
        [](const auto& args) { print_help(); });
    
    // Регистрируем остальные команды
    register_debugger_commands(cli, debugger, cpu, memory);
    register_memory_commands(cli, memory);
    register_cpu_commands(cli, cpu, memory);  // Передаем memory
}