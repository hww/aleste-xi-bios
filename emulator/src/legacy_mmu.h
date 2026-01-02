#ifndef _LEGACY_MMU_
#define _LEGACY_MMU_

#include <cstdint>
#include <cstring>


class LegacyMMU {
private:
    // Указатель на всю память системы
    uint8_t* memory;
    
    // Внутренние регистры (CPC 6128 совместимые)
    uint8_t reg_rmr;        // RMR: Control Interrupt counter, ROM mapping
    uint8_t reg_mmr;        // MMR: RAM memory mapping  
    uint8_t reg_upper_rom;  // Upper ROM selection register
    
    // Текущий режим работы
    bool legacy_mode;
    
    // Константы
    static constexpr uint8_t LOWER_ROM_BANK = 0xFC;
    static constexpr uint8_t CPC_RAM_SLOT = 0x00;
    static constexpr uint8_t CPC_ROM_SLOT = 0x01;
    static constexpr uint32_t IO_SPACE_BASE = 0xFF0000;
    
    // Вспомогательные методы
    uint8_t get_rmr_config() const { return reg_mmr & 0x07; }
    uint8_t get_rmr_bank() const   { return (reg_mmr >> 3) & 0x07; }
    uint8_t get_graphic_mode() const { return reg_rmr & 0x03; }
    bool is_lower_rom_enabled() const { return !(reg_rmr & 0x04); }
    bool is_upper_rom_enabled() const { return !(reg_rmr & 0x08); }
    bool is_irq_control() const    { return reg_rmr & 0x10; }
    
    // Определение типа доступа
    bool is_7fxx_address(uint16_t addr) const { return (addr >> 8) == 0x7F; }
    bool is_dfxx_address(uint16_t addr) const { return (addr >> 8) == 0xDF; }
    
    // Вычисление номера RAM страницы по конфигурации
    uint8_t calculate_ram_page(uint16_t addr) const {
        uint8_t config = get_rmr_config();
        uint8_t page_select = (addr >> 14) & 0x03; // Бит 15:14
        
        switch (config) {
            case 0x00: // Config 0: RAM_0, RAM_1, RAM_2, RAM_3
                return page_select;
                
            case 0x01: // Config 1: RAM_0, RAM_1, RAM_2, RAM_7
                return (page_select == 3) ? 7 : page_select;
                
            case 0x02: // Config 2: RAM_4, RAM_5, RAM_6, RAM_7
                return page_select + 4;
                
            case 0x03: // Config 3: RAM_0, RAM_3, RAM_2, RAM_7
                if (page_select == 1) return 3;
                if (page_select == 3) return 7;
                return page_select;
                
            case 0x04: // Config 4: RAM_0, RAM_4, RAM_2, RAM_3
                if (page_select == 1) return 4;
                if (page_select == 3) return 3;
                return page_select;
                
            case 0x05: // Config 5: RAM_0, RAM_5, RAM_2, RAM_3
                if (page_select == 1) return 5;
                if (page_select == 3) return 3;
                return page_select;
                
            case 0x06: // Config 6: RAM_0, RAM_6, RAM_2, RAM_3
                if (page_select == 1) return 6;
                if (page_select == 3) return 3;
                return page_select;
                
            case 0x07: // Config 7: RAM_0, RAM_7, RAM_2, RAM_3
                if (page_select == 1) return 7;
                if (page_select == 3) return 3;
                return page_select;
                
            default:
                return 0;
        }
    }
    
    // Обработка записи во внутренние регистры
    void handle_internal_io_write(uint16_t addr, uint8_t data) {
        if (is_7fxx_address(addr)) {
            uint8_t reg_type = (data >> 6) & 0x03;
            if (reg_type == 0x02) {      // RMR write
                reg_rmr = data;
            } else if (reg_type == 0x03) { // MMR write
                reg_mmr = data;
            }
            // Другие регистры (палитра) игнорируем
        } 
        else if (is_dfxx_address(addr)) { // Upper ROM select
            reg_upper_rom = data;
        }
    }
    
