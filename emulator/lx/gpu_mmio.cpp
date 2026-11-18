// === FILE: gpu_mmio.cpp ===
#include "gpu_mmio.h"

namespace lx {

GpuMmio::GpuMmio()
    : mem_(0x100000), gpu_(mem_), host_(gpu_) {
    reset();
}

GpuMmio::GpuMmio(uint8_t* vram_ptr, uint32_t vram_size)
    : mem_(vram_ptr, vram_size), gpu_(mem_), host_(gpu_) {
    reset();
}

void GpuMmio::attach_memory(uint8_t* vram_ptr, uint32_t vram_size) {
    mem_.attach(vram_ptr, vram_size);
}

bool GpuMmio::load_bootloader_hex(const std::string& path) {
    return gpu_.load_rom_hex(path);
}

void GpuMmio::reset() {
    host_.reset();
    gpu_.reset();
    clk_acc_ = 0;
}

uint8_t GpuMmio::read(uint8_t reg) {
    // В новом RTL хост-интерфейс занимает 5 регистров (0x00..0x04):
    // 0..2: GPU_ADDR0..2, 3: COMMAND, 4: STATUS ({HALT, RESULT[6:0]})
    return host_.read(reg & 0x07);
}

void GpuMmio::write(uint8_t reg, uint8_t val) {
    host_.write(reg & 0x07, val);
}

void GpuMmio::tick(int z80_cycles) {
    if (z80_cycles <= 0 || gpu_.halted()) return;

    clk_acc_ += z80_cycles * clk_num_;
    int gpu_steps = clk_acc_ / clk_den_;
    clk_acc_ %= clk_den_;

    if (gpu_steps > 0) {
        gpu_.tick(gpu_steps);
    }
}

} // namespace lx
