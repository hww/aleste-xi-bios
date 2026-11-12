#pragma once
#include <cstdint>

namespace lx {

enum PixFmt : uint8_t {
    FMT_1BPP = 0, FMT_RSV = 1, FMT_4BPP = 2, FMT_8BPP = 3,
    FMT_16BPP = 4, FMT_PASS_LO8 = 5, FMT_PASS_HI8 = 6, FMT_PASS16 = 7,
};

class AddrPacker {
public:
    // idx — пиксельный индекс внутри страницы; page — старшие 8 бит адреса.
    // addr24 = (page<<16) | ((idx>>shift)<<1); pos = idx & mask.
    // base_lo16 — опциональные младшие 16 бит base (в FPGA не используются).
    static void compute(uint32_t idx, uint8_t page,
                        uint32_t base_lo16, uint32_t /*stride unused*/,
                        PixFmt fmt, uint32_t& addr24, uint8_t& pos);
};

} // namespace lx