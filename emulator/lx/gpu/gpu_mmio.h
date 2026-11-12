#pragma once
#include <cstdint>
#include <vector>
#include "gpu.h"

namespace lx {

class GpuMmio {
public:
    explicit GpuMmio(GPU& gpu) : gpu_(gpu) {}

    uint8_t read(uint8_t reg);
    void    write(uint8_t reg, uint8_t v);

    void tick(int cpu_cycles) { gpu_.tick(cpu_cycles); }
    bool irq() const { return gpu_.irq_pending(); }

private:
    GPU& gpu_;
    uint32_t entry_ = 0;
    std::vector<uint16_t> packet_;
    uint16_t packet_lo_ = 0;
    bool     packet_have_lo_ = false;
    bool     packet_err_ = false;
    uint8_t  clk_num_ = 1, clk_den_ = 1;
};

} // namespace lx