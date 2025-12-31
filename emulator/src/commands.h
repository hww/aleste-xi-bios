#ifndef COMMANDS_H
#define COMMANDS_H

#include "debugger.h"
#include "cli.h"

// Инициализация всех команд дебаггера
void init_debugger_commands(Debugger* debugger, Z80EX_CONTEXT* cpu, uint8_t* memory);

// Отдельные команды для регистрации
void register_debugger_commands(CLI& cli, Debugger* debugger, Z80EX_CONTEXT* cpu, uint8_t* memory);
void register_memory_commands(CLI& cli, uint8_t* memory);
void register_cpu_commands(CLI& cli, Z80EX_CONTEXT* cpu, uint8_t* memory);  // Добавляем memory

#endif // COMMANDS_H