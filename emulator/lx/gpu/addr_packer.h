#pragma once
#include <cstdint>

namespace lx {

enum PixFmt : uint8_t {
    FMT_1BPP = 0, FMT_RSV = 1, FMT_4BPP = 2, FMT_8BPP = 3,
    FMT_16BPP = 4, FMT_PASS_LO8 = 5, FMT_PASS_HI8 = 6, FMT_PASS16 = 7,
};

// Единый регистр режима канала (3 бита fmt).
// pos больше не хранится — его вычисляет AddrPacker.
struct ChannelMode {
    uint8_t fmt : 3;
    uint8_t _rsv : 5;
};

class AddrPacker {
public:
    // x_int, y_int — целые части координат (12 бит каждое)
    // base — база (в байтах), stride_bytes — pitch в байтах
    // Выход: addr24 — адрес слова (в байтах), pos — позиция пикселя в слове
    static void compute(uint32_t x_int, uint32_t y_int,
                        uint32_t base, uint32_t stride_bytes,
                        PixFmt fmt, uint32_t& addr24, uint8_t& pos);
};

} // namespace lx