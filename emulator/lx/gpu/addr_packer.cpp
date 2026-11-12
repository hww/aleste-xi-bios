#include "addr_packer.h"

namespace lx {

void AddrPacker::compute(uint32_t idx, uint8_t page,
                         uint32_t base_lo16, uint32_t /*stride*/,
                         PixFmt fmt, uint32_t& addr24, uint8_t& pos) {
    uint32_t word_offset = 0;
    switch (fmt) {
    case FMT_4BPP:  pos = (uint8_t)(idx & 3);  word_offset = idx >> 2; break;
    case FMT_8BPP:  pos = (uint8_t)(idx & 1);  word_offset = idx >> 1; break;
    case FMT_1BPP:  pos = (uint8_t)(idx & 15); word_offset = idx >> 4; break;
    case FMT_16BPP: pos = 0;                    word_offset = idx;      break;
    default:        pos = 0;                    word_offset = idx;      break;
    }
    addr24 = (((uint32_t)page << 16) | (base_lo16 & 0xFFFF) | (word_offset << 1)) & 0xFFFFFF;
}

} // namespace lx