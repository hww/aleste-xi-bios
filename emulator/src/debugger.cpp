#include "debugger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <z80ex.h>
#include <z80ex_dasm.h>
#include "render/text_colors.h"
#include "render/buffer_printer.h"
#include "ports.h"

// Функция для чтения байта (для дизассемблера)
static uint8_t dasm_read_byte(uint16_t addr, void *user_data)
{
    uint8_t *mem = (uint8_t*)user_data;
    if (addr < 0x10000) {
        return mem[addr];
    }
    return 0xFF;
}

// Дизассемблер
static void disassemble_instruction(uint16_t pc, uint8_t *memory, char *buffer, size_t buf_size)
{
    if (!buffer || buf_size == 0) return;
    
    int t1, t2;
    z80ex_dasm(buffer, buf_size, 0, &t1, &t2, dasm_read_byte, pc, memory);
}

// ============ ИНИЦИАЛИЗАЦИЯ ============

void debugger_init(Debugger *dbg)
{
    if (!dbg) return;
    
    dbg->mode = DEBUG_MODE_PAUSE;
    dbg->breakpoint_count = 0;
    dbg->history_index = 0;
    dbg->history_count = 0;
    dbg->should_update_screen = false;
    dbg->trace_delay_ms = 100;
    dbg->total_cycles = 0;
    dbg->instruction_count = 0;
    
    // Очищаем точки останова
    for (int i = 0; i < 16; i++) {
        dbg->breakpoints[i].address = 0;
        dbg->breakpoints[i].enabled = false;
        dbg->breakpoints[i].description[0] = '\0';
    }
    
    // Очищаем историю
    for (int i = 0; i < 10; i++) {
        memset(&dbg->history[i], 0, sizeof(CPUState));
        strcpy(dbg->history[i].instruction, "NOP");
    }
}

// ============ УПРАВЛЕНИЕ ИСПОЛНЕНИЕМ ============

void debugger_run(Debugger *dbg)
{
    if (!dbg) return;
    
    dbg->mode = DEBUG_MODE_RUN;
    dbg->should_update_screen = true;
}

void debugger_trace(Debugger *dbg)
{
    if (!dbg) return;
    
    dbg->mode = DEBUG_MODE_TRACE;
    dbg->should_update_screen = true;
}

void debugger_pause(Debugger *dbg)
{
    if (!dbg) return;
    
    dbg->mode = DEBUG_MODE_PAUSE;
}

void debugger_step(Debugger *dbg)
{
    if (!dbg) return;
    
    dbg->mode = DEBUG_MODE_STEP;
}

// ============ ЗАХВАТ СОСТОЯНИЯ ============

void debugger_capture_state(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory)
{
    if (!dbg || !cpu || !memory) return;
    
    CPUState *state = &dbg->history[dbg->history_index];
    
    // Регистры
    state->pc = z80ex_get_reg(cpu, regPC);
    state->sp = z80ex_get_reg(cpu, regSP);
    state->af = z80ex_get_reg(cpu, regAF);
    state->bc = z80ex_get_reg(cpu, regBC);
    state->de = z80ex_get_reg(cpu, regDE);
    state->hl = z80ex_get_reg(cpu, regHL);
    state->af_ = z80ex_get_reg(cpu, regAF_);
    state->bc_ = z80ex_get_reg(cpu, regBC_);
    state->de_ = z80ex_get_reg(cpu, regDE_);
    state->hl_ = z80ex_get_reg(cpu, regHL_);
    state->ix = z80ex_get_reg(cpu, regIX);
    state->iy = z80ex_get_reg(cpu, regIY);
    state->i = z80ex_get_reg(cpu, regI);
    state->r = z80ex_get_reg(cpu, regR);
    state->im = z80ex_get_reg(cpu, regIM);
    state->iff1 = z80ex_get_reg(cpu, regIFF1);
    state->iff2 = z80ex_get_reg(cpu, regIFF2);
    
    // Флаги
    uint8_t f = state->af & 0xFF;
    state->flags.s = (f >> 7) & 1;
    state->flags.z = (f >> 6) & 1;
    state->flags.y = (f >> 5) & 1;
    state->flags.h = (f >> 4) & 1;
    state->flags.x = (f >> 3) & 1;
    state->flags.p = (f >> 2) & 1;
    state->flags.n = (f >> 1) & 1;
    state->flags.c = f & 1;
    
    // Дизассемблируем текущую инструкцию
    disassemble_instruction(state->pc, memory, state->instruction, sizeof(state->instruction));
    
    // Сдвигаем кольцевой буфер
    dbg->history_index = (dbg->history_index + 1) % 10;
    if (dbg->history_count < 10) {
        dbg->history_count++;
    }
}

// ============ КОМПАКТНЫЙ ЛОГ ============

void debugger_log_compact(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory)
{
    if (!dbg || !cpu) return;
    
    // Получаем регистры напрямую
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    uint16_t sp = z80ex_get_reg(cpu, regSP);
    uint16_t af = z80ex_get_reg(cpu, regAF);
    uint16_t bc = z80ex_get_reg(cpu, regBC);
    uint16_t de = z80ex_get_reg(cpu, regDE);
    uint16_t hl = z80ex_get_reg(cpu, regHL);
    
    // Флаги из AF
    uint8_t f = af & 0xFF;
    char flags[9] = {
        (f & 0x80) ? 'S' : '.',
        (f & 0x40) ? 'Z' : '.', 
        (f & 0x20) ? 'Y' : '.',
        (f & 0x10) ? 'H' : '.',
        (f & 0x08) ? 'X' : '.',
        (f & 0x04) ? 'P' : '.',
        (f & 0x02) ? 'N' : '.',
        (f & 0x01) ? 'C' : '.',
        '\0'
    };
    
    // Дизассемблируем текущую инструкцию
    char instruction[32];
    disassemble_instruction(pc, memory, instruction, sizeof(instruction));
    
    // Компактный вывод в одну строку
    printf("[%04X] %-20s | PC:%04X SP:%04X AF:%04X BC:%04X DE:%04X HL:%04X | %s\n",
           pc, instruction,
           pc, sp, af, bc, de, hl, flags);
}

// ============ ТОЧКИ ОСТАНОВА ============

bool debugger_add_breakpoint(Debugger *dbg, uint16_t address, const char *desc)
{
    if (!dbg || dbg->breakpoint_count >= 16) return false;
    
    // Проверяем, не существует ли уже такой точки
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].address == address) {
            return false;
        }
    }
    
    // Добавляем новую точку
    dbg->breakpoints[dbg->breakpoint_count].address = address;
    dbg->breakpoints[dbg->breakpoint_count].enabled = true;
    if (desc) {
        strncpy(dbg->breakpoints[dbg->breakpoint_count].description, 
                desc, sizeof(dbg->breakpoints[0].description) - 1);
    } else {
        snprintf(dbg->breakpoints[dbg->breakpoint_count].description,
                sizeof(dbg->breakpoints[0].description),
                "Breakpoint %04X", address);
    }
    
    dbg->breakpoint_count++;
    return true;
}

bool debugger_remove_breakpoint(Debugger *dbg, uint16_t address)
{
    if (!dbg) return false;
    
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].address == address) {
            // Сдвигаем остальные точки
            for (int j = i; j < dbg->breakpoint_count - 1; j++) {
                dbg->breakpoints[j] = dbg->breakpoints[j + 1];
            }
            dbg->breakpoint_count--;
            return true;
        }
    }
    
    return false;
}

void debugger_list_breakpoints(const Debugger *dbg, char *buffer, int max_len)
{
    if (!dbg || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
    char *ptr = buffer;
    int remaining = max_len;
    
    int written = snprintf(ptr, remaining, "=== BREAKPOINTS ===\n");
    if (written > 0) {
        ptr += written;
        remaining -= written;
    }
    
    if (dbg->breakpoint_count == 0) {
        snprintf(ptr, remaining, "No breakpoints set\n");
        return;
    }
    
    for (int i = 0; i < dbg->breakpoint_count && remaining > 0; i++) {
        written = snprintf(ptr, remaining, "%2d. %04X [%s] - %s\n",
               i + 1,
               dbg->breakpoints[i].address,
               dbg->breakpoints[i].enabled ? "ENABLED" : "DISABLED",
               dbg->breakpoints[i].description);
        
        if (written > 0) {
            ptr += written;
            remaining -= written;
        }
    }
}

bool debugger_check_breakpoint(const Debugger *dbg, uint16_t address)
{
    if (!dbg) return false;
    
    for (int i = 0; i < dbg->breakpoint_count; i++) {
        if (dbg->breakpoints[i].enabled && 
            dbg->breakpoints[i].address == address) {
            return true;
        }
    }
    
    return false;
}

// ============ УТИЛИТЫ ============

void debugger_print_registers(Z80EX_CONTEXT *cpu, char *buffer, int max_len)
{
    if (!cpu || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
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
    char flags[9] = {
        (f & 0x80) ? 'S' : '.',
        (f & 0x40) ? 'Z' : '.',
        (f & 0x20) ? 'Y' : '.',
        (f & 0x10) ? 'H' : '.',
        (f & 0x08) ? 'X' : '.',
        (f & 0x04) ? 'P' : '.',
        (f & 0x02) ? 'N' : '.',
        (f & 0x01) ? 'C' : '.',
        '\0'
    };
    
    snprintf(buffer, max_len,
        "PC:%04X SP:%04X AF:%04X BC:%04X DE:%04X HL:%04X\n"
        "IX:%04X IY:%04X AF':%04X BC':%04X DE':%04X HL':%04X\n"
        "FLAGS: %s",
        pc, sp, af, bc, de, hl,
        ix, iy, af_, bc_, de_, hl_,
        flags);
}

void debugger_print_current_instruction(Z80EX_CONTEXT *cpu, uint8_t *memory, char *buffer, int max_len)
{
    if (!cpu || !memory || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    char dasm_buffer[64];
    int t1, t2;
    
    z80ex_dasm(dasm_buffer, sizeof(dasm_buffer), 0, &t1, &t2,
               dasm_read_byte, pc, memory);
    
    snprintf(buffer, max_len, "[%04X] %s", pc, dasm_buffer);
}

void debugger_print_status(const Debugger *dbg, Z80EX_CONTEXT *cpu, char *buffer, int max_len)
{
    if (!dbg || !cpu || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
    const char *mode_str;
    switch (dbg->mode) {
        case DEBUG_MODE_RUN: mode_str = "RUN"; break;
        case DEBUG_MODE_TRACE: mode_str = "TRACE"; break;
        case DEBUG_MODE_STEP: mode_str = "STEP"; break;
        case DEBUG_MODE_PAUSE: mode_str = "PAUSED"; break;
        default: mode_str = "UNKNOWN";
    }
    
    snprintf(buffer, max_len,
        "=== DEBUGGER STATUS ===\n"
        "Mode: %s\n"
        "Instructions: %llu  Cycles: %llu\n"
        "Breakpoints: %d",
        mode_str,
        (unsigned long long)dbg->instruction_count,
        (unsigned long long)dbg->total_cycles,
        dbg->breakpoint_count);
}

// ============ ПРОСМОТР ПАМЯТИ ============

void debugger_memory_dump(uint8_t *memory, uint16_t start, uint16_t length, char *buffer, int max_len)
{
    if (!memory || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
    char *ptr = buffer;
    int remaining = max_len;
    int written;
    
    written = snprintf(ptr, remaining, "=== MEMORY DUMP %04X-%04X ===\n", 
                      start, start + length - 1);
    if (written > 0) {
        ptr += written;
        remaining -= written;
    }
    
    for (uint16_t i = 0; i < length && remaining > 0; i += 16) {
        // Адрес
        written = snprintf(ptr, remaining, "%04X: ", start + i);
        if (written > 0) {
            ptr += written;
            remaining -= written;
        }
        
        // Hex dump
        for (int j = 0; j < 16 && remaining > 0; j++) {
            if (i + j < length) {
                written = snprintf(ptr, remaining, "%02X ", memory[start + i + j]);
            } else {
                written = snprintf(ptr, remaining, "   ");
            }
            
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
        
        // ASCII dump
        if (remaining > 0) {
            written = snprintf(ptr, remaining, " ");
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
            
            for (int j = 0; j < 16 && remaining > 0; j++) {
                if (i + j < length) {
                    uint8_t c = memory[start + i + j];
                    written = snprintf(ptr, remaining, "%c", 
                                     (c >= 32 && c < 127) ? c : '.');
                }
                
                if (written > 0) {
                    ptr += written;
                    remaining -= written;
                }
            }
        }
        
        if (remaining > 0) {
            written = snprintf(ptr, remaining, "\n");
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
    }
}

void debugger_disassemble_range(uint8_t *memory, uint16_t start, uint16_t end, char *buffer, int max_len)
{
    if (!memory || !buffer || max_len <= 0 || start >= end) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
    char *ptr = buffer;
    int remaining = max_len;
    int written;
    
    written = snprintf(ptr, remaining, "=== DISASSEMBLY %04X-%04X ===\n", start, end);
    if (written > 0) {
        ptr += written;
        remaining -= written;
    }
    
    uint16_t addr = start;
    while (addr < end && remaining > 0) {
        char dasm_buffer[64];
        int t1, t2;
        
        int len = z80ex_dasm(dasm_buffer, sizeof(dasm_buffer), 0, &t1, &t2,
                            dasm_read_byte, addr, memory);
        
        if (len == 0) break;
        
        // Адрес
        written = snprintf(ptr, remaining, "%04X: ", addr);
        if (written > 0) {
            ptr += written;
            remaining -= written;
        }
        
        // Байты инструкции
        for (int i = 0; i < len && remaining > 0; i++) {
            written = snprintf(ptr, remaining, "%02X ", memory[addr + i]);
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
        
        // Выравнивание
        for (int i = len; i < 4 && remaining > 0; i++) {
            written = snprintf(ptr, remaining, "   ");
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
        
        // Инструкция
        if (remaining > 0) {
            written = snprintf(ptr, remaining, " %s\n", dasm_buffer);
            if (written > 0) {
                ptr += written;
                remaining -= written;
            }
        }
        
        addr += len;
    }
}

// ============ ОБНОВЛЕНИЕ СОСТОЯНИЯ ============

bool debugger_update(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory)
{
    if (!dbg || !cpu || !memory) return false;
    
    // Если в режиме паузы - ничего не делаем
    if (dbg->mode == DEBUG_MODE_PAUSE) {
        return false;
    }
    
    uint16_t pc = z80ex_get_reg(cpu, regPC);
    
    // Проверяем точки останова
    if (debugger_check_breakpoint(dbg, pc)) {
        dbg->mode = DEBUG_MODE_PAUSE;
        printf("\n*** BREAKPOINT HIT at %04X ***\n", pc);
        debugger_capture_state(dbg, cpu, memory);
        dbg->should_update_screen = true;
        return true;
    }
    
    // Режим STEP: выполняем одну инструкцию и переходим в паузу
    if (dbg->mode == DEBUG_MODE_STEP) {
        int cycles = z80ex_step(cpu);
        dbg->total_cycles += cycles;
        dbg->instruction_count++;
        
        // Захватываем состояние после выполнения
        debugger_capture_state(dbg, cpu, memory);
        
        // Показываем инструкцию в консоли
        debugger_log_compact(dbg, cpu, memory);
        
        dbg->mode = DEBUG_MODE_PAUSE;
        dbg->should_update_screen = true;
        return true;
    }
    
    // Режимы RUN и TRACE
    if (dbg->mode == DEBUG_MODE_TRACE) {
        int cycles = z80ex_step(cpu);
        dbg->total_cycles += cycles;
        dbg->instruction_count++;
        
        // Захватываем состояние после выполнения
        debugger_capture_state(dbg, cpu, memory);
        
        // В режиме TRACE показываем инструкцию
        if (dbg->mode == DEBUG_MODE_TRACE) {
            debugger_log_compact(dbg, cpu, memory);
            dbg->should_update_screen = true;
        }
        
        return true;
    }
    
    if (dbg->mode == DEBUG_MODE_RUN) {
        for (int i=0; i<2000; i++) {
            int cycles = z80ex_step(cpu);
            dbg->total_cycles += cycles;
            dbg->instruction_count++;
        }
        // Захватываем состояние после выполнения
        debugger_capture_state(dbg, cpu, memory);
        
        return true;
    }

    return false;
}

// ============ РЕНДЕРИНГ В БУФЕР ============

// Функция для получения символа из байта
static char get_char_from_byte(uint8_t b) {
    // Возвращаем печатный символ или точку
    return (b >= 32 && b < 127) ? (char)b : '.';
}

// Функция для получения байтов из памяти по адресу
static void get_bytes_at_address(uint8_t *memory, uint16_t addr, uint8_t *bytes, int count) {
    for (int i = 0; i < count; i++) {
        if (addr + i < 0x10000) {
            bytes[i] = memory[addr + i];
        } else {
            bytes[i] = 0xFF;
        }
    }
}


void debugger_render_to_buffer(Debugger *dbg, Z80EX_CONTEXT *cpu, uint8_t *memory, char *buffer, int max_len)
{
    if (!dbg || !cpu || !memory || !buffer || max_len <= 0) {
        if (buffer && max_len > 0) buffer[0] = '\0';
        return;
    }
    
    // Инициализация BufferPrinter
    BufferPrinter bp;
    bp_init(&bp, buffer, max_len);
    
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
    char flags[9] = {
        (f & 0x80) ? 'S' : 's',
        (f & 0x40) ? 'Z' : 'z', 
        (f & 0x20) ? '5' : 'x', // Y
        (f & 0x10) ? 'H' : 'h',
        (f & 0x08) ? '3' : 'x', // X
        (f & 0x04) ? 'P' : 'p',
        (f & 0x02) ? 'N' : 'n',
        (f & 0x01) ? 'C' : 'c',
        '\0'
    };
    
    // Получаем байты для регистров, указывающих на память
    uint8_t sp_bytes[2], bc_bytes[2], de_bytes[2], hl_bytes[2], ix_bytes[2], iy_bytes[2];
    get_bytes_at_address(memory, sp, sp_bytes, 2);
    get_bytes_at_address(memory, bc, bc_bytes, 2);
    get_bytes_at_address(memory, de, de_bytes, 2);
    get_bytes_at_address(memory, hl, hl_bytes, 2);
    get_bytes_at_address(memory, ix, ix_bytes, 2);
    get_bytes_at_address(memory, iy, iy_bytes, 2);
    
      
    // 7. Статус дебаггера
    bp_println(&bp, "");
    
    const char *mode_str;
    switch (dbg->mode) {
        case DEBUG_MODE_RUN: mode_str = "RUN"; break;
        case DEBUG_MODE_TRACE: mode_str = "TRACE"; break;
        case DEBUG_MODE_STEP: mode_str = "STEP"; break;
        case DEBUG_MODE_PAUSE: mode_str = "PAUSE"; break;
        default: mode_str = "UNKNOWN";
    }
    
    bp_println_color(&bp, COLOR_GREEN, ">> XI Aleste DBG | MODE: %s | BP: %d | INS: %llu",
                   mode_str, dbg->breakpoint_count,
                   (unsigned long long)dbg->instruction_count);

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
    bp_println_color(&bp, COLOR_CYAN, "PC  %04X %s I=%02X", pc, di_ei, i);
    
    // 2. Регистр SP с байтами
    bp_println_color(&bp, COLOR_CYAN, "SP  %04X %02X%02X %c%c", 
                   sp, sp_bytes[0], sp_bytes[1],
                   get_char_from_byte(sp_bytes[0]),
                   get_char_from_byte(sp_bytes[1]));
    

                   
    // 3. Регистр A с флагами
    bp_println_color(&bp, COLOR_CYAN, " A  %02X %c %s",
                   (af >> 8) & 0xFF,
                   get_char_from_byte((af >> 8) & 0xFF),
                   flags);

    
    // 4. Регистры BC, DE, HL, IX
    bp_println_color(&bp, COLOR_CYAN, "BC  %04X %02X%02X %c%c",
                   bc, bc_bytes[0], bc_bytes[1],
                   get_char_from_byte(bc_bytes[0]),
                   get_char_from_byte(bc_bytes[1]));

    
    bp_println_color(&bp, COLOR_CYAN, "DE  %04X %02X%02X %c%c",
                   de, de_bytes[0], de_bytes[1],
                   get_char_from_byte(de_bytes[0]),
                   get_char_from_byte(de_bytes[1]));

    
    bp_println_color(&bp, COLOR_CYAN, "HL  %04X %02X%02X %c%c",
                   hl, hl_bytes[0], hl_bytes[1],
                   get_char_from_byte(hl_bytes[0]),
                   get_char_from_byte(hl_bytes[1]));

    
    bp_println_color(&bp, COLOR_CYAN, "IX  %04X %02X%02X %c%c",
                   ix, ix_bytes[0], ix_bytes[1],
                   get_char_from_byte(ix_bytes[0]),
                   get_char_from_byte(ix_bytes[1]));

    
    // 5. Альтернативные регистры
    uint8_t af_bytes[2], bc__bytes[2], de__bytes[2], hl__bytes[2];
    get_bytes_at_address(memory, af_, af_bytes, 2);
    get_bytes_at_address(memory, bc_, bc__bytes, 2);
    get_bytes_at_address(memory, de_, de__bytes, 2);
    get_bytes_at_address(memory, hl_, hl__bytes, 2);
    
    uint8_t f_ = af_ & 0xFF;
    char flags_[9] = {
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
    
    bp_println_color(&bp, COLOR_MAGENTA, " A` %02X %c %s",
                   (af_ >> 8) & 0xFF,
                   get_char_from_byte((af_ >> 8) & 0xFF),
                   flags_);

    
    bp_println_color(&bp, COLOR_MAGENTA, "BC` %04X %02X%02X %c%c",
                   bc_, bc__bytes[0], bc__bytes[1],
                   get_char_from_byte(bc__bytes[0]),
                   get_char_from_byte(bc__bytes[1]));

    
    bp_println_color(&bp, COLOR_MAGENTA, "DE` %04X %02X%02X %c%c",
                   de_, de__bytes[0], de__bytes[1],
                   get_char_from_byte(de__bytes[0]),
                   get_char_from_byte(de__bytes[1]));

    
    bp_println_color(&bp, COLOR_MAGENTA, "HL` %04X %02X%02X %c%c",
                   hl_, hl__bytes[0], hl__bytes[1],
                   get_char_from_byte(hl__bytes[0]),
                   get_char_from_byte(hl__bytes[1]));

    bp_println(&bp, ""); // Пустая строка
    
    // 6. Дизассемблер
    bp_println_color(&bp, COLOR_YELLOW, "DISASSEMBLER:");

    
    // Сначала показываем историю (предыдущие команды)
    if (dbg->history_count > 0) {
        // Показываем последние 2 команды из истории (перед текущей)
        int history_to_show = (dbg->history_count < 2) ? dbg->history_count : 2;
        
        for (int i = 0; i < history_to_show; i++) {
            // Берем из истории (от более старой к более новой)
            int idx = (dbg->history_index - history_to_show + i + 10) % 10;
            
            // Проверяем, что есть данные
            if (idx < 0 || idx >= 10) continue;
            
            CPUState *hist_entry = &dbg->history[idx];
            
            // Дизассемблируем заново для получения байтов
            char dasm_buffer[64];
            int t1, t2;
            int len = z80ex_dasm(dasm_buffer, sizeof(dasm_buffer), 0, &t1, &t2,
                                dasm_read_byte, hist_entry->pc, memory);
            
            if (len == 0) continue;
            
            // Формируем строку с байтами
            char bytes_str[16] = "";
            for (int j = 0; j < len && j < 4; j++) {
                char byte_hex[4];
                snprintf(byte_hex, sizeof(byte_hex), "%02X", memory[hist_entry->pc + j]);
                strcat(bytes_str, byte_hex);
            }
            
            bp_print(&bp, "  %04X %-8s %s", hist_entry->pc, bytes_str, dasm_buffer);
            bp_println(&bp, "");
        }
    }
    
    // Текущая команда
    char current_dasm[64];
    int t1, t2;
    int current_len = z80ex_dasm(current_dasm, sizeof(current_dasm), 0, &t1, &t2,
                                dasm_read_byte, pc, memory);
    
    if (current_len > 0) {
        // Формируем строку с байтами текущей команды
        char current_bytes_str[16] = "";
        for (int j = 0; j < current_len && j < 4; j++) {
            char byte_hex[4];
            snprintf(byte_hex, sizeof(byte_hex), "%02X", memory[pc + j]);
            strcat(current_bytes_str, byte_hex);
        }
        
        // Выделяем текущую команду красным цветом
        bp_println_color(&bp, COLOR_RED, " >%04X %-8s %s", pc, current_bytes_str, current_dasm);
        bp_println(&bp, "");
        
        // Следующие команды (после текущей)
        uint16_t next_pc = pc + current_len;
        int next_to_show = 3; // Показываем 3 следующие команды
        
        for (int i = 0; i < next_to_show; i++) {
            char next_dasm[64];
            int t1_next, t2_next;
            int next_len = z80ex_dasm(next_dasm, sizeof(next_dasm), 0, &t1_next, &t2_next,
                                     dasm_read_byte, next_pc, memory);
            
            if (next_len == 0) break;
            
            // Формируем строку с байтами следующей команды
            char next_bytes_str[16] = "";
            for (int j = 0; j < next_len && j < 4; j++) {
                char byte_hex[4];
                snprintf(byte_hex, sizeof(byte_hex), "%02X", memory[next_pc + j]);
                strcat(next_bytes_str, byte_hex);
            }
            
            bp_print(&bp, "  %04X %-8s %s", next_pc, next_bytes_str, next_dasm);
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
}