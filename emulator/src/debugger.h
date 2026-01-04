#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <z80ex.h>
#include <z80ex_dasm.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <array>
#include "utils/z88dk_sym_parser.h"

// Режимы работы дебаггера
enum class DebugMode {
    RUN,      // Беспрерывное исполнение
    TRACE,    // Исполнение с паузами (трассировка)
    STEP,     // Пошаговое исполнение
    STEP_OVER,// 
    STEP_OUT, // 
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
    bool temporary;
    std::array<char, 32> description;
    
    Breakpoint();
    Breakpoint(uint16_t addr, bool temp = false, const std::string& desc = "");
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

    Z80EX_CONTEXT* cpu_;
    uint8_t* memory_;
    Z88dkSymParser sym_parser_;    
    std::string current_sym_file_;
public:
    Debugger();

    void init(Z80EX_CONTEXT *cpu, uint8_t* memory) {
        cpu_ = cpu;
        memory_ = memory;
    }
    
    bool load_symbols(const std::string& path);
    std::string get_symbol_near(uint16_t addr);
    bool is_likely_address(uint16_t value) const;
    int z80ex_dasm_local(char *output, int output_size, unsigned flags,
                                         int *t_states, int *t_states2,
                                         z80ex_dasm_readbyte_cb readbyte_cb,
                                         Z80EX_WORD addr, void *user_data);
    int disassemble_instruction(uint16_t pc, uint8_t *memory, char *buffer, size_t buf_size);
    std::string format_disasm_line(uint16_t addr, bool show_address = true) ;


    Z80EX_CONTEXT* cpu() { return cpu_; }
    uint8_t* memory() {return memory_; }

    // Управление исполнением
    void run();
    void trace();
    void pause();
    void step();
    void step_over();
    void step_out();
    
    // Точки останова
    bool add_breakpoint(uint16_t address, bool temp, const std::string& desc = "");
    bool remove_breakpoint(uint16_t address);
    void remove_temp_breakpoint();
    std::string list_breakpoints() const;
    bool check_breakpoint(uint16_t address) const;
    int breakpoint_count() {return breakpoint_count_;}
    Breakpoint& breakpoint_at(int idx) { return breakpoints_[idx]; }
    void clear_breakpoint() { breakpoint_count_ = 0; }


    // Захват состояния
    CPUState* capture_state();
    
    // Компактный лог
    void log_compact();
    
    // Утилиты
    std::string print_status() const;
    std::string print_registers();
    int print_current_instruction(char *buffer, int max_len);
    
    // Просмотр памяти
    std::string memory_dump(uint16_t start, uint16_t length);
    std::string disassemble_range(uint16_t start, uint16_t end);
    std::string memory_wb_dump(uint32_t start, uint16_t length);

    // Обновление состояния
    bool update();
    
    // Рендеринг в буфер
    std::string render_to_buffer();
    
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
        case DebugMode::RUN:       return "RUN";     // Беспрерывное исполнение
        case DebugMode::TRACE:     return "TRACE";   // Исполнение с паузами (трассировка)
        case DebugMode::STEP:      return "STEP";    // Пошаговое исполнение
        case DebugMode::STEP_OVER: return "S-OVER";    // Пошаговое исполнение с обходом
        case DebugMode::STEP_OUT:  return "S-OUT";     // Пошаговое исполнение с обходом
        case DebugMode::PAUSE:     return "PAUSE";   // Пауза
        default: return "UNKNOWN";
        }
    };
};

#endif // DEBUGGER_HPP