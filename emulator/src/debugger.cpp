#include "debugger.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <z80ex.h>
#include <z80ex_dasm.h>
#include "utils/text_colors.h"
#include "utils/buffer_printer.h"
#include "utils/ibuffer.h"
#include "ports.h"
#include "memory.h"
#include <sstream>
#include <iomanip>

// ============ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ============
// Простая функция поиска hex адреса после префикса '#'
static std::string find_hex_after_hash(const std::string& line) {
    size_t pos = line.find('#');
    if (pos == std::string::npos) return "";
    
    // Проверяем что после '#' есть минимум 4 символа
    if (line.length() < pos + 5) return "";
    
    // Берем 4 символа после '#'
    std::string potential = line.substr(pos + 1, 4);
    
    // Проверяем что все символы hex
    for (char c : potential) {
        if (!isxdigit(c)) return "";
    }
    
    return potential;
}

// Простая конвертация
static uint16_t hex_to_uint16_simple(const std::string& hex) {
    uint16_t val = 0;
    
    for (char c : hex) {
        val = val * 16;
        
        if (c >= '0' && c <= '9') {
            val += (c - '0');
        } else if (c >= 'A' && c <= 'F') {
            val += (c - 'A' + 10);
        } else if (c >= 'a' && c <= 'f') {
            val += (c - 'a' + 10);
        }
    }
    
    return val;
}
// Функция для замены всех hex адресов на символы
static std::string replace_hex_with_symbols(const std::string& line, Z88dkSymParser& sym_parser) {
    std::string result = line;
    size_t pos = 0;
    
    while (true) {
        // Ищем следующий '#'
        pos = result.find('#', pos);
        if (pos == std::string::npos) break;
        
        // Проверяем что после '#' есть 4 hex символа
        if (result.length() < pos + 5) break;
        
        std::string hex_part = result.substr(pos + 1, 4);
        bool is_hex = true;
        for (char c : hex_part) {
            if (!isxdigit(c)) {
                is_hex = false;
                break;
            }
        }
        
        if (!is_hex) {
            pos++;
            continue;
        }
        
        // Конвертируем hex
        uint16_t addr = 0;
        for (char c : hex_part) {
            addr = addr * 16;
            if (c >= '0' && c <= '9') addr += (c - '0');
            else if (c >= 'A' && c <= 'F') addr += (c - 'A' + 10);
            else if (c >= 'a' && c <= 'f') addr += (c - 'a' + 10);
        }
        
        // Ищем символ
        std::string symbol = sym_parser.get_name(addr, 256);
        
        // Если нашли символ - заменяем
        if (!symbol.empty() && symbol.find('+') == std::string::npos) {
            // Заменяем "#xxxx" на "symbol@xxxx"
            std::string replacement = symbol + " @ #" + hex_part;
            result.replace(pos, 5, replacement);
            
            // Сдвигаем позицию после замены
            pos += replacement.length();
        } else {
            // Не нашли символ, идем дальше
            pos += 5;
        }
    }
    
    return result;
}

// Упрощенная версия функции
int Debugger::z80ex_dasm_local(char *output, int output_size, unsigned flags,
                               int *t_states, int *t_states2,
                               z80ex_dasm_readbyte_cb readbyte_cb,
                               Z80EX_WORD addr, void *user_data) {

    int cnt = z80ex_dasm(output, output_size, flags, t_states, t_states2,
                         readbyte_cb, addr, user_data);
    
    if (cnt > 0 && sym_parser_.is_loaded()) {
        std::string line = output;
        
        // Заменяем все hex адреса на символы
        std::string new_line = replace_hex_with_symbols(line, sym_parser_);
        
        // Копируем обратно
        if (new_line.length() < static_cast<size_t>(output_size)) {
            strcpy(output, new_line.c_str());
        } else {
            strncpy(output, new_line.c_str(), output_size - 1);
            output[output_size - 1] = '\0';
        }
    }
    
    return cnt;
}
// Функция для дизассемблирования инструкции returns bytes cunt in the instruction
int Debugger::disassemble_instruction(uint16_t pc, uint8_t *memory, char *buffer, size_t buf_size) {
    if (!buffer || buf_size == 0 || !memory) return 0;
    
    if (pc >= 0xFFFF) return 0;
    
    int t1, t2;
    int result = z80ex_dasm_local(buffer, buf_size - 1, 0, &t1, &t2, 
                           dasm_read_byte, pc, memory);
    return result;
}
// Метод для форматирования строки дизассемблирования
std::string Debugger::format_disasm_line(uint16_t addr, bool is_current) {
    std::ostringstream oss;
    
    // Дизассемблируем инструкцию
    std::array<char, 64> dasm_buffer;
    int t1, t2;
    int len = z80ex_dasm_local(dasm_buffer.data(), dasm_buffer.size(), 0, 
                               &t1, &t2, dasm_read_byte, addr, 0);
    
    if (len == 0) {
        if (is_current) {
            oss << " >??????: ???";
        } else {
            oss << "  ??????: ???";
        }
        return oss.str();
    }
    
    // Получаем физический адрес и метку
    auto full_addr = dasm_translate_address(addr, ECycle::MRD);
    auto label = sym_parser_.get_name(addr, 0);
    
    // Формируем строку байтов
    std::string bytes_str;
    for (int j = 0; j < len && j < 4; j++) {
        char byte_hex[4];
        auto data = dasm_read_byte(addr + j, 0);
        snprintf(byte_hex, sizeof(byte_hex), "%02X", data);
        bytes_str += byte_hex;
        if (j < len - 1) bytes_str += " ";
    }
    
    // Выравниваем до 8 символов
    while (bytes_str.length() < 8) {
        bytes_str += " ";
    }
    
    // Форматируем строку - ЕДИНЫЙ ФОРМАТ ДЛЯ ВСЕХ!
    // >C00000 метка           байты   инструкция
    //  C00010 другая_метка    байты   инструкция
    if (is_current) {
        oss << " >";
    } else {
        oss << "  ";
    }
    
    oss << std::hex << std::uppercase 
        << std::setw(6) << std::setfill('0') << full_addr << " ";
    
    // Метка: заполнитель ' ' (пробел)
    oss << std::setfill(' ') << std::setw(16) << std::left << label << " ";
    
    // Байты: заполнитель ' ' (пробел)
    oss << std::setw(8) << std::left << bytes_str << " ";
    
    oss << dasm_buffer.data();
    
    return oss.str();

}

