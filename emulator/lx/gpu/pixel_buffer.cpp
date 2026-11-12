#include "pixel_buffer.h"

namespace lx {

void PixelBuffer::reset() {
    for (auto& e : e_) e = BufferEntry{};
    error_ = false;
    hits = misses = writebacks = 0;
}

int PixelBuffer::set_page(Channel ch, uint8_t page) {
    BufferEntry& e = e_[ch];
    int t = 0;
    if (e.valid && e.dirty) {
        uint32_t a = ((uint32_t)e.page << 16) | ((uint32_t)e.tag << 1);
        mem_.write16(a, e.data);
        ++writebacks; t += MISS_CYCLES;
    }
    e = BufferEntry{}; e.page = page;
    return t;
}

int PixelBuffer::read(Channel ch, uint32_t addr24, uint16_t& out) {
    BufferEntry& e = e_[ch];
    uint8_t pg = (uint8_t)(addr24 >> 16);
    uint16_t tg = (uint16_t)((addr24 >> 1) & 0xFFFF);
#ifdef SIMULATE_CACHE
    if (e.valid && e.page != pg) { error_ = true; }
#endif
    if (e.valid && e.tag == tg && e.page == pg) {
        out = e.data; ++hits; return 1;
    }
    if (e.valid && e.dirty) {
        uint32_t a = ((uint32_t)e.page << 16) | ((uint32_t)e.tag << 1);
        mem_.write16(a, e.data); ++writebacks;
    }
    e.data = mem_.read16(addr24);
    e.tag = tg; e.page = pg; e.valid = true; e.dirty = false;
    ++misses; out = e.data;
    return 1 + MISS_CYCLES;
}

int PixelBuffer::write(Channel ch, uint32_t addr24, uint16_t data) {
    BufferEntry& e = e_[ch];
    uint8_t pg = (uint8_t)(addr24 >> 16);
    uint16_t tg = (uint16_t)((addr24 >> 1) & 0xFFFF);
#ifdef SIMULATE_CACHE
    if (e.valid && e.page != pg) { error_ = true; }
#endif
    int t = 0;
    if (!(e.valid && e.tag == tg && e.page == pg)) {
        if (e.valid && e.dirty) {
            uint32_t a = ((uint32_t)e.page << 16) | ((uint32_t)e.tag << 1);
            mem_.write16(a, e.data); ++writebacks;
        }
        e.data = mem_.read16(addr24);
        e.tag = tg; e.page = pg; e.valid = true; e.dirty = false;
        ++misses; t += MISS_CYCLES;
    } else { ++hits; }
    e.data = data; e.dirty = true;
    return 1 + t;
}

int PixelBuffer::flush() {
    int t = 0;
    for (auto& e : e_) {
        if (e.valid && e.dirty) {
            uint32_t a = ((uint32_t)e.page << 16) | ((uint32_t)e.tag << 1);
            mem_.write16(a, e.data); ++writebacks; t += MISS_CYCLES;
        }
        e = BufferEntry{};
    }
    return t;
}

} // namespace lx