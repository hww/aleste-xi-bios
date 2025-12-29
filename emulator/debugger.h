#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include <z80ex.h>
#include <stdint.h>
#include <stdbool.h>

// Режимы работы дебаггера
typedef enum {
    DEBUG_MODE_RUN,      // Беспрерывное исполнение
    DEBUG_MODE_TRACE,    // Исполнение с паузами (трассировка)
    DEBUG_MODE_STEP,     // Пошаговое исполнение
    DEBUG_MODE_PAUSE     // Пауза
} DebugMode;

// Структура для хранения состояния процессора
typedef struct {
    // Регистры
    uint16_t pc;
    uint16_t sp;
    uint16_t af, bc, de, hl;
    uint16_t af_, bc_, de_, hl_;
    uint16_t ix, iy;
    uint8_t i, r, im;
    uint8_t iff1, iff2;
    
    // Дизассемблированная инструкция
    char instruction[32];
    
    // Флаги
    struct {
        uint8_t s:1, z:1, y:1, h:1, x:1, p:1, n:1, c:1;
    } flags;
} CPUState;

// Структура точки останова
typedef struct {
    uint16_t address;
    bool enabled;
    char description[32];
} Breakpoint;

// Главная структура дебаггера
typedef struct {
    DebugMode mode;
    
    // Точки останова
    Breakpoint breakpoints[16];
    int breakpoint_count;
    
    // История исполнения (кольцевой буфер)
    CPUState history[10];
    int history_index;
    int history_count;
    
    // Флаги состояния
    bool should_update_screen;
    int trace_delay_ms;
    
    // Статистика
    uint64_t total_cycles;
    uint64_t instruction_count;
} Debugger;

// ============ ОСНОВНОЙ ИНТЕРФЕЙС ============

// Инициализация
void debugger_init(Debugger *dbg);

// Управление исполнением
void debugger_run(Debugger *dbg);
void debugger_trace(Debugger *dbg);
void debugger_pause(Debugger *dbg);
void debugger_step(Debugger *dbg);

// Точки останова
bool debugger_add_breakpoint(Debugger *dbg, uint16_t address, const char *desc);
bool debugger_remove_breakpoint(Debugger *dbg, uint16_t address);
void debugger_list_breakpoints(const Debugger *dbg, char *buffer, int max_len);
bool debugger_check_breakpoint(const Debugger *dbg, uint16_t address);

// Захват состояния (аналог debug_capture_state)
void debugger_capture_state(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory);

// Компактный лог (аналог debug_log_compact)
void debugger_log_compact(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory);

// Утилиты
void debugger_print_status(const Debugger *dbg, Z80EX_CONTEXT *cpu, char *buffer, int max_len);
void debugger_print_registers(Z80EX_CONTEXT *cpu, char *buffer, int max_len);
void debugger_print_current_instruction(Z80EX_CONTEXT *cpu, uint8_t *memory, char *buffer, int max_len);

// Просмотр памяти
void debugger_memory_dump(uint8_t *memory, uint16_t start, uint16_t length, char *buffer, int max_len);
void debugger_disassemble_range(uint8_t *memory, uint16_t start, uint16_t end, char *buffer, int max_len);

// Обновление состояния (вызывать на каждом шаге CPU)
bool debugger_update(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory);

// Рендеринг в буфер (для экрана) - аналог debug_render_to_buffer
void debugger_render_to_buffer(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory, char *buffer, int max_len);

#endif