// Функция для получения символа из байта
static char get_char_from_byte(uint8_t b) {
    return (b >= 32 && b < 127) ? static_cast<char>(b) : '.';
}

// Функция для получения байтов из памяти по адресу
static void get_bytes_at_address(uint16_t addr, uint8_t *bytes, int count) {
    for (int i = 0; i < count; i++) {
        bytes[i] = dasm_read_byte(addr + i, 0);
    }
}

// ============ CPUState ============

CPUState::CPUState() : 
    pc(0), sp(0), af(0), bc(0), de(0), hl(0),
    af_(0), bc_(0), de_(0), hl_(0), ix(0), iy(0),
    i(0), r(0), im(0), iff1(0), iff2(0), len(0) {
    instruction.fill(0);
    strncpy(instruction.data(), "---", instruction.size() - 1);
}

// ============ Breakpoint ============

Breakpoint::Breakpoint() : address(0), enabled(false), temporary(false) {
    description.fill(0);
}


Breakpoint::Breakpoint(uint16_t addr, bool temp, const std::string& desc) : 
    address(addr), enabled(true), temporary(temp) {
    description.fill(0);
    if (desc.empty()) {
        snprintf(description.data(), description.size(), "Breakpoint %04X", addr);
    } else {
        strncpy(description.data(), desc.c_str(), description.size() - 1);
    }
}

// ============ Debugger ============

Debugger::Debugger() : 
    mode_(DebugMode::PAUSE),
    breakpoint_count_(0),
    history_index_(0),
    history_count_(0),
    should_update_screen_(false),
    trace_delay_ms_(100),
    total_cycles_(0),
    instruction_count_(0),
    current_sym_file_(),
    sym_parser_()
    {
    
    breakpoints_.fill(Breakpoint());
    history_.fill(CPUState());
}

bool Debugger::load_symbols(const std::string& path) {
    if (sym_parser_.load_file(path)) {
        current_sym_file_ = path;
        std::cout << "Loaded symbols from: " << path << std::endl;
        return true;
    }
    return false;
}

// ============ УПРАВЛЕНИЕ ИСПОЛНЕНИЕМ ============

void Debugger::run() {
    mode_ = DebugMode::RUN;
    should_update_screen_ = true;
}

void Debugger::trace() {
    mode_ = DebugMode::TRACE;
    should_update_screen_ = true;
}

void Debugger::pause() {
    mode_ = DebugMode::PAUSE;
}

void Debugger::step() {
    mode_ = DebugMode::STEP;
}

void Debugger::step_out() {
    mode_ = DebugMode::STEP_OVER;
}

void Debugger::step_over() {
    
    uint16_t current_pc = z80ex_get_reg(cpu_, regPC);
    
    // Дизассемблируем текущую инструкцию
    std::array<char, 64> instruction;
    int len = disassemble_instruction(current_pc, memory_, instruction.data(), instruction.size());
    
    if (len == 0) {
        // Не удалось дизассемблировать, делаем обычный шаг
        mode_ = DebugMode::STEP;
        return;
    }
    
    // Проверяем, является ли текущая инструкция CALL или RST
    uint8_t opcode = dasm_read_byte(current_pc, 0);
    bool is_call_or_rst = false;
    
    // Проверяем CALL инструкции
    if (opcode == 0xCD ||                     // CALL nn
        (opcode & 0xC7) == 0xC4 ||           // CALL cc, nn
        (opcode == 0xDD && dasm_read_byte(current_pc + 1, 0) == 0xCD) ||  // CALL (IX)
        (opcode == 0xFD && dasm_read_byte(current_pc + 1, 0) == 0xCD)) {  // CALL (IY)
        is_call_or_rst = true;
    }
    
    // Проверяем RST инструкции
    if ((opcode & 0xC7) == 0xC7) {  // RST p
        is_call_or_rst = true;
    }
    
    if (is_call_or_rst) {
        // Это вызов - ставим временную точку останова на следующей инструкции
        uint16_t next_instr_address = current_pc + len;
        
        // Удаляем старую временную точку, если есть
        remove_temp_breakpoint();
        
        // Добавляем новую временную точку
        add_breakpoint(next_instr_address, true, "[TEMP] Step over breakpoint");
;
        
        // Переходим в режим RUN
        mode_ = DebugMode::RUN;
    } else {
        // Не вызов - просто делаем STEP
        mode_ = DebugMode::STEP;
    }
    
    should_update_screen_ = true;
}

