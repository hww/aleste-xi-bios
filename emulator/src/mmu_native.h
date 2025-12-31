// =============================================================================
// KISS Software MMU Emulator for Aleste LX System
// =============================================================================
#ifndef _MMU_NATIVE_H_
#define _MMU_NATIVE_H_

#include <cstdint>
#include <array>

#include "globals.h"

class NativeMMU {
public:
    // Конструктор
    NativeMMU() {
        reset();
    }
    
    // Сброс
    void reset() {
        reg_clock = 0x02;          // Divide clock by 4
        reg_control = 0x13;        // [4]=1(unlock), [1]=1(supervisor), [0]=1(native)
        reg_mmio_page = 0x00;
        reg_super_slot = 0xFF;     // Supervisor: все страницы = слот 3
        reg_user_slot = 0x00;      // User: все страницы = слот 0
        reg_syscall_function = 0x00;
        
        reg_bank.fill(0x00);
        
        native_mode = true;
        supervisor_mode = true;
        supervisor_exit_pending = false;
        
        cpu_write_pending = false;
        cpu_write_data = 0x00;
        cpu_write_addr = 0x00;
    }
    
    // Проверяет, реагирует ли MMU на этот цикл
    bool check_port(uint16_t addr, ECycle cycle) const;
    
    // Выполняет цикл чтения/записи
    uint8_t process_cycle(uint16_t addr, ECycle cycle, uint8_t data_in = 0x00);
    
    // Транслирует 16-битный адрес в 24-битный физический
    uint32_t translate_address(uint16_t addr, ECycle cycle) const;
    
    // Вспомогательные методы
    bool is_supervisor_mode() const { return supervisor_mode; }
    bool is_native_mode() const { return native_mode; }
    bool is_user_locked() const { return user_locked(); }
    uint8_t get_clock_config() const { return reg_clock & 0x0F; }
    
    // Получить текущий слот
    uint8_t get_current_slot(uint16_t addr, ECycle cycle) const;

    // Получить текущий банк для отладки
    uint8_t get_current_bank(uint16_t addr, ECycle cycle) const;

    // Получить текущий слот регистр           
    uint8_t get_current_slot_reg() const { return supervisor_mode ? reg_super_slot : reg_user_slot; }

    uint8_t get_user_slot_reg() const { return reg_user_slot; }
    uint8_t get_super_slot_reg() const { return reg_super_slot; }
private:
    // Регистры
    uint8_t reg_clock;
    uint8_t reg_control;
    uint8_t reg_mmio_page;
    uint8_t reg_super_slot;
    uint8_t reg_user_slot;
    uint8_t reg_syscall_function;
    std::array<uint8_t, 16> reg_bank;
    
    // Состояние
    bool supervisor_mode;
    bool supervisor_exit_pending;
    bool native_mode;
    
    // Временные переменные для обработки записи
    bool cpu_write_pending;
    uint8_t cpu_write_data;
    uint8_t cpu_write_addr;
    uint8_t current_slot_reg;
    
    // Вспомогательные методы
    bool user_locked() const {
        return (reg_control & 0x10) == 0;
    }
    
    bool reg_trap_enabled() const {
        return (reg_control & 0x04) != 0;
    }
    

    
    // Получить индекс маппера
    uint8_t get_mapper_index(uint8_t current_slot, uint16_t addr, ECycle cycle) const;
    
    // Обработка записи в IO порт
    void handle_io_write(uint16_t addr, uint8_t data);
    
    // Обработка чтения из IO порта
    uint8_t handle_io_read(uint16_t addr) const;
    
    // Обработка доступа к памяти
    void handle_memory_access(uint16_t addr, ECycle cycle);
    
    // Обработка записи регистров в Native mode
    void handle_native_register_write(uint8_t reg_addr, uint8_t data);
    
    // Обработка чтения регистров в Native mode
    uint8_t handle_native_register_read(uint8_t reg_addr) const;

    // Обработка записи регистров в Legacy mode
    void handle_legacy_register_write(uint8_t reg_addr, uint8_t data);
    
    // Обработка чтения регистров в Legacy mode
    uint8_t handle_legacy_register_read(uint8_t reg_addr) const;
};


#endif