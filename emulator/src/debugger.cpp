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

// Функция для дизассемблирования инструкции returns bytes cunt in the instruction
static int disassemble_instruction(uint16_t pc, uint8_t *memory, char *buffer, size_t buf_size) {
    if (!buffer || buf_size == 0 || !memory) return 0;
    
    if (pc >= 0xFFFF) return 0;
    
    int t1, t2;
    int result = z80ex_dasm(buffer, buf_size - 1, 0, &t1, &t2, 
                           dasm_read_byte, pc, memory);
    
    return result;
}


// Функция для получения символа из байта
static char get_char_from_byte(uint8_t b) {
    return (b >= 32 && b < 127) ? static_cast<char>(b) : '.';
}

// Функция для получения байтов из памяти по адресу
static void get_bytes_at_address(uint8_t *memory, uint16_t addr, uint8_t *bytes, int count) {
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

Breakpoint::Breakpoint() : address(0), enabled(false) {
    description.fill(0);
}

Breakpoint::Breakpoint(uint16_t addr, const std::string& desc) : 
    address(addr), enabled(true) {
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
    instruction_count_(0) {
    
    breakpoints_.fill(Breakpoint());
    history_.fill(CPUState());
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

// ============ ТОЧКИ ОСТАНОВА ============

bool Debugger::add_breakpoint(uint16_t address, const std::string& desc) {
    if (breakpoint_count_ >= breakpoints_.size()) return false;
    
    // Проверяем, не существует ли уже такой точки
    for (size_t i = 0; i < breakpoint_count_; i++) {
        if (breakpoints_[i].address == address) {
            return false;
        }
    }
    
    // Добавляем новую точку
    breakpoints_[breakpoint_count_] = Breakpoint(address, desc);
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

void Debugger::capture_state(Z80EX_CONTEXT *cpu, uint8_t *memory) {
    if (!cpu || !memory) return;
    
    CPUState &state = history_[history_index_];
    
    // Регистры
    state.pc = z80ex_get_reg(cpu, regPC);
    state.sp = z80ex_get_reg(cpu, regSP);
    state.af = z80ex_get_reg(cpu, regAF);
    state.bc = z80ex_get_reg(cpu, regBC);
    state.de = z80ex_get_reg(cpu, regDE);
    state.hl = z80ex_get_reg(cpu, regHL);
    state.af_ = z80ex_get_reg(cpu, regAF_);
    state.bc_ = z80ex_get_reg(cpu, regBC_);
    state.de_ = z80ex_get_reg(cpu, regDE_);
    state.hl_ = z80ex_get_reg(cpu, regHL_);
    state.ix = z80ex_get_reg(cpu, regIX);
    state.iy = z80ex_get_reg(cpu, regIY);
    state.i = z80ex_get_reg(cpu, regI);
    state.r = z80ex_get_reg(cpu, regR);
    state.im = z80ex_get_reg(cpu, regIM);
    state.iff1 = z80ex_get_reg(cpu, regIFF1);
    state.iff2 = z80ex_get_reg(cpu, regIFF2);
    
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
    
    // Дизассемблируем текущую инструкцию
    state.len = disassemble_instruction(state.pc, memory, state.instruction.data(), state.instruction.size());
    
    // Сдвигаем кольцевой буфер
    history_index_ = (history_index_ + 1) % history_.size();
    if (history_count_ < history_.size()) {
        history_count_++;
    }
}

// ============ КОМПАКТНЫЙ ЛОГ ============

void Debugger::log_compact(Z80EX_CONTEXT *cpu, uint8_t *memory) {
    if (!cpu) return;
    
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    uint16_t sp = z80ex_get_reg(cpu, regSP);
    uint16_t af = z80ex_get_reg(cpu, regAF);
    uint16_t bc = z80ex_get_reg(cpu, regBC);
    uint16_t de = z80ex_get_reg(cpu, regDE);
    uint16_t hl = z80ex_get_reg(cpu, regHL);
    
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
    
    // Дизассемблируем текущую инструкцию
    std::array<char, 32> instruction;
    disassemble_instruction(pc, memory, instruction.data(), instruction.size());
    
    ibuffer.printf("[%04X] %-20s | PC:%04X SP:%04X AF:%04X BC:%04X DE:%04X HL:%04X | %s\n",
           pc, instruction.data(),
           pc, sp, af, bc, de, hl, flags.c_str());
}

// ============ УТИЛИТЫ ============

std::string Debugger::print_registers(Z80EX_CONTEXT *cpu) {
    if (!cpu) return "";
    
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    uint16_t sp = z80ex_get_reg(cpu, regSP);
    uint16_t af = z80ex_get_reg(cpu, regAF);
    uint16_t bc = z80ex_get_reg(cpu, regBC);
    uint16_t de = z80ex_get_reg(cpu, regDE);
    uint16_t hl = z80ex_get_reg(cpu, regHL);
    uint16_t ix = z80ex_get_reg(cpu, regIX);
    uint16_t iy = z80ex_get_reg(cpu, regIY);
    uint16_t af_ = z80ex_get_reg(cpu, regAF_);
    uint16_t bc_ = z80ex_get_reg(cpu, regBC_);
    uint16_t de_ = z80ex_get_reg(cpu, regDE_);
    uint16_t hl_ = z80ex_get_reg(cpu, regHL_);
    
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

int Debugger::print_current_instruction(Z80EX_CONTEXT *cpu, uint8_t *memory, char *buffer, int max_len) {
    if (!cpu || !memory || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return 0;
    }
    
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    std::array<char, 64> dasm_buffer;
    int t1, t2;
    
    int len = z80ex_dasm(dasm_buffer.data(), dasm_buffer.size(), 0, &t1, &t2,
                         dasm_read_byte, pc, memory);
    
    snprintf(buffer, max_len, "[%04X] %s", pc, dasm_buffer.data());
    return len;
}

std::string Debugger::print_status(Z80EX_CONTEXT *cpu) const {
    if (!cpu) return "";
    
    const char *mode_str;
    switch (mode_) {
        case DebugMode::RUN: mode_str = "RUN"; break;
        case DebugMode::TRACE: mode_str = "TRACE"; break;
        case DebugMode::STEP: mode_str = "STEP"; break;
        case DebugMode::PAUSE: mode_str = "PAUSED"; break;
        default: mode_str = "UNKNOWN";
    }
    
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

std::string Debugger::memory_dump(uint8_t *memory, uint32_t start, uint16_t length) {
    if (!memory) return "";
    
    std::ostringstream oss;
    oss << "=== MEMORY DUMP " 
        << std::hex << std::setw(4) << std::setfill('0') << start 
        << "-" << std::setw(4) << (start + length - 1) << " ===\n";
    
    for (uint16_t i = 0; i < length; i += 16) {
        // Адрес
        oss << std::hex << std::setw(6) << std::setfill('0') << (start + i) << ": ";
        
        // Hex dump
        for (int j = 0; j < 16; j++) {
            if (i + j < length) {
                oss << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(memory[start + i + j]) << " ";
            } else {
                oss << "   ";
            }
        }
        
        // ASCII dump
        oss << " ";
        for (int j = 0; j < 16; j++) {
            if (i + j < length) {
                uint8_t c = memory[start + i + j];
                oss << ((c >= 32 && c < 127) ? static_cast<char>(c) : '.');
            }
        }
        
        if (i + 16 < length) oss << "\n";
    }
    ibuffer.println(oss.str());
    return oss.str();
}

std::string Debugger::disassemble_range(uint8_t *memory, uint32_t start, uint16_t end) {
    if (!memory || start >= end) return "";
    
    std::ostringstream oss;
    oss << "=== DISASSEMBLY " 
        << std::hex << std::setw(4) << std::setfill('0') << start 
        << "-" << std::setw(4) << end << " ===\n";
    
    uint16_t addr = start;
    while (addr < end) {
        std::array<char, 64> dasm_buffer;
        int t1, t2;
        
        int len = z80ex_dasm(dasm_buffer.data(), dasm_buffer.size(), 0, &t1, &t2,
                            dasm_read_byte, addr, memory);
        
        if (len == 0) break;
        
        // Адрес
        oss << std::hex << std::setw(4) << std::setfill('0') << addr << ": ";
        
        // Байты инструкции
        for (int i = 0; i < len; i++) {
            oss << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(memory[addr + i]) << " ";
        }
        
        // Выравнивание
        for (int i = len; i < 4; i++) {
            oss << "   ";
        }
        
        // Инструкция
        oss << " " << dasm_buffer.data() << "\n";
        
        addr += len;
    }
    ibuffer.println(oss.str());
    return oss.str();
}

// ============ ОБНОВЛЕНИЕ СОСТОЯНИЯ ============

bool Debugger::update(Z80EX_CONTEXT *cpu, uint8_t *memory) {
    if (!cpu || !memory) return false;
    
    // Если в режиме паузы - ничего не делаем
    if (mode_ == DebugMode::PAUSE) {
        return false;
    }
    
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    
    // Проверяем точки останова
    if (check_breakpoint(pc)) {
        mode_ = DebugMode::PAUSE;
        printf("\n*** BREAKPOINT HIT at %04X ***\n", pc);
        should_update_screen_ = true;
        return true;
    }
    
    // В режиме STEP: захватываем текущее состояние, выполняем одну инструкцию и переходим в паузу
    if (mode_ == DebugMode::STEP) {
        // Сохраняем состояние ДО выполнения инструкции
        capture_state(cpu, memory);
        
        // Выполняем одну инструкцию
        int cycles = z80ex_step(cpu);
        total_cycles_ += cycles;
        instruction_count_++;
        
        // Показываем инструкцию в консоли
        log_compact(cpu, memory);
        
        mode_ = DebugMode::PAUSE;
        should_update_screen_ = true;
        return true;
    }
    
    // В режиме TRACE: захватываем состояние, выполняем инструкцию, но остаемся в TRACE
    if (mode_ == DebugMode::TRACE) {
        // Сохраняем состояние ДО выполнения инструкции
        capture_state(cpu, memory);
        
        // Выполняем одну инструкцию
        int cycles = z80ex_step(cpu);
        total_cycles_ += cycles;
        instruction_count_++;
        
        // В режиме TRACE показываем инструкцию
        log_compact(cpu, memory);
        should_update_screen_ = true;
        
        return true;
    }
    
    // Режим RUN
    if (mode_ == DebugMode::RUN) {
        for (int i = 0; i < 2000; i++) {
            int cycles = z80ex_step(cpu);
            total_cycles_ += cycles;
            instruction_count_++;
        }
        return true;
    }

    return false;
}

// ============ РЕНДЕРИНГ В БУФЕР ============

std::string Debugger::render_to_buffer(Z80EX_CONTEXT *cpu, uint8_t *memory) {
    if (!cpu || !memory) return "";
    
    const size_t BUFFER_SIZE = 2048;
    std::array<char, BUFFER_SIZE> buffer;
    BufferPrinter bp;
    bp_init(&bp, buffer.data(), buffer.size());
    
    // Получаем текущие регистры
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    uint16_t sp = z80ex_get_reg(cpu, regSP);
    uint16_t af = z80ex_get_reg(cpu, regAF);
    uint16_t bc = z80ex_get_reg(cpu, regBC);
    uint16_t de = z80ex_get_reg(cpu, regDE);
    uint16_t hl = z80ex_get_reg(cpu, regHL);
    uint16_t ix = z80ex_get_reg(cpu, regIX);
    uint16_t iy = z80ex_get_reg(cpu, regIY);
    uint16_t af_ = z80ex_get_reg(cpu, regAF_);
    uint16_t bc_ = z80ex_get_reg(cpu, regBC_);
    uint16_t de_ = z80ex_get_reg(cpu, regDE_);
    uint16_t hl_ = z80ex_get_reg(cpu, regHL_);
    uint8_t i = z80ex_get_reg(cpu, regI);
    uint8_t r = z80ex_get_reg(cpu, regR);
    uint8_t im = z80ex_get_reg(cpu, regIM);
    uint8_t iff1 = z80ex_get_reg(cpu, regIFF1);
    uint8_t iff2 = z80ex_get_reg(cpu, regIFF2);
    
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
    get_bytes_at_address(memory, sp, sp_bytes.data(), 2);
    get_bytes_at_address(memory, bc, bc_bytes.data(), 2);
    get_bytes_at_address(memory, de, de_bytes.data(), 2);
    get_bytes_at_address(memory, hl, hl_bytes.data(), 2);
    get_bytes_at_address(memory, ix, ix_bytes.data(), 2);
    get_bytes_at_address(memory, iy, iy_bytes.data(), 2);
    
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
    
    bp_println_color(&bp, COLOR_CYAN, ">> XI Aleste DBG | MODE: %s | BP: %zu | INS: %llu\n",
                   mode_str, breakpoint_count_, instruction_count_);

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
    bp_println_color(&bp, COLOR_RED, "PC  %04X %s I=%02X", pc, di_ei, i);
    
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
    bp_println_color(&bp, COLOR_WHITE, "BC  %04X %02X%02X %c%c",
                   bc, bc_bytes[0], bc_bytes[1],
                   get_char_from_byte(bc_bytes[0]),
                   get_char_from_byte(bc_bytes[1]));

    bp_println_color(&bp, COLOR_WHITE, "DE  %04X %02X%02X %c%c",
                   de, de_bytes[0], de_bytes[1],
                   get_char_from_byte(de_bytes[0]),
                   get_char_from_byte(de_bytes[1]));

    bp_println_color(&bp, COLOR_WHITE, "HL  %04X %02X%02X %c%c",
                   hl, hl_bytes[0], hl_bytes[1],
                   get_char_from_byte(hl_bytes[0]),
                   get_char_from_byte(hl_bytes[1]));

    bp_println_color(&bp, COLOR_WHITE, "IX  %04X %02X%02X %c%c",
                   ix, ix_bytes[0], ix_bytes[1],
                   get_char_from_byte(ix_bytes[0]),
                   get_char_from_byte(ix_bytes[1]));

    // 5. Альтернативные регистры
    std::array<uint8_t, 2> af_bytes, bc__bytes, de__bytes, hl__bytes;
    get_bytes_at_address(memory, af_, af_bytes.data(), 2);
    get_bytes_at_address(memory, bc_, bc__bytes.data(), 2);
    get_bytes_at_address(memory, de_, de__bytes.data(), 2);
    get_bytes_at_address(memory, hl_, hl__bytes.data(), 2);
    
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
    
    bp_println_color(&bp, COLOR_GRAY_C0, "BC` %04X %02X%02X %c%c",
                   bc_, bc__bytes[0], bc__bytes[1],
                   get_char_from_byte(bc__bytes[0]),
                   get_char_from_byte(bc__bytes[1]));

    bp_println_color(&bp, COLOR_GRAY_C0, "DE` %04X %02X%02X %c%c",
                   de_, de__bytes[0], de__bytes[1],
                   get_char_from_byte(de__bytes[0]),
                   get_char_from_byte(de__bytes[1]));

    bp_println_color(&bp, COLOR_GRAY_C0, "HL` %04X %02X%02X %c%c",
                   hl_, hl__bytes[0], hl__bytes[1],
                   get_char_from_byte(hl__bytes[0]),
                   get_char_from_byte(hl__bytes[1]));

    bp_println(&bp, ""); // Пустая строка
    
    // 6. Дизассемблер
    bp_println_color(&bp, COLOR_CYAN, "DISASSEMBLER:");
    
    // Показываем предыдущие инструкции из истории
    if (history_count_ > 0) {
        int history_to_show = (history_count_ < 3) ? static_cast<int>(history_count_) : 3;
        
        for (int i = 0; i < history_to_show; i++) {
            int idx = (static_cast<int>(history_index_) - history_to_show + i) % 10;
            if (idx < 0) idx += 10;
            
            CPUState &hist_entry = history_[idx];
            
            if (hist_entry.pc == 0 && hist_entry.instruction[0] == '\0') {
                continue;
            }
            
            std::string bytes_str;
            uint16_t hist_pc = hist_entry.pc;
            
            for (int j = 0; j < hist_entry.len; j++) {
                uint8_t byte = dasm_read_byte(hist_pc + j, 0);
                char byte_hex[4];
                snprintf(byte_hex, sizeof(byte_hex), "%02X", byte);
                bytes_str += byte_hex;
            }
            
            auto full_hist_pc = dasm_translate_address(hist_entry.pc, ECycle::MRD);
            bp_print(&bp, "  %06X %-8s %s", full_hist_pc, bytes_str.c_str(), hist_entry.instruction.data());
            bp_println(&bp, "");
        }
    }
    
    // Текущая команда
    std::array<char, 64> current_dasm;
    int t1, t2;
    int current_len = z80ex_dasm(current_dasm.data(), current_dasm.size(), 0, &t1, &t2,
                                dasm_read_byte, pc, 0);
    
    if (current_len > 0) {
        std::string current_bytes_str;
        for (int j = 0; j < current_len && j < 4; j++) {
            char byte_hex[4];
            auto data = dasm_read_byte(pc + j, 0);
            snprintf(byte_hex, sizeof(byte_hex), "%02X", data);
            current_bytes_str += byte_hex;
        }
        
        while (current_bytes_str.length() < 8) {
            current_bytes_str += " ";
        }
        
        auto full_pc = dasm_translate_address(pc, ECycle::MRD);
        bp_println_color(&bp, COLOR_YELLOW, " >%06X %s %s", full_pc, current_bytes_str.c_str(), current_dasm.data());
        
        // Показываем следующие команды
        uint16_t next_pc = pc + current_len;
        int next_to_show = 3;
        
        for (int i = 0; i < next_to_show; i++) {
            std::array<char, 64> next_dasm;
            int t1_next, t2_next;
            int next_len = z80ex_dasm(next_dasm.data(), next_dasm.size(), 0, &t1_next, &t2_next,
                                     dasm_read_byte, next_pc, 0);
            
            if (next_len == 0) break;
            
            std::string next_bytes_str;
            for (int j = 0; j < next_len && j < 4; j++) {
                char byte_hex[4];
                auto data = dasm_read_byte(next_pc + j, 0);
                snprintf(byte_hex, sizeof(byte_hex), "%02X", data);
                next_bytes_str += byte_hex;
            }
            
            while (next_bytes_str.length() < 8) {
                next_bytes_str += " ";
            }
            
            auto full_next_pc = dasm_translate_address(next_pc, ECycle::MRD);
            bp_print(&bp, "  %06X %s %s", full_next_pc, next_bytes_str.c_str(), next_dasm.data());
            bp_println(&bp, "");
            
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