// ============ ТОЧКИ ОСТАНОВА ============

bool Debugger::add_breakpoint(uint16_t address, bool temp, const std::string& desc) {
    if (breakpoint_count_ >= breakpoints_.size()) return false;
    
    // Проверяем, не существует ли уже такой точки
    for (size_t i = 0; i < breakpoint_count_; i++) {
        if (breakpoints_[i].address == address) {
            return false;
        }
    }
    
    // Добавляем новую точку
    breakpoints_[breakpoint_count_] = Breakpoint(address, temp, desc);
    breakpoint_count_++;
    return true;
}

bool Debugger::remove_breakpoint(uint16_t address) {
    for (size_t i = 0; i < breakpoint_count_; i++) {
        if (breakpoints_[i].address == address) {
            // Сдвигаем остальные точки
            for (size_t j = i; j < breakpoint_count_ - 1; j++) {
                breakpoints_[j] = breakpoints_[j + 1];
            }
            breakpoint_count_--;
            breakpoints_[breakpoint_count_] = Breakpoint();
            return true;
        }
    }
    
    return false;
}

void Debugger::remove_temp_breakpoint()
{
     for (size_t i = 0; i < breakpoint_count_; i++) {
        if (breakpoints_[i].temporary) {
            // Сдвигаем остальные точки
            for (size_t j = i; j < breakpoint_count_ - 1; j++) {
                breakpoints_[j] = breakpoints_[j + 1];
            }
            breakpoint_count_--;
            breakpoints_[breakpoint_count_] = Breakpoint();
        }
    }   
}

std::string Debugger::list_breakpoints() const {
    std::ostringstream oss;
    oss << "=== BREAKPOINTS ===\n";
    
    if (breakpoint_count_ == 0) {
        oss << "No breakpoints set\n";
        return oss.str();
    }
    
    for (size_t i = 0; i < breakpoint_count_; i++) {
        oss << std::setw(2) << (i + 1) << ". "
            << std::hex << std::setw(4) << std::setfill('0') 
            << breakpoints_[i].address << " ["
            << (breakpoints_[i].enabled ? "ENABLED" : "DISABLED") << "] - "
            << breakpoints_[i].description.data() << "\n";
    }
    
    return oss.str();
}

bool Debugger::check_breakpoint(uint16_t address) const {
    for (size_t i = 0; i < breakpoint_count_; i++) {
        if (breakpoints_[i].enabled && breakpoints_[i].address == address) {
            return true;
        }
    }
    
    return false;
}

// ============ ЗАХВАТ СОСТОЯНИЯ ============

CPUState* Debugger::capture_state() {
    
    CPUState &state = history_[history_index_];
    
    // Регистры
    state.pc    = z80ex_get_reg(cpu_, regPC);
    state.sp    = z80ex_get_reg(cpu_, regSP);
    state.af    = z80ex_get_reg(cpu_, regAF);
    state.bc    = z80ex_get_reg(cpu_, regBC);
    state.de    = z80ex_get_reg(cpu_, regDE);
    state.hl    = z80ex_get_reg(cpu_, regHL);
    state.af_   = z80ex_get_reg(cpu_, regAF_);
    state.bc_   = z80ex_get_reg(cpu_, regBC_);
    state.de_   = z80ex_get_reg(cpu_, regDE_);
    state.hl_   = z80ex_get_reg(cpu_, regHL_);
    state.ix    = z80ex_get_reg(cpu_, regIX);
    state.iy    = z80ex_get_reg(cpu_, regIY);
    state.i     = z80ex_get_reg(cpu_, regI);
    state.r     = z80ex_get_reg(cpu_, regR);
    state.im    = z80ex_get_reg(cpu_, regIM);
    state.iff1  = z80ex_get_reg(cpu_, regIFF1);
    state.iff2  = z80ex_get_reg(cpu_, regIFF2);
    
    // Флаги
    uint8_t f = state.af & 0xFF;
    state.flags.s = (f >> 7) & 1;
    state.flags.z = (f >> 6) & 1;
    state.flags.y = (f >> 5) & 1;
    state.flags.h = (f >> 4) & 1;
    state.flags.x = (f >> 3) & 1;
    state.flags.p = (f >> 2) & 1;
    state.flags.n = (f >> 1) & 1;
    state.flags.c = f & 1;
    
    // Используем новый метод для форматирования инструкции
    std::string disasm = format_disasm_line(state.pc, true);
    strncpy(state.instruction.data(), disasm.c_str(), state.instruction.size() - 1);
    state.instruction[state.instruction.size() - 1] = '\0';
    
    // Получаем длину инструкции (находим первый пробел в байтах)
    size_t space_pos = disasm.find(' ');
    if (space_pos != std::string::npos) {
        // Подсчитываем количество байтов
        std::string bytes_part = disasm.substr(0, space_pos);
        state.len = (std::count(bytes_part.begin(), bytes_part.end(), ' ') + 1) / 2;
    } else {
        state.len = 1; // минимальная длина
    }
    
    // Сдвигаем кольцевой буфер
    history_index_ = (history_index_ + 1) % history_.size();
    if (history_count_ < history_.size()) {
        history_count_++;
    }
    
    return &state;
}

