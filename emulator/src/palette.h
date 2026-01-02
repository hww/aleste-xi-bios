#ifndef _PALETTE_H_
#define _PALETTE_H_

#include <cstdint>
#include <array>
#include <functional>

#include "forward_decl.h"


class Palette {
private:
    // Внутренние регистры
    uint8_t palette_index = 0;
    uint8_t control_reg = 0;
    uint8_t palette_modifier = 0;
    uint16_t border_color = 0x888;  // 12-bit цвет бордюра
    std::array<uint16_t, 256> palette_ram;  // 256 записей по 12 бит
    
    // Буфер для младшего байта (для 12-битного режима)
    uint8_t low_byte_buffer = 0;
    
    // Флаги доступа
    bool cfg_legacy = false;
    
    // Адреса регистров
    enum class RegAddr : uint8_t {
        COLOR_INDEX     = 0,
        COLOR_DATA_LO   = 1,
        COLOR_DATA_HI   = 2,
        COLOR_BORDER_LO = 3,
        COLOR_BORDER_HI = 4,
        COLOR_CTRL      = 5,
        COLOR_MODIFIER  = 6
    };
    
    // Режимы записи палитры
    enum class WriteMode : uint8_t {
        MODE_12BIT = 0,
        MODE_CPC   = 1,
        MODE_MSX   = 2,
        MODE_YJK   = 3
    };
    
    // Флаги из регистра управления
    bool modifier_enabled() const { return control_reg & 0x80; }
    bool modifier_is_xor() const   { return control_reg & 0x40; }
    bool auto_increment() const    { return control_reg & 0x20; }
    WriteMode palette_write_mode() const { return static_cast<WriteMode>(control_reg & 0x03); }
    
    // Преобразователи цветов
    uint16_t cpc_to_rgb12(uint8_t cpc_color) const {
        // CPC цвет: формат R2G2B2
        uint8_t r = (cpc_color & 0x20) ? 0xF : 0x0;
        uint8_t g = (cpc_color & 0x10) ? 0xF : 0x0;
        uint8_t b = (cpc_color & 0x08) ? 0xF : 0x0;
        // Интенсивность для младших битов
        r |= (cpc_color & 0x04) ? 0x7 : 0x0;
        g |= (cpc_color & 0x02) ? 0x7 : 0x0;
        b |= (cpc_color & 0x01) ? 0x7 : 0x0;
        return (r << 8) | (g << 4) | b;
    }
    
    uint16_t msx_to_rgb12(uint8_t msx_color, bool yjk_mode) const {
        if (yjk_mode) {
            // YJK режим - упрощенная конвертация
            uint8_t y = (msx_color >> 4) & 0x0F;
            uint8_t jk = msx_color & 0x0F;
            return ((y * 0x11) << 8) | ((jk * 0x11) << 4) | (jk * 0x11);
        } else {
            // MSX режим: формат R2G2B2
            uint8_t r = ((msx_color >> 4) & 0x03) * 0x55;
            uint8_t g = ((msx_color >> 2) & 0x03) * 0x55;
            uint8_t b = (msx_color & 0x03) * 0x55;
            return (r << 4) | (g << 0) | (b >> 4);  // Адаптация к 4-4-4
        }
    }
    
    // Модификатор индекса пикселя
    uint8_t modify_pixel_index(uint8_t pixel_index) const {
        if (!modifier_enabled()) {
            return pixel_index;
        }
        if (modifier_is_xor()) {
            return pixel_index ^ palette_modifier;
        } else {
            return pixel_index | palette_modifier;
        }
    }

public:
    Palette() {
        reset();
    }

    void reset();

    // Проверяет, реагирует ли палитра на этот цикл
    bool check_port(uint16_t addr, ECycle cycle) const;

    // Выполняет цикл чтения/записи
    uint8_t process_cycle(uint16_t addr, ECycle cycle, uint8_t data_in = 0x00);

    // Транслирует 8-битный индекс цвета в 12-битный RGB
    uint16_t translate_color(uint8_t pixel_index, bool is_border = false) const;
    
    uint32_t translate_color_argb32(uint8_t pixel_index, bool is_border = false) const;
    

    // Конвертирует 12-битный цвет в 32-битный ARGB (для отображения)
    uint32_t to_argb32(uint16_t rgb12) const;
    
    // Установка режима legacy/native
    void set_legacy_mode(bool enable) {
        cfg_legacy = enable;
    }
    
    // Получение текущего цвета бордюра
    uint16_t get_border_color() const {
        return border_color;
    }
    
    // Получение цвета из палитры по индексу
    uint16_t get_palette_color(uint8_t index) const {
        return palette_ram[index];
    }
    
    // Установка цвета в палитру (прямой доступ)
    void set_palette_color(uint8_t index, uint16_t color) {
        palette_ram[index] = color & 0x0FFF;
    }
};

#endif // _PALLETE_H_
