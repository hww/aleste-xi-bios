#include "gpu_mmio.h"

namespace lx {

uint8_t GpuMmio::read(uint8_t reg) {
    switch (reg) {
    case 0x00: return 0;
    case 0x01: return entry_ & 0xFF;
    case 0x02: return (entry_ >> 8) & 0xFF;
    case 0x03: return (entry_ >> 16) & 0xFF;
    case 0x04: {
        uint8_t s = 0;
        if (gpu_.busy())        s |= 1;
        if (gpu_.halted())      s |= 2;
        if (gpu_.error())       s |= 4;
        if (gpu_.irq())         s |= 8;
        return s;
    }
    case 0x05: return gpu_.irq_code() & 0xFF;
    case 0x06: return (gpu_.irq_code() >> 8) & 0xFF;
    case 0x07: return 0;
    case 0x08: return 0;
    case 0x09: return (uint8_t)(packet_.size() & 0xFF);
    case 0x0A: return 0;
    case 0x0B: return 0;
    case 0x0C: return packet_err_ ? 1 : 0;
    case 0x0D: return clk_num_;
    case 0x0E: return clk_den_;
    }
    return 0xFF;
}

void GpuMmio::write(uint8_t reg, uint8_t v) {
    switch (reg) {
    case 0x00: if (v & 1) gpu_.start(entry_); break;
    case 0x01: entry_ = (entry_ & 0xFFFF00) | v; break;
    case 0x02: entry_ = (entry_ & 0xFF00FF) | ((uint32_t)v << 8); break;
    case 0x03: entry_ = (entry_ & 0x00FFFF) | ((uint32_t)v << 16); break;
    case 0x07: if (v & 1) gpu_.clear_irq(); break;
    case 0x08:
        if (!packet_have_lo_) { packet_lo_ = v; packet_have_lo_ = true; }
        else { packet_.push_back((uint16_t)(packet_lo_ | (v << 8))); packet_have_lo_ = false; }
        break;
    case 0x0A:
        if (v & 1) {
            packet_err_ = !gpu_.load_packet(packet_.data(), packet_.size());
            packet_.clear();
            packet_have_lo_ = false;
        }
        break;
    case 0x0B: if (v & 1) gpu_.reset(); break;
    case 0x0D: clk_num_ = v ? v : 1; gpu_.set_clock_ratio(clk_num_, clk_den_); break;
    case 0x0E: clk_den_ = v ? v : 1; gpu_.set_clock_ratio(clk_num_, clk_den_); break;
    }
}

} // namespace lx