// ============ КОМПАКТНЫЙ ЛОГ ============
void Debugger::log_compact() {
    uint16_t pc = z80ex_get_reg(cpu_, regPC);
    uint16_t sp = z80ex_get_reg(cpu_, regSP);
    uint16_t af = z80ex_get_reg(cpu_, regAF);
    uint16_t bc = z80ex_get_reg(cpu_, regBC);
    uint16_t de = z80ex_get_reg(cpu_, regDE);
    uint16_t hl = z80ex_get_reg(cpu_, regHL);
    
    // Флаги из AF
    uint8_t f = af & 0xFF;
    std::string flags(8, ' ');
    flags[0] = (f & 0x80) ? 'S' : '.';
    flags[1] = (f & 0x40) ? 'Z' : '.';
    flags[2] = (f & 0x20) ? 'Y' : '.';
    flags[3] = (f & 0x10) ? 'H' : '.';
    flags[4] = (f & 0x08) ? 'X' : '.';
    flags[5] = (f & 0x04) ? 'P' : '.';
    flags[6] = (f & 0x02) ? 'N' : '.';
    flags[7] = (f & 0x01) ? 'C' : '.';
    
    // Используем тот же метод для дизассемблирования
    std::string disasm_line = format_disasm_line(pc, false);
    
    // Убираем лишние пробелы в начале (префикс "  " или " >")
    std::string clean_disasm = disasm_line;
    if (clean_disasm.size() > 2 && clean_disasm[0] == ' ' && 
        (clean_disasm[1] == ' ' || clean_disasm[1] == '>')) {
        clean_disasm = clean_disasm.substr(2);
    }
    ibuffer.println(disasm_line.c_str());
    // Выводим дизассемблированную строку и регистры справа
    //ibuffer.printf("%-48s | PC:%04X SP:%04X AF:%04X BC:%04X DE:%04X HL:%04X | %s\n",
    //       clean_disasm.c_str(),
    //       pc, sp, af, bc, de, hl, flags.c_str());
}

// ============ УТИЛИТЫ ============

std::string Debugger::print_registers() {
    
    uint16_t pc  = z80ex_get_reg(cpu_, regPC);
    uint16_t sp  = z80ex_get_reg(cpu_, regSP);
    uint16_t af  = z80ex_get_reg(cpu_, regAF);
    uint16_t bc  = z80ex_get_reg(cpu_, regBC);
    uint16_t de  = z80ex_get_reg(cpu_, regDE);
    uint16_t hl  = z80ex_get_reg(cpu_, regHL);
    uint16_t ix  = z80ex_get_reg(cpu_, regIX);
    uint16_t iy  = z80ex_get_reg(cpu_, regIY);
    uint16_t af_ = z80ex_get_reg(cpu_, regAF_);
    uint16_t bc_ = z80ex_get_reg(cpu_, regBC_);
    uint16_t de_ = z80ex_get_reg(cpu_, regDE_);
    uint16_t hl_ = z80ex_get_reg(cpu_, regHL_);
    
    // Флаги
    uint8_t f = af & 0xFF;
    std::string flags(8, ' ');
    flags[0] = (f & 0x80) ? 'S' : '.';
    flags[1] = (f & 0x40) ? 'Z' : '.';
    flags[2] = (f & 0x20) ? 'Y' : '.';
    flags[3] = (f & 0x10) ? 'H' : '.';
    flags[4] = (f & 0x08) ? 'X' : '.';
    flags[5] = (f & 0x04) ? 'P' : '.';
    flags[6] = (f & 0x02) ? 'N' : '.';
    flags[7] = (f & 0x01) ? 'C' : '.';
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << "PC:" << std::setw(4) << pc 
        << " SP:" << std::setw(4) << sp 
        << " AF:" << std::setw(4) << af 
        << " BC:" << std::setw(4) << bc 
        << " DE:" << std::setw(4) << de 
        << " HL:" << std::setw(4) << hl << "\n"
        << "IX:" << std::setw(4) << ix 
        << " IY:" << std::setw(4) << iy 
        << " AF':" << std::setw(4) << af_ 
        << " BC':" << std::setw(4) << bc_ 
        << " DE':" << std::setw(4) << de_ 
        << " HL':" << std::setw(4) << hl_ << "\n"
        << "FLAGS: " << flags;
    
    ibuffer.println(oss.str());
    return oss.str();
}

