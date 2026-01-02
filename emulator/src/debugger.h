#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <z80ex.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <array>

// Режимы работы дебаггера
enum class DebugMode {
    RUN,      // Беспрерывное исполнение
    TRACE,    // Исполнение с паузами (трассировка)
    STEP,     // Пошаговое исполнение
    PAUSE     // Пауза
};

// Структура для хранения состояния процессора
struct CPUState {
    // Регистры
    uint16_t pc;
    uint16_t sp;
    uint16_t af, bc, de, hl;
    uint16_t af_, bc_, de_, hl_;
    uint16_t ix, iy;
    uint8_t i, r, im;
    uint8_t iff1, iff2;
    
    // Дизассемблированная инструкция
    std::array<char, 32> instruction;
    
    // Флаги
    struct {
        uint8_t s:1, z:1, y:1, h:1, x:1, p:1, n:1, c:1;
    } flags;

    int len;
    
    CPUState();
};

// Структура точки останова
struct Breakpoint {
    uint16_t address;
    bool enabled;
    std::array<char, 32> description;
    
    Breakpoint();
    Breakpoint(uint16_t addr, const std::string& desc = "");
};

// Главный класс дебаггера
class Debugger {
private:
    DebugMode mode_;
    
    // Точки останова
    std::array<Breakpoint, 16> breakpoints_;
    size_t breakpoint_count_;
    
    // История исполнения (кольцевой буфер)
    std::array<CPUState, 10> history_;
    size_t history_index_;
    size_t history_count_;
    
    // Флаги состояния
    bool should_update_screen_;
    int trace_delay_ms_;
    
    // Статистика
    uint64_t total_cycles_;
    uint64_t instruction_count_;
    
public:
    Debugger();
    
    // Управление исполнением
    void run();
    void trace();
    void pause();
    void step();
    
    // Точки останова
    bool add_breakpoint(uint16_t address, const std::string& desc = "");
    bool remove_breakpoint(uint16_t address);
    std::string list_breakpoints() const;
    bool check_breakpoint(uint16_t address) const;
    int breakpoint_count() {return breakpoint_count_;}
    Breakpoint& breakpoint_at(int idx) { return breakpoints_[idx]; }
    void clear_breakpoint() { breakpoint_count_ = 0; }
    // Захват состояния
    void capture_state(Z80EX_CONTEXT *cpu, uint8_t *memory);
    
    // Компактный лог
    void log_compact(Z80EX_CONTEXT *cpu, uint8_t *memory);
    
    // Утилиты
    std::string print_status(Z80EX_CONTEXT *cpu) const;
    static std::string print_registers(Z80EX_CONTEXT *cpu);
    static int print_current_instruction(Z80EX_CONTEXT *cpu, uint8_t *memory, char *buffer, int max_len);
    
    // Просмотр памяти
    static std::string memory_dump(uint8_t *memory, uint16_t start, uint16_t length);
    static std::string disassemble_range(uint8_t *memory, uint16_t start, uint16_t end);
    static std::string memory_wb_dump(uint8_t *memory, uint32_t start, uint16_t length);
    
    // Обновление состояния
    bool update(Z80EX_CONTEXT *cpu, uint8_t *memory);
    
    // Рендеринг в буфер
    std::string render_to_buffer(Z80EX_CONTEXT *cpu, uint8_t *memory);
    
    // Геттеры
    DebugMode mode() const { return mode_; }
    size_t breakpoint_count() const { return breakpoint_count_; }
    uint64_t total_cycles() const { return total_cycles_; }
    uint64_t instruction_count() const { return instruction_count_; }
    bool should_update_screen() const { return should_update_screen_; }
    
    // Сеттеры
    void set_trace_delay(int delay_ms) { trace_delay_ms_ = delay_ms; }
    void set_should_update_screen(bool update) { should_update_screen_ = update; }

    static const char* mode_to_str(DebugMode mode) {
        switch (mode) 
        {
        case DebugMode::RUN:      return "RUN";     // Беспрерывное исполнение
        case DebugMode::TRACE:    return "TRACE";   // Исполнение с паузами (трассировка)
        case DebugMode::STEP:     return "STEP";    // Пошаговое исполнение
        case DebugMode::PAUSE:    return "PAUSE";   // Пауза
        default: return "UNKNOWN";
        }
    };
};

#endif // DEBUGGER_HPP