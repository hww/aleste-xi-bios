#include "palette.h"
#include "hex_dump.h"
#include "bus.h"

void Palette::reset() {
    palette_index = 0;
    control_reg = 0;
    palette_modifier = 0;
    border_color = 0x888;
    low_byte_buffer = 0;
    
    // Инициализация палитры градиентом
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i >> 5) & 0x07;
        uint8_t g = (i >> 2) & 0x07;
        uint8_t b = i & 0x03;
        palette_ram[i] = (r << 8) | (g << 4) | b;
    }
    palette_ram[1] = 0xFF00;
}


// Проверяет, реагирует ли палитра на этот цикл
bool Palette::check_port(uint16_t addr, ECycle cycle) const {
    // Проверяем native access (IO порты)
    if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
        // Native: 7 регистров по 8 бит
        uint8_t reg = addr & 0x07;
        if (reg <= 6) {  // Регистры 0-6
            return true;
        }
    }
    
    // Проверяем legacy CPC access
    if (cfg_legacy && cycle == ECycle::IOWR) {
        // CPC Gate Array: запись по любому адресу с данными[7:6] = 00 или 01
        // (это проверяется в process_cycle по содержимому данных)
        return true;
    }
    
    return false;
}

// Выполняет цикл чтения/записи
uint8_t Palette::process_cycle(uint16_t addr, ECycle cycle, uint8_t data_in) {
    uint8_t data_out = 0x00;
    
    // Legacy CPC access (только запись)
    if (cfg_legacy && cycle == ECycle::IOWR) {
        // CPC Gate Array регистры определяются по данным[7:6]
        uint8_t reg_type = (data_in >> 6) & 0x03;
        
        switch (reg_type) {
            case 0x00:  // REG_COLOR_INDEX_CPC
                palette_index = data_in & 0x1F;  // 5 бит индекса
                break;
                
            case 0x01:  // REG_COLOR_DATA_CPC
                if (palette_index & 0x10) {  // Бит 4 = бордюр
                    border_color = cpc_to_rgb12(data_in & 0x3F);
                } else {
                    uint8_t idx = palette_index & 0x0F;
                    palette_ram[idx] = cpc_to_rgb12(data_in & 0x3F);
                }
                break;
                
            default:
                // Другие регистры CPC не относятся к палитре
                break;
        }
        return 0x00;
    }
    
    // Native access
    if (cycle == ECycle::IOWR || cycle == ECycle::IORD) {
        uint8_t reg = addr & 0x07;
        
        if (cycle == ECycle::IOWR) {
            // Запись
            switch (static_cast<RegAddr>(reg)) {
                case RegAddr::COLOR_INDEX:
                    palette_index = data_in;
                    break;
                    
                case RegAddr::COLOR_DATA_LO:
                    switch (palette_write_mode()) {
                        case WriteMode::MODE_CPC:
                            palette_ram[palette_index] = cpc_to_rgb12(data_in);
                            if (auto_increment()) {
                                palette_index++;
                            }
                            break;
                            
                        case WriteMode::MODE_12BIT:
                            low_byte_buffer = data_in;
                            break;
                            
                        case WriteMode::MODE_MSX:
                        case WriteMode::MODE_YJK:
                            palette_ram[palette_index] = msx_to_rgb12(
                                data_in, 
                                palette_write_mode() == WriteMode::MODE_YJK
                            );
                            if (auto_increment()) {
                                palette_index++;
                            }
                            break;
                    }
                    break;
                    
                case RegAddr::COLOR_DATA_HI:
                    if (palette_write_mode() == WriteMode::MODE_12BIT) {
                        palette_ram[palette_index] = 
                            ((data_in & 0x0F) << 8) | low_byte_buffer;
                        if (auto_increment()) {
                            palette_index++;
                        }
                    }
                    break;
                    
                case RegAddr::COLOR_BORDER_LO:
                    switch (palette_write_mode()) {
                        case WriteMode::MODE_12BIT:
                            low_byte_buffer = data_in;
                            break;
                            
                        case WriteMode::MODE_CPC:
                            border_color = cpc_to_rgb12(data_in);
                            break;
                            
                        case WriteMode::MODE_MSX:
                        case WriteMode::MODE_YJK:
                            border_color = msx_to_rgb12(
                                data_in,
                                palette_write_mode() == WriteMode::MODE_YJK
                            );
                            break;
                    }
                    break;
                    
                case RegAddr::COLOR_BORDER_HI:
                    if (palette_write_mode() == WriteMode::MODE_12BIT) {
                        border_color = ((data_in & 0x0F) << 8) | low_byte_buffer;
                    }
                    break;
                    
                case RegAddr::COLOR_CTRL:
                    control_reg = data_in;
                    break;
                    
                case RegAddr::COLOR_MODIFIER:
                    palette_modifier = data_in;
                    break;
            }
        } else {
            // Чтение
            switch (static_cast<RegAddr>(reg)) {
                case RegAddr::COLOR_INDEX:
                    data_out = palette_index;
                    break;
                    
                case RegAddr::COLOR_DATA_LO:
                    data_out = palette_ram[palette_index] & 0xFF;
                    break;
                    
                case RegAddr::COLOR_DATA_HI:
                    data_out = (palette_ram[palette_index] >> 8) & 0x0F;
                    break;
                    
                case RegAddr::COLOR_BORDER_LO:
                    data_out = border_color & 0xFF;
                    break;
                    
                case RegAddr::COLOR_BORDER_HI:
                    data_out = (border_color >> 8) & 0x0F;
                    break;
                    
                case RegAddr::COLOR_CTRL:
                    data_out = control_reg;
                    break;
                    
                case RegAddr::COLOR_MODIFIER:
                    data_out = palette_modifier;
                    break;
                    
                default:
                    data_out = 0x00;
                    break;
            }
        }
    }
    
    return data_out;
}

// Транслирует 8-битный индекс цвета в 12-битный RGB
uint16_t Palette::translate_color(uint8_t pixel_index, bool is_border) const {
    if (is_border) {
        return border_color;
    }
    
    uint8_t modified_idx = modify_pixel_index(pixel_index);
    return palette_ram[modified_idx];
}

uint32_t Palette::translate_color_argb32(uint8_t pixel_index, bool is_border) const {
    if (is_border) {
        return to_argb32(border_color);
    }
    
    uint8_t modified_idx = modify_pixel_index(pixel_index);
    return to_argb32(palette_ram[modified_idx]);
}

// Конвертирует 12-битный цвет в 32-битный ARGB (для отображения)
uint32_t Palette::to_argb32(uint16_t rgb12) const {
    uint8_t r = (rgb12 >> 8) & 0x0F;
    uint8_t g = (rgb12 >> 4) & 0x0F;
    uint8_t b = rgb12 & 0x0F;
    
    // Расширяем 4-битные компоненты до 8-битных
    r = (r << 4) | r;
    g = (g << 4) | g;
    b = (b << 4) | b;
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}