int Debugger::print_current_instruction(char *buffer, int max_len) {
    if (!cpu_ || !memory_ || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return 0;
    }
    
    uint16_t pc = z80ex_get_reg(cpu_, regPC);
    std::array<char, 64> dasm_buffer;
    int t1, t2;
    
    int len = z80ex_dasm_local(dasm_buffer.data(), dasm_buffer.size(), 0, &t1, &t2,
                         dasm_read_byte, pc, memory_);
    
    snprintf(buffer, max_len, "[%04X] %s", pc, dasm_buffer.data());
    return len;
}

std::string Debugger::print_status() const {
    if (!cpu_) return "";
    
    const char *mode_str = mode_to_str(mode_);
    
    std::ostringstream oss;
    oss << "=== DEBUGGER STATUS ===\n"
        << "Mode: " << mode_str << "\n"
        << "Instructions: " << instruction_count_ 
        << "  Cycles: " << total_cycles_ << "\n"
        << "Breakpoints: " << breakpoint_count_;
    ibuffer.println(oss.str());

    return oss.str();
}

// ============ ПРОСМОТР ПАМЯТИ ============
std::string Debugger::memory_dump(uint16_t start, uint16_t length) {
    auto start_mmu = dasm_translate_address(start, ECycle::MRD);
    return memory_wb_dump(start_mmu, length);
}
std::string Debugger::memory_wb_dump(uint32_t start, uint16_t length) {
    if (!memory_) return "";
    
    std::ostringstream oss;
    oss << "=== MEMORY DUMP " 
        << std::hex << std::setw(6) << std::setfill('0') << start 
        << "-" << std::setw(6) << (start + length - 1) << " ===\n";
    
    for (uint16_t i = 0; i < length; i += 16) {
        // Адрес
        oss << std::hex << std::setw(6) << std::setfill('0') << (start + i) << ": ";
        
        // Hex dump
        for (int j = 0; j < 16; j++) {
            if (i + j < length) {
                oss << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(memory_[start + i + j]) << " ";
            } else {
                oss << "   ";
            }
        }
        
        // ASCII dump
        oss << " ";
        for (int j = 0; j < 16; j++) {
            if (i + j < length) {
                uint8_t c = memory_[start + i + j];
                oss << ((c >= 32 && c < 127) ? static_cast<char>(c) : '.');
            }
        }
        
        if (i + 16 < length) oss << "\n";
    }
    return oss.str();
}

std::string Debugger::disassemble_range(uint16_t start, uint16_t dislen) {
    if (dislen <= 0) return "";
    auto end = start + dislen;
    std::ostringstream oss;
    oss << "=== DISASSEMBLY " 
        << std::hex << std::setw(6) << std::setfill('0') << start 
        << "-" << std::setw(6) << end << " ===\n";
    
    uint16_t addr = start;
    while (addr < end) {
        std::array<char, 64> dasm_buffer;
        int t1, t2;
        
        int len = z80ex_dasm_local(dasm_buffer.data(), dasm_buffer.size(), 0, &t1, &t2,
                            dasm_read_byte, addr, memory_);
        
        if (len == 0) break;
        
        // Адрес
        oss << std::hex << std::setw(4) << std::setfill('0') << addr << ": ";
        
        // Байты инструкции
        for (int i = 0; i < len; i++) {
            oss << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(memory_[addr + i]) << " ";
        }
        
        // Выравнивание
        for (int i = len; i < 4; i++) {
            oss << "   ";
        }
        
        // Инструкция
        oss << " " << dasm_buffer.data() << "\n";
        
        addr += len;
    }
    return oss.str();
}

// ============ ОБНОВЛЕНИЕ СОСТОЯНИЯ ============

bool Debugger::update() {
    
    // Если в режиме паузы - ничего не делаем
    if (mode_ == DebugMode::PAUSE) {
        return false;
    }
    
    uint16_t pc = z80ex_get_reg(cpu_, regPC);
    
    // Проверяем точки останова
    if (check_breakpoint(pc)) {
        mode_ = DebugMode::PAUSE;
        printf("\n*** BREAKPOINT HIT at %04X ***\n", pc);
        should_update_screen_ = true;
        return true;
    }
    
    // В режиме STEP: захватываем текущее состояние, выполняем одну инструкцию и переходим в паузу
    if (mode_ == DebugMode::STEP ) {
        // Сохраняем состояние ДО выполнения инструкции
        auto state_ptr = capture_state();
        
        // Выполняем одну инструкцию
        int cycles = z80ex_step(cpu_);
        total_cycles_ += cycles;
        instruction_count_++;
        
        // Показываем инструкцию в консоли
        log_compact();
        
        mode_ = DebugMode::PAUSE;
        should_update_screen_ = true;



        return true;
    }
    
    // exit from function
    if (mode_ == DebugMode::STEP_OUT) {
        // Сохраняем состояние ДО выполнения инструкции
        auto state_ptr = capture_state();
        
        // Выполняем одну инструкцию
        int cycles = z80ex_step(cpu_);
        total_cycles_ += cycles;
        instruction_count_++;
        
        // step out waiting the RET instruction and switch to 
        if (state_ptr == NULL || state_ptr->i == 0xC9) {
            should_update_screen_ = true;
            mode_ == DebugMode::STEP;
        }
        return true;
    }

    if (mode_ == DebugMode::STEP_OVER) {
        // Сохраняем состояние ДО выполнения инструкции
        capture_state();
        
        // Выполняем одну инструкцию
        int cycles = z80ex_step(cpu_);
        total_cycles_ += cycles;
        instruction_count_++;
        
        // В режиме TRACE показываем инструкцию
        log_compact();
        should_update_screen_ = true;

        return true;
    }
    
    // В режиме TRACE: захватываем состояние, выполняем инструкцию, но остаемся в TRACE
    if (mode_ == DebugMode::TRACE) {
        // Сохраняем состояние ДО выполнения инструкции
        capture_state();
        
        // Выполняем одну инструкцию
        int cycles = z80ex_step(cpu_);
        total_cycles_ += cycles;
        instruction_count_++;
        
        // В режиме TRACE показываем инструкцию
        log_compact();
        should_update_screen_ = true;

        return true;
    }
  

    // Режим RUN
    if (mode_ == DebugMode::RUN)  {
        for (int i = 0; i < 2000; i++) {
            int cycles = z80ex_step(cpu_);
            total_cycles_ += cycles;
            instruction_count_++;
        }
        return true;
    }

    return false;
}

bool Debugger::is_likely_address(uint16_t value) const {
    // Исключаем очевидные не-адреса
    if (value == 0) return false;
    
    // Проверяем диапазоны памяти (для Z80 обычно 0x0000-0xFFFF)
    // Но для вашей системы с MMU - может быть иначе
    if (value > 0xFFFF) return false;  // 16-битный адрес
    
    // Дополнительные проверки:
    // 1. Адрес выровнен? (необязательно, но может помочь)
    // 2. Попадает ли в области памяти программы?
    
    // Простая эвристика: если в символе есть точное совпадение или близкое
    // ИЛИ адрес в диапазоне, где обычно размещается код/данные
    uint16_t aligned = value & 0xFFF0; // Выровнять по 16 байтам
    
    // Проверяем основные диапазоны вашей системы:
    // 0x0000-0x3FFF - обычно ROM/BIOS
    // 0x4000-0x7FFF - обычно RAM
    // 0x8000-0xBFFF - обычно RAM
    // 0xC000-0xFFFF - обычно RAM/видео/порты
    
    // Для начала просто возвращаем true для всех не-нулевых значений
    // так как false-отрицательные хуже false-положительных
    return value != 0;
}

std::string Debugger::get_symbol_near(uint16_t addr) {
    if (!is_likely_address(addr)) {
        return "";
    }
    
    // Ищем точное совпадение
    auto symbol = sym_parser_.get_symbol(addr);
    if (symbol) {
        return symbol->name;
    }
    
    // Ищем близкое совпадение (в пределах функции)
    std::string near_sym = sym_parser_.get_name(addr, 256); // ±256 байт
    
    // Фильтруем мусор - слишком длинные имена могут быть ошибочными
    if (!near_sym.empty()) {
        // Проверяем, не слишком ли большое смещение?
        // Формат может быть "имя+число" или "имя-число"
        size_t plus_pos = near_sym.find('+');
        size_t minus_pos = near_sym.find('-');
        
        if (plus_pos != std::string::npos || minus_pos != std::string::npos) {
            // Есть смещение, проверяем его величину
            std::string offset_str;
            if (plus_pos != std::string::npos) {
                offset_str = near_sym.substr(plus_pos + 1);
            } else {
                offset_str = near_sym.substr(minus_pos + 1);
            }
            
            try {
                int offset = std::stoi(offset_str);
                // Если смещение слишком большое - игнорируем
                if (abs(offset) > 1024) { // 1KB - разумный предел для функции
                    return "";
                }
            } catch (...) {
                // Не смогли распарсить число
            }
        }
    }
    
    return near_sym;
}
// ============ РЕНДЕРИНГ В БУФЕР ============

std::string Debugger::render_to_buffer() {
    const size_t BUFFER_SIZE = 2048;
    std::array<char, BUFFER_SIZE> buffer;
    BufferPrinter bp;
    bp_init(&bp, buffer.data(), buffer.size());
    
    // Получаем текущие регистры
    uint16_t pc  = z80ex_get_reg(cpu_, regPC);
    uint16_t sp  = z80ex_get_reg(cpu_, regSP);
    uint16_t af  = z80ex_get_reg(cpu_, regAF);
    uint16_t bc  = z80ex_get_reg(cpu_, regBC);
    uint16_t de  = z80ex_get_reg(cpu_, regDE);
    uint16_t hl  = z80ex_get_reg(cpu_, regHL);
    uint16_t ix  = z80ex_get_reg(cpu_, regIX);
    uint16_t iy  = z80ex_get_reg(cpu_, regIY);
    uint16_t af_ = z80ex_get_reg(cpu_, regAF_);
    uint16_t bc_ = z80ex_get_reg(cpu_, regBC_);
    uint16_t de_ = z80ex_get_reg(cpu_, regDE_);
    uint16_t hl_ = z80ex_get_reg(cpu_, regHL_);
    uint8_t i    = z80ex_get_reg(cpu_, regI);
    uint8_t r    = z80ex_get_reg(cpu_, regR);
    uint8_t im   = z80ex_get_reg(cpu_, regIM);
    uint8_t iff1 = z80ex_get_reg(cpu_, regIFF1);
    uint8_t iff2 = z80ex_get_reg(cpu_, regIFF2);
    
    // Получаем флаги
    uint8_t f = af & 0xFF;
    std::array<char, 9> flags = {
        (f & 0x80) ? 'S' : 's',
        (f & 0x40) ? 'Z' : 'z',
        (f & 0x20) ? '5' : 'x',
        (f & 0x10) ? 'H' : 'h',
        (f & 0x08) ? '3' : 'x',
        (f & 0x04) ? 'P' : 'p',
        (f & 0x02) ? 'N' : 'n',
        (f & 0x01) ? 'C' : 'c',
        '\0'
    };
    
    // Получаем байты для регистров, указывающих на память
    std::array<uint8_t, 2> sp_bytes, bc_bytes, de_bytes, hl_bytes, ix_bytes, iy_bytes;
    get_bytes_at_address(sp, sp_bytes.data(), 2);
    get_bytes_at_address(bc, bc_bytes.data(), 2);
    get_bytes_at_address(de, de_bytes.data(), 2);
    get_bytes_at_address(hl, hl_bytes.data(), 2);
    get_bytes_at_address(ix, ix_bytes.data(), 2);
    get_bytes_at_address(iy, iy_bytes.data(), 2);
    
    // Статус дебаггера
    bp_println(&bp, "");
    
    const char *mode_str;
    switch (mode_) {
        case DebugMode::RUN: mode_str = "RUN"; break;
        case DebugMode::TRACE: mode_str = "TRACE"; break;
        case DebugMode::STEP: mode_str = "STEP"; break;
        case DebugMode::PAUSE: mode_str = "PAUSE"; break;
        default: mode_str = "UNKNOWN";
    }
    
    bp_println_color(&bp, COLOR_CYAN, ">> XI Aleste DBG | MODE: %s | BP: %zu | INS: %llu SYM: `%s`\n",
                   mode_str, breakpoint_count_, instruction_count_, current_sym_file_.c_str());

    bp_println_color(&bp, COLOR_LIME, "SLT %02X USER %02X SUPER %02X", 
                   g_mmu.get_current_slot_reg(), 
                   g_mmu.get_user_slot_reg(), 
                   g_mmu.get_super_slot_reg());

    bp_println_color(&bp, COLOR_LIME, "MAP %01X:%02X %01X:%02X %01X:%02X %01X:%02X", 
                   g_mmu.get_current_slot(0x0000, ECycle::MRD), g_mmu.get_current_bank(0x0000, ECycle::MRD), 
                   g_mmu.get_current_slot(0x4000, ECycle::MRD), g_mmu.get_current_bank(0x4000, ECycle::MRD), 
                   g_mmu.get_current_slot(0x8000, ECycle::MRD), g_mmu.get_current_bank(0x8000, ECycle::MRD), 
                   g_mmu.get_current_slot(0xC000, ECycle::MRD), g_mmu.get_current_bank(0xC000, ECycle::MRD)); 

    // 1. Первая строка: PC и состояние прерываний
    const char *di_ei = (iff1 == 0) ? "DI" : "EI";
    auto pc_sym = get_symbol_near(pc); 
    bp_println_color(&bp, COLOR_RED, "PC  %04X %s I=%02X %s", pc, di_ei, i, pc_sym.c_str());
    
    // 2. Регистр SP с байтами
    bp_println_color(&bp, COLOR_GRAY_C0, "SP  %04X %02X%02X %c%c", 
                   sp, sp_bytes[0], sp_bytes[1],
                   get_char_from_byte(sp_bytes[0]),
                   get_char_from_byte(sp_bytes[1]));
    
    // 3. Регистр A с флагами
    bp_println_color(&bp, COLOR_WHITE, " A  %02X %c %s",
                   (af >> 8) & 0xFF,
                   get_char_from_byte((af >> 8) & 0xFF),
                   flags.data());

    // 4. Регистры BC, DE, HL, IX
    auto bc_sym = get_symbol_near(bc); 
    bp_println_color(&bp, COLOR_WHITE, "BC  %04X %02X%02X %c%c %s",
                bc, bc_bytes[0], bc_bytes[1],
                get_char_from_byte(bc_bytes[0]),
                get_char_from_byte(bc_bytes[1]),
                bc_sym.c_str());

    auto de_sym = get_symbol_near(de); 
    bp_println_color(&bp, COLOR_WHITE, "DE  %04X %02X%02X %c%c %s",
                de, de_bytes[0], de_bytes[1],
                get_char_from_byte(de_bytes[0]),
                get_char_from_byte(de_bytes[1]),
                de_sym.c_str());

    auto hl_sym = get_symbol_near(hl); 
    bp_println_color(&bp, COLOR_WHITE, "HL  %04X %02X%02X %c%c %s",
                hl, hl_bytes[0], hl_bytes[1],
                get_char_from_byte(hl_bytes[0]),
                get_char_from_byte(hl_bytes[1]),
                hl_sym.c_str());

    auto ix_sym = get_symbol_near(ix); 
    bp_println_color(&bp, COLOR_WHITE, "IX  %04X %02X%02X %c%c %s",
                ix, ix_bytes[0], ix_bytes[1],
                get_char_from_byte(ix_bytes[0]),
                get_char_from_byte(ix_bytes[1]),
                ix_sym.c_str());

    // 5. Альтернативные регистры
    std::array<uint8_t, 2> af_bytes, bc__bytes, de__bytes, hl__bytes;
    get_bytes_at_address(af_, af_bytes.data(), 2);
    get_bytes_at_address(bc_, bc__bytes.data(), 2);
    get_bytes_at_address(de_, de__bytes.data(), 2);
    get_bytes_at_address(hl_, hl__bytes.data(), 2);
    
    uint8_t f_ = af_ & 0xFF;
    std::array<char, 9> flags_ = {
        (f_ & 0x80) ? 'S' : 's',
        (f_ & 0x40) ? 'Z' : 'z',
        (f_ & 0x20) ? '5' : 'x',
        (f_ & 0x10) ? 'H' : 'h',
        (f_ & 0x08) ? '3' : 'x',
        (f_ & 0x04) ? 'P' : 'p',
        (f_ & 0x02) ? 'N' : 'n',
        (f_ & 0x01) ? 'C' : 'c',
        '\0'
    };
    
    bp_println_color(&bp, COLOR_GRAY_C0, " A` %02X %c %s",
                   (af_ >> 8) & 0xFF,
                   get_char_from_byte((af_ >> 8) & 0xFF),
                   flags_.data());
    
    auto bc_sym_ = get_symbol_near(bc); 
    bp_println_color(&bp, COLOR_GRAY_C0, "BC` %04X %02X%02X %c%c %s",
                   bc_, bc__bytes[0], bc__bytes[1],
                   get_char_from_byte(bc__bytes[0]),
                   get_char_from_byte(bc__bytes[1]),
                   bc_sym_.c_str()
                );

    auto de_sym_ = get_symbol_near(de); 
    bp_println_color(&bp, COLOR_GRAY_C0, "DE` %04X %02X%02X %c%c %s",
                   de_, de__bytes[0], de__bytes[1],
                   get_char_from_byte(de__bytes[0]),
                   get_char_from_byte(de__bytes[1]),
                   de_sym_.c_str());

    auto hl_sym_ = get_symbol_near(hl); 
    bp_println_color(&bp, COLOR_GRAY_C0, "HL` %04X %02X%02X %c%c %s",
                   hl_, hl__bytes[0], hl__bytes[1],
                   get_char_from_byte(hl__bytes[0]),
                   get_char_from_byte(hl__bytes[1]),
                   hl_sym_.c_str());

    bp_println(&bp, ""); // Пустая строка
    
    // 6. Дизассемблер
    bp_println_color(&bp, COLOR_CYAN, "DISASSEMBLER:");
    
// История - 2 предыдущие инструкции
    if (history_count_ > 0) {
        int history_to_show = std::min(2, static_cast<int>(history_count_));
        
        for (int i = 0; i < history_to_show; i++) {
            int idx = (static_cast<int>(history_index_) - history_to_show + i) % 10;
            if (idx < 0) idx += 10;
            
            CPUState &hist_entry = history_[idx];
            
            if (hist_entry.pc == 0) continue;
            
            // Для истории используем is_current=false
            std::string hist_line = format_disasm_line(hist_entry.pc, false);
            bp_println(&bp, "%s", hist_line.c_str());
        }
    }
    
    // Текущая инструкция
    uint16_t current_pc = z80ex_get_reg(cpu_, regPC);
    std::string current_line = format_disasm_line(current_pc, true);
    bp_println_color(&bp, COLOR_YELLOW, "%s", current_line.c_str());
    
    // Следующие инструкции
    std::array<char, 64> dasm_buffer;
    int t1, t2;
    int current_len = z80ex_dasm_local(dasm_buffer.data(), dasm_buffer.size(), 0, 
                                       &t1, &t2, dasm_read_byte, current_pc, 0);
    
    if (current_len > 0) {
        uint16_t next_pc = current_pc + current_len;
        int next_to_show = 4;
        
        for (int i = 0; i < next_to_show; i++) {
            // Для следующих инструкций используем is_current=false
            std::string next_line = format_disasm_line(next_pc, false);
            
            // Получаем длину следующей инструкции
            int next_len = z80ex_dasm_local(dasm_buffer.data(), dasm_buffer.size(), 0, 
                                           &t1, &t2, dasm_read_byte, next_pc, 0);
            if (next_len == 0) break;
            
            bp_println(&bp, "%s", next_line.c_str());
            next_pc += next_len;
        }
    }
    
    // Завершаем строку нуль-терминатором
    if (bp.position < bp.capacity) {
        bp.buffer[bp.position] = '\0';
    } else if (bp.capacity > 0) {
        bp.buffer[bp.capacity - 1] = '\0';
    }
    
    return std::string(buffer.data());
}