    // Получение данных из внутренних регистров
    uint8_t handle_internal_io_read(uint16_t addr) const {
        // Для чтения внутренних регистров обычно возвращается 0xFF
        // или последнее записанное значение, зависит от реализации
        return 0xFF;
    }
    
    // Трансляция адреса для памяти
    uint32_t translate_memory_address(uint16_t addr) const {
        uint8_t ram_page = calculate_ram_page(addr);
        
        // Проверка доступа к ROM
        uint16_t page_area = (addr >> 14) & 0x03;
        
        // Lower ROM access (0000-3FFF)
        if (page_area == 0 && is_lower_rom_enabled()) {
            // ROM доступ: SLOT(2) + BANK(8) + offset(16)
            return (static_cast<uint32_t>(CPC_ROM_SLOT) << 22) |
                   (static_cast<uint32_t>(LOWER_ROM_BANK) << 16) |
                   addr;
        }
        
        // Upper ROM access (C000-FFFF)
        if (page_area == 3 && is_upper_rom_enabled()) {
            // ROM доступ: SLOT(2) + BANK(8) + offset(14)
            return (static_cast<uint32_t>(CPC_ROM_SLOT) << 22) |
                   (static_cast<uint32_t>(reg_upper_rom) << 16) |
                   (addr & 0x3FFF);
        }
        
        // RAM доступ
        if (ram_page & 0x04) { // Расширенная RAM (страницы 4-7)
            // RAM доступ: SLOT(2) + EXT_FLAG(3) + BANK(3) + PAGE(2) + offset(14)
            return (static_cast<uint32_t>(CPC_RAM_SLOT) << 22) |
                   (0x02 << 19) | // EXT_FLAG = 010
                   (static_cast<uint32_t>(get_rmr_bank()) << 16) |
                   (static_cast<uint32_t>(ram_page & 0x03) << 14) |
                   (addr & 0x3FFF);
        } else { // Стандартная RAM (страницы 0-3)
            // RAM доступ: SLOT(2) + ZEROS(6) + offset(16)
            return (static_cast<uint32_t>(CPC_RAM_SLOT) << 22) |
                   addr;
        }
    }
    
    // Трансляция адреса для IO
    uint32_t translate_io_address(uint16_t addr) const {
        // IO адреса мапятся в пространство 3Fxxxxh
        return IO_SPACE_BASE | addr;
    }
    
    // Физический доступ к памяти
    uint8_t physical_read(uint32_t phys_addr) const {
        // Предполагаем, что memory указывает на начало всей памяти системы
        if (phys_addr < (1 << 24)) { // 24-битный адрес
            return memory[phys_addr];
        }
        return 0xFF;
    }
    
    void physical_write(uint32_t phys_addr, uint8_t data) {
        if (phys_addr < (1 << 24)) {
            memory[phys_addr] = data;
        }
    }

public:
    // Конструктор
    LegacyMMU(uint8_t* mem, Mode mode = Mode::LEGACY) 
        : memory(mem), legacy_mode(mode == Mode::LEGACY) {
        reset();
    }
    
    // Сброс
    void reset() {
        reg_rmr = 0b10000000;        // Default RMR: Mode 0, ROMs enabled
        reg_mmr = 0b11000000;        // Default MMR: Config 0, Bank 0
        reg_upper_rom = 0x00;        // Default upper ROM bank 0
    }
    
    // Проверяет, реагирует ли MMU на этот цикл
    bool check_port(uint16_t addr, ECycle cycle) const {
        if (!legacy_mode) return false;
        
        // MMU реагирует на порты 7Fxxh и DFxxh для записи в IO
        if (cycle == ECycle::IOWR) {
            return is_7fxx_address(addr) || is_dfxx_address(addr);
        }
        
        // MMU реагирует на все IO чтения и записи
        if (cycle == ECycle::IORD || cycle == ECycle::IOWR) {
            return true;
        }
        
        // Для памяти MMU всегда активен в legacy режиме
        if (cycle == ECycle::MRD || cycle == ECycle::MWR || cycle == ECycle::OPCODE) {
            return true;
        }
        
        return false;
    }
    
    // Выполняет цикл чтения/записи
    uint8_t process_cycle(uint16_t addr, ECycle cycle, uint8_t data_in = 0x00) {
        if (!legacy_mode) return 0xFF;
        
        switch (cycle) {
            case ECycle::IOWR: {
                // Запись в IO порт
                if (is_7fxx_address(addr) || is_dfxx_address(addr)) {
                    // Внутренние регистры MMU
                    handle_internal_io_write(addr, data_in);
                    return 0xFF;
                } else {
                    // Внешние IO порты
                    uint32_t phys_addr = translate_io_address(addr);
                    physical_write(phys_addr, data_in);
                    return 0xFF;
                }
            }
                
            case ECycle::IORD: {
                // Чтение из IO порта
                if (is_7fxx_address(addr) || is_dfxx_address(addr)) {
                    // Внутренние регистры MMU
                    return handle_internal_io_read(addr);
                } else {
                    // Внешние IO порты
                    uint32_t phys_addr = translate_io_address(addr);
                    return physical_read(phys_addr);
                }
            }
                
            case ECycle::MWR: 
            case ECycle::MRD:
            case ECycle::OPCODE: {
                // Доступ к памяти
                uint32_t phys_addr = translate_memory_address(addr);
                
                if (cycle == ECycle::MWR) {
                    physical_write(phys_addr, data_in);
                    return 0xFF;
                } else {
                    return physical_read(phys_addr);
                }
            }
                
            default:
                return 0xFF;
        }
    }
    
    // Транслирует 16-битный адрес в 24-битный физический
    uint32_t translate_address(uint16_t addr, ECycle cycle) const {
        if (!legacy_mode) return addr; // В native режиме прямой доступ
        
        if (cycle == ECycle::IORD || cycle == ECycle::IOWR) {
            return translate_io_address(addr);
        } else {
            return translate_memory_address(addr);
        }
    }
    
    // Управление режимом
    void set_mode(Mode mode) {
        legacy_mode = (mode == Mode::LEGACY);
        if (legacy_mode) {
            reset();
        }
    }
    
    Mode get_mode() const {
        return legacy_mode ? Mode::LEGACY : Mode::NATIVE;
    }
    
    // Получение состояния регистров (для отладки)
    uint8_t get_rmr() const { return reg_rmr; }
    uint8_t get_mmr() const { return reg_mmr; }
    uint8_t get_upper_rom() const { return reg_upper_rom; }
    
    // Получение конфигурации графики
    uint8_t get_graphic_mode_config() const { return get_graphic_mode(); }
    bool get_irq_control() const { return is_irq_control(); }
    
    // Получение статуса ROM
    bool is_lower_rom_active() const { return is_lower_rom_enabled(); }
    bool is_upper_rom_active() const { return is_upper_rom_enabled(); }
    
    // Проверка типа доступа (для отладки)
    bool is_rom_access(uint16_t addr) const {
        if (!legacy_mode) return false;
        
        uint16_t page_area = (addr >> 14) & 0x03;
        if (page_area == 0 && is_lower_rom_enabled()) return true;
        if (page_area == 3 && is_upper_rom_enabled()) return true;
        
        return false;
    }
    
    bool is_ram_access(uint16_t addr) const {
        return legacy_mode && !is_rom_access(addr);
    }
    
    bool is_io_access(uint16_t addr, ECycle cycle) const {
        return legacy_mode && (cycle == ECycle::IORD || cycle == ECycle::IOWR);
    }
};

#endif // _LEGACY_MMU_