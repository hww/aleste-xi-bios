// =============================================================================
// KISS Software MMU Emulator for Aleste LX System
// =============================================================================

#include <cstdint>
#include <array>

#include "bus.h"

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
        
        supervisor_mode = true;
        supervisor_exit_pending = false;
        native_mode = true;
        
        cpu_write_pending = false;
        cpu_write_data = 0x00;
        cpu_write_addr = 0x00;
    }
    
    // Проверяет, реагирует ли MMU на этот цикл
    bool check_port(uint16_t addr, ECycle cycle) const {
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            // IO доступ
            
            // Syscall всегда доступен
            uint8_t syscall_addr = native_mode ? 0xF2 : 0xFFF2;
            if (addr == syscall_addr) {
                return true;
            }
            
            if (native_mode) {
                // Native mode: порты C0-FF
                if ((addr & 0xFF) >= 0xC0 && (addr & 0xFF) <= 0xFF) {
                    // Проверяем доступ
                    if (supervisor_mode || !user_locked()) {
                        return true;
                    }
                }
                
                // MMIO: порты 00-BF
                if ((addr & 0xFF) < 0xC0) {
                    if (supervisor_mode || !user_locked()) {
                        return true;
                    }
                }
            } else {
                // Legacy mode: порты F0-FF
                if ((addr & 0xFF00) == 0xFF00) {
                    if (supervisor_mode || !user_locked()) {
                        return true;
                    }
                }
            }
        }
        
        // Всегда реагируем на доступ к памяти
        if (cycle == ECycle::MWR || cycle == ECycle::MRD || cycle == ECycle::OPCODE) {
            return true;
        }
        
        return false;
    }
    
    // Выполняет цикл чтения/записи
    uint8_t process_cycle(uint16_t addr, ECycle cycle, uint8_t data_in = 0x00) {
        uint8_t result = 0xFF;
        
        if (cycle == ECycle::IOWR) {
            // Запись в IO порт
            handle_io_write(addr, data_in);
            result = 0x00; // Для записи результат не важен
        }
        else if (cycle == ECycle::IORD) {
            // Чтение из IO порта
            result = handle_io_read(addr);
        }
        else if (cycle == ECycle::OPCODE) {
            // Обработка opcode fetch (для TRAP)
            if (reg_trap_enabled() && (addr == 0x0038 || addr == 0x0066)) {
                if (!supervisor_mode) {
                    supervisor_mode = true;
                    supervisor_exit_pending = false;
                }
            }
            result = 0x00; // MMU не возвращает данные при fetch
        }
        else if (cycle == ECycle::MWR || cycle == ECycle::MRD) {
            // Для памяти MMU не обрабатывает данные напрямую
            // но обновляет состояние если нужно
            handle_memory_access(addr, cycle);
            result = 0x00;
        }
        
        return result;
    }
    
    // Транслирует 16-битный адрес в 24-битный физический
    uint32_t translate_address(uint16_t addr, ECycle cycle) const {
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            // IO доступ
            if (native_mode) {
                if ((addr & 0xFF) < 0xC0) {
                    // MMIO доступ
                    return (0xFF000000 | (reg_mmio_page << 8) | (addr & 0xFF));
                } else {
                    // Обычный IO
                    return (0xFF000000 | addr);
                }
            } else {
                // Legacy mode
                return 0xFF000000 | addr;
            }
        } else {
            // Доступ к памяти
            if (native_mode) {
                // Получаем текущий слот
                uint8_t current_slot = get_current_slot(addr, cycle);
                
                // Получаем индекс маппера
                uint8_t mapper_index = get_mapper_index(current_slot, addr, cycle);
                
                // Получаем банк
                uint8_t bank = reg_bank[mapper_index];
                
                // Формируем адрес: {slot, bank, offset}
                return ((current_slot & 0x03) << 22) | 
                       (bank << 14) | 
                       (addr & 0x3FFF);
            } else {
                // Legacy mode: прямое отображение
                return addr;
            }
        }
    }
    
    // Вспомогательные методы
    bool is_supervisor_mode() const { return supervisor_mode; }
    bool is_native_mode() const { return native_mode; }
    bool is_user_locked() const { return user_locked(); }
    uint8_t get_clock_config() const { return reg_clock & 0x0F; }
    
    // Получить текущий слот для отладки
    uint8_t get_current_slot_debug(uint16_t addr, ECycle cycle) const {
        return get_current_slot(addr, cycle);
    }
    
    // Получить текущий банк для отладки
    uint8_t get_current_bank_debug(uint16_t addr, ECycle cycle) const {
        uint8_t current_slot = get_current_slot(addr, cycle);
        uint8_t mapper_index = get_mapper_index(current_slot, addr, cycle);
        return reg_bank[mapper_index];
    }
    
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
    
    // Получить текущий слот
    uint8_t get_current_slot(uint16_t addr, ECycle cycle) const {
        uint8_t page;
        
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            page = (addr >> 0) & 0x03;  // Для IO: биты A[1:0]
        } else {
            page = (addr >> 14) & 0x03; // Для памяти: биты A[15:14]
        }
        
        uint8_t slot_reg = supervisor_mode ? reg_super_slot : reg_user_slot;
        
        switch (page) {
            case 0: return (slot_reg >> 0) & 0x03;
            case 1: return (slot_reg >> 2) & 0x03;
            case 2: return (slot_reg >> 4) & 0x03;
            case 3: return (slot_reg >> 6) & 0x03;
            default: return 0;
        }
    }
    
    // Получить индекс маппера
    uint8_t get_mapper_index(uint8_t current_slot, uint16_t addr, ECycle cycle) const {
        uint8_t page;
        
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            page = (addr >> 0) & 0x03;  // Для IO: биты A[1:0]
        } else {
            page = (addr >> 14) & 0x03; // Для памяти: биты A[15:14]
        }
        
        return (current_slot << 2) | page;
    }
    
    // Обработка записи в IO порт
    void handle_io_write(uint16_t addr, uint8_t data) {
        // Обработка syscall
        uint8_t syscall_addr = native_mode ? 0xF2 : 0xFFF2;
        if (addr == syscall_addr) {
            reg_syscall_function = data;
            if (!supervisor_mode) {
                supervisor_mode = true;
                supervisor_exit_pending = false;
            }
            return;
        }
        
        // Для других регистров MMU
        if (native_mode) {
            // Native mode: младшие 5 бит адреса
            uint8_t reg_addr = addr & 0x1F;
            handle_native_register_write(reg_addr, data);
        } else {
            // Legacy mode: старший байт адреса
            uint8_t reg_addr = (addr >> 8) & 0xFF;
            handle_legacy_register_write(reg_addr, data);
        }
    }
    
    // Обработка чтения из IO порта
    uint8_t handle_io_read(uint16_t addr) const {
        // Для syscall возвращаем функцию
        uint8_t syscall_addr = native_mode ? 0xF2 : 0xFFF2;
        if (addr == syscall_addr) {
            return reg_syscall_function;
        }
        
        // Для других регистров MMU
        if (native_mode) {
            uint8_t reg_addr = addr & 0x1F;
            return handle_native_register_read(reg_addr);
        } else {
            uint8_t reg_addr = (addr >> 8) & 0xFF;
            return handle_legacy_register_read(reg_addr);
        }
    }
    
    // Обработка доступа к памяти
    void handle_memory_access(uint16_t addr, ECycle cycle) {
        // Для M1 циклов проверяем выход из supervisor mode
        if (cycle == ECycle::OPCODE && supervisor_exit_pending) {
            supervisor_mode = false;
            supervisor_exit_pending = false;
        }
    }
    
    // Обработка записи регистров в Native mode
    void handle_native_register_write(uint8_t reg_addr, uint8_t data) {
        // Получаем текущий слот для записи
        uint8_t current_slot = get_current_slot(0, ECycle::IOWR); // Используем адрес 0 для получения слотов
        
        switch (reg_addr) {
            case 0xF0: // CONTROL
                reg_control = data;
                native_mode = (data & 0x01) != 0;
                if ((data & 0x01) == 0 && supervisor_mode) {
                    supervisor_exit_pending = true;
                }
                break;
                
            case 0xF1: // MMIO PAGE
                reg_mmio_page = data;
                break;
                
            case 0xF3: // CLOCK
                reg_clock = data;
                break;
                
            case 0xFA: // SLOT SUPERVISOR
                reg_super_slot = data;
                break;
                
            case 0xFB: // SLOT USER
                reg_user_slot = data;
                break;
                
            case 0xFC: // MAPPER 0
                reg_bank[(current_slot << 2) | 0] = data;
                break;
                
            case 0xFD: // MAPPER 1
                reg_bank[(current_slot << 2) | 1] = data;
                break;
                
            case 0xFE: // MAPPER 2
                reg_bank[(current_slot << 2) | 2] = data;
                break;
                
            case 0xFF: // MAPPER 3
                reg_bank[(current_slot << 2) | 3] = data;
                break;
                
            default:
                // Расширенные регистры E0-EF
                if (reg_addr < 0x10) {
                    reg_bank[reg_addr] = data;
                }
                break;
        }
    }
    
    // Обработка чтения регистров в Native mode
    uint8_t handle_native_register_read(uint8_t reg_addr) const {
        // Получаем текущий слот для чтения
        uint8_t current_slot = get_current_slot(0, ECycle::IORD);
        
        switch (reg_addr) {
            case 0xF0: return reg_control;
            case 0xF1: return reg_mmio_page;
            case 0xF2: return reg_syscall_function;
            case 0xF3: return reg_clock;
            case 0xFA: return reg_super_slot;
            case 0xFB: return reg_user_slot;
            case 0xFC: return reg_bank[(current_slot << 2) | 0];
            case 0xFD: return reg_bank[(current_slot << 2) | 1];
            case 0xFE: return reg_bank[(current_slot << 2) | 2];
            case 0xFF: return reg_bank[(current_slot << 2) | 3];
            default:
                if (reg_addr < 0x10) {
                    return reg_bank[reg_addr];
                }
                return 0xFF;
        }
    }
    
    // Обработка записи регистров в Legacy mode
    void handle_legacy_register_write(uint8_t reg_addr, uint8_t data) {
        // Аналогично Native mode, но с другими адресами
        uint8_t current_slot = get_current_slot(0, ECycle::IOWR);
        
        switch (reg_addr) {
            case 0xF0: reg_control = data; break;
            case 0xF1: reg_mmio_page = data; break;
            case 0xF3: reg_clock = data; break;
            case 0xFA: reg_super_slot = data; break;
            case 0xFB: reg_user_slot = data; break;
            case 0xFC: reg_bank[(current_slot << 2) | 0] = data; break;
            case 0xFD: reg_bank[(current_slot << 2) | 1] = data; break;
            case 0xFE: reg_bank[(current_slot << 2) | 2] = data; break;
            case 0xFF: reg_bank[(current_slot << 2) | 3] = data; break;
        }
    }
    
    // Обработка чтения регистров в Legacy mode
    uint8_t handle_legacy_register_read(uint8_t reg_addr) const {
        uint8_t current_slot = get_current_slot(0, ECycle::IORD);
        
        switch (reg_addr) {
            case 0xF0: return reg_control;
            case 0xF1: return reg_mmio_page;
            case 0xF2: return reg_syscall_function;
            case 0xF3: return reg_clock;
            case 0xFA: return reg_super_slot;
            case 0xFB: return reg_user_slot;
            case 0xFC: return reg_bank[(current_slot << 2) | 0];
            case 0xFD: return reg_bank[(current_slot << 2) | 1];
            case 0xFE: return reg_bank[(current_slot << 2) | 2];
            case 0xFF: return reg_bank[(current_slot << 2) | 3];
            default: return 0xFF;
        }
    }
};

extern NativeMMU mmu;
