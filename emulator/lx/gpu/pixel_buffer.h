#pragma once
#include <cstdint>
#include "memory.h"

namespace lx {

enum Channel : uint8_t {
    CH_SOURCE = 0, CH_DEST = 1, CH_STM = 2, CH_RSV = 3,
};

struct BufferEntry {
    uint16_t tag = 0;      // word-address внутри страницы (addr24 >> 1)
    uint16_t data = 0;
    uint8_t  page = 0;
    bool     valid = false;
    bool     dirty = false;
};

class PixelBuffer {
public:
    explicit PixelBuffer(Memory16& mem) : mem_(mem) {}

    void reset();
    int  set_page(Channel ch, uint8_t page);
    int  read (Channel ch, uint32_t addr24, uint16_t& out);
    int  write(Channel ch, uint32_t addr24, uint16_t  data);
    int  flush();

    bool error() const { return error_; }
    void clear_error() { error_ = false; }

    uint64_t hits = 0, misses = 0, writebacks = 0;

private:
    BufferEntry& slot(Channel ch) { return e_[ch]; }
    static uint8_t  page_of(uint32_t a) { return (uint8_t)(a >> 16); }
    static uint16_t tag_of (uint32_t a) { return (uint16_t)((a >> 1) & 0xFFFF); }

    static constexpr int MISS_CYCLES = 6;

    Memory16& mem_;
    BufferEntry e_[4];
    bool error_ = false;
};

} // namespace lx