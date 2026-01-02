#include "mmu_native.h"




    // Проверяет, реагирует ли MMU на этот цикл
    bool NativeMMU::check_port(uint16_t addr, ECycle cycle) const {
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
    uint8_t NativeMMU::process_cycle(uint16_t addr, ECycle cycle, uint8_t data_in) {
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
            result = handle_memory_access(addr, cycle, data_in);
        }
        
        return result;
    }
    
    // Транслирует 16-битный адрес в 24-битный физический
    uint32_t NativeMMU::translate_address(uint16_t addr, ECycle cycle) const {
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            // IO доступ

                if ((addr & 0xFF) < 0xC0) {
                    // MMIO доступ
                    return (0xFF000000 | (reg_mmio_page << 8) | (addr & 0xFF));
                } else {
                    // Обычный IO
                    return (0xFF000000 | addr);
                }

        } else {
            // Доступ к памяти


                uint8_t slot = get_current_slot(addr, cycle);
                uint8_t bank = get_current_bank(addr, cycle);

                // Формируем адрес: {slot, bank, offset}
                return ((slot & 0x03) << 22) | 
                       (bank << 14) | 
                       (addr & 0x3FFF);

        }
    }

    uint8_t NativeMMU::get_current_bank(uint16_t addr, ECycle cycle) const {
        // Доступ к памяти
        if (native_mode) {        
            // Получаем текущий слот
            uint8_t current_slot = get_current_slot(addr, cycle);
            
            // Получаем индекс маппера
            uint8_t mapper_index = get_mapper_index(current_slot, addr, cycle);
            
            // Получаем банк
            return reg_bank[mapper_index];
        } else {
            return 0;
        }
    }
 
    // Получить текущий слот
    uint8_t NativeMMU::get_current_slot(uint16_t addr, ECycle cycle) const {
        uint8_t page;
        
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            page = (addr >> 0) & 0x03;  // Для IO: биты A[1:0]
        } else {
            page = (addr >> 14) & 0x03; // Для памяти: биты A[15:14]
        }
        return get_current_slot_for_page(page);
    }
    
    uint8_t NativeMMU::get_current_slot_for_page(uint8_t page) const {
        
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
    uint8_t NativeMMU::get_mapper_index(uint8_t current_slot, uint16_t addr, ECycle cycle) const {
        uint8_t page;
        
        if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
            page = (addr >> 0) & 0x03;  // Для IO: биты A[1:0]
        } else {
            page = (addr >> 14) & 0x03; // Для памяти: биты A[15:14]
        }
        
        return (current_slot << 2) | page;
    }
    
    // Обработка записи в IO порт
    void NativeMMU::handle_io_write(uint16_t addr, uint8_t data) {
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
        handle_native_register_write(addr, data);
    }
    
    // Обработка чтения из IO порта
    uint8_t NativeMMU::handle_io_read(uint16_t addr) const {
        // Для syscall возвращаем функцию
        uint8_t syscall_addr = native_mode ? 0xF2 : 0xFFF2;
        if (addr == syscall_addr) {
            return reg_syscall_function;
        }
        
        // Для других регистров MMU
        return handle_native_register_read(addr);
    }
    
    // Обработка доступа к памяти
    uint8_t NativeMMU::handle_memory_access(uint16_t addr, ECycle cycle, uint8_t data_in) {
        // Для M1 циклов проверяем выход из supervisor mode
        if (cycle == ECycle::OPCODE && supervisor_exit_pending) {
            supervisor_mode = false;
            supervisor_exit_pending = false;
        }
        auto address = translate_address(addr, cycle);
        switch (cycle)
        {
        case ECycle::MWR:
            memory[address] = data_in;
            return 0xFF;
        case ECycle::MRD:
            return memory[address];
        default:
            break;
        }
        return 0xFF;   
    }
    
    // Обработка записи регистров в Native mode
    void NativeMMU::handle_native_register_write(uint8_t reg_addr, uint8_t data) {
        // Получаем текущий слот для записи
        uint8_t current_slot = get_current_slot_for_page(reg_addr & 3); 
        
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
                if (reg_addr >= 0xE0 && reg_addr <= 0xEF) {
                    reg_bank[reg_addr&0x0f] = data;
                }
                break;
        }
    }
    
    // Обработка чтения регистров в Native mode
    uint8_t NativeMMU::handle_native_register_read(uint8_t reg_addr) const {
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
                if (reg_addr >= 0xE0 && reg_addr <= 0xEF) {
                    return reg_bank[reg_addr & 0x0f];
                }
                return 0xFF;
        }
        return 0xFF;
    }
    
