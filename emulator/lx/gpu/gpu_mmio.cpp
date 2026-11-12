#include "gpu_mmio.h"

namespace lx {

// MMIO-карта:
//  0x00 W: start (1 = запуск); R: status (bit0 RUN, bit1 IDLE, bit2 ERROR, bit3 IRQ)
//  0x01 W: entry[7:0]
//  0x02 W: entry[15:8]
//  0x03 W: entry[23:16]
//  0x04 R: status (как 0x00)
//  0x05 R: error_code[7:0]
//  0x06 R: error_code[15:8]
//  0x07 R: flags
//  0x08 W: packet low byte
//  0x09 W: packet high byte
//  0x0A W: commit (1 = load packet); R: 0=OK, 1=error
//  0x0B W: reset
//  0x0C R: buffer_error
//  0x0D W: clear buffer_error
//  0x0E W/R: clk_num
//  0x0F W/R: clk_den
//  0x10 R: irq_code[7:0]
//  0x11 R: irq_code[15:8]
//  0x12 W: clear_irq (1)

uint8_t GpuMmio::read(uint8_t reg) {
    switch (reg) {
    case 0x00: case 0x04: {
        uint8_t s = 0;
        if (gpu_.busy())        s |= 1;
        if (gpu_.halted())      s |= 2;
        if (gpu_.error())       s |= 4;
        if (gpu_.irq_pending()) s |= 8;
        return s;
    }
    case 0x05: return gpu_.error_code() & 0xFF;
    case 0x06: return (gpu_.error_code() >> 8) & 0xFF;
    case 0x07: return gpu_.flags();
    case 0x0A: return packet_err_ ? 1 : 0;
    case 0x0C: return gpu_.buffer_error() ? 1 : 0;
    case 0x0E: return clk_num_;
    case 0x0F: return clk_den_;
    case 0x10: return gpu_.irq_code() & 0xFF;
    case 0x11: return (gpu_.irq_code() >> 8) & 0xFF;
    default: return 0;
    }
}

void GpuMmio::write(uint8_t reg, uint8_t v) {
    switch (reg) {
    case 0x00:
        if (v & 1) { packet_err_ = false; gpu_.start(entry_); }
        break;
    case 0x01: entry_ = (entry_ & 0xFFFF00u) | v; break;
    case 0x02: entry_ = (entry_ & 0xFF00FFu) | ((uint32_t)v << 8); break;
    case 0x03: entry_ = (entry_ & 0x00FFFFu) | ((uint32_t)v << 16); break;
    case 0x08: packet_lo_ = v; packet_have_lo_ = true; break;
    case 0x09:
        if (packet_have_lo_) {
            packet_.push_back(packet_lo_ | ((uint16_t)v << 8));
            packet_have_lo_ = false;
        }
        break;
    case 0x0A:
        if (v & 1) {
            packet_err_ = !gpu_.load_packet(packet_.data(), packet_.size());
            packet_.clear();
        }
        break;
    case 0x0B:
        gpu_.reset(); entry_ = 0; packet_.clear();
        packet_have_lo_ = false; packet_err_ = false;
        break;
    case 0x0D: gpu_.clear_buffer_error(); break;
    case 0x0E: clk_num_ = v ? v : 1; break;
    case 0x0F: clk_den_ = v ? v : 1; break;
    case 0x12: if (v & 1) gpu_.clear_irq(); break;
    default: break;
    }
}

} // namespace lx