#include "addr_packer.h"

namespace lx {

void AddrPacker::compute(uint32_t x_int, uint32_t y_int,
                         uint32_t base, uint32_t stride_bytes,
                         PixFmt fmt, uint32_t& addr24, uint8_t& pos) {
    uint32_t stride_words = stride_bytes >> 1;
    uint32_t word_offset = y_int * stride_words;
    switch (fmt) {
    case FMT_4BPP:                       // 4 пикселя в 16-битном слове
        pos = (uint8_t)(x_int & 3);
        word_offset += (x_int >> 2);
        break;
    case FMT_8BPP:                       // 2 пикселя
        pos = (uint8_t)(x_int & 1);
        word_offset += (x_int >> 1);
        break;
    case FMT_1BPP:                       // 16 пикселей
        pos = (uint8_t)(x_int & 15);
        word_offset += (x_int >> 4);
        break;
    case FMT_16BPP:                      // 1 пиксель
        pos = 0;
        word_offset += x_int;
        break;
    default:                             // PASS16/LO8/HI8
        pos = 0;
        word_offset += x_int;
        break;
    }
    addr24 = (base + word_offset * 2) & 0xFFFFFF;
}

} // namespace lx