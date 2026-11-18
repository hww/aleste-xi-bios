// === FILE: gpu_mmio.h ===
#pragma once
#include <cstdint>
#include <string>

// Подключаем заголовки нового эмулятора из сабмодуля lx-gpu-rtl/emul
#include "memory.h"
#include "gpu_top.h"
#include "host_if.h"

namespace lx {

class GpuMmio {
public:
    GpuMmio();
    explicit GpuMmio(uint8_t* vram_ptr, uint32_t vram_size);

    // Подключение к общей видеопамяти платформы (без копирования)
    void attach_memory(uint8_t* vram_ptr, uint32_t vram_size);

    // Загрузка микрокода загрузчика (из boot/bootloader.asm или .hex)
    bool load_bootloader_hex(const std::string& path);
    void write_rom(uint16_t addr, uint32_t word) { gpu_.write_rom(addr, word); }

    // 8-битный MMIO-интерфейс для Z80 (регистры 0x00..0x04)
    uint8_t read(uint8_t reg);
    void    write(uint8_t reg, uint8_t val);

    // Продвижение работы GPU пропорционально тактам Z80
    void tick(int z80_cycles);

    // Состояние прерывания и сброс
    bool irq() const { return gpu_.irq(); }
    void ack_irq()   { gpu_.ack_irq(); }
    void reset();

    // Настройка соотношения частот (по умолчанию GPU быстрее Z80 в 2..3 раза)
    void set_clock_ratio(uint8_t num, uint8_t den) {
        clk_num_ = num ? num : 1;
        clk_den_ = den ? den : 1;
    }

    GpuTop& core() { return gpu_; }

private:
    Memory16 mem_;
    GpuTop   gpu_;
    HostIf   host_;

    uint8_t  clk_num_ = 3; // например, 54 МГц GPU / 16 МГц Z80
    uint8_t  clk_den_ = 1;
    int      clk_acc_ = 0;
};

} // namespace lx
