#include "pixel_buffer.h"

namespace lx {

void PixelBuffer::reset() {
    for (auto& x : e_) x = BufferEntry{};
    error_ = false;
}

int PixelBuffer::set_page(Channel ch, uint32_t page_base24) {
    BufferEntry& s = slot(ch);
    int t = 0;
    if (s.valid && s.dirty) {
        uint32_t addr = ((uint32_t)s.page << 16) | s.tag;
        mem_.write16(addr, s.data);
        ++writebacks;
        t += MISS_CYCLES;
    }
    s = BufferEntry{};
    s.page = page_of(page_base24);
    return t ? t : 1;
}

int PixelBuffer::read(Channel ch, uint32_t addr24, uint16_t& out) {
    BufferEntry& s = slot(ch);
    uint8_t  pg = page_of(addr24);
    uint16_t tg = tag_of(addr24);
    if (s.page != pg && (s.valid || s.dirty)) {
        error_ = true; return 1;
    }
    if (s.valid && s.tag == tg && s.page == pg) {
        out = s.data; ++hits; return 1;
    }
    int t = 1;
    if (s.valid && s.dirty) {
        uint32_t a = ((uint32_t)s.page << 16) | s.tag;
        mem_.write16(a, s.data);
        ++writebacks;
        t += MISS_CYCLES;
    }
    s.page = pg; s.tag = tg;
    s.data = mem_.read16(addr24);
    s.valid = true; s.dirty = false;
    out = s.data;
    ++misses;
    return t + MISS_CYCLES;
}

int PixelBuffer::write(Channel ch, uint32_t addr24, uint16_t data) {
    BufferEntry& s = slot(ch);
    uint8_t  pg = page_of(addr24);
    uint16_t tg = tag_of(addr24);
    if (s.page != pg && (s.valid || s.dirty)) {
        error_ = true; return 1;
    }
    if (!(s.valid && s.tag == tg && s.page == pg)) {
        int t = 1;
        if (s.valid && s.dirty) {
            uint32_t a = ((uint32_t)s.page << 16) | s.tag;
            mem_.write16(a, s.data);
            ++writebacks;
            t += MISS_CYCLES;
        }
        s.page = pg; s.tag = tg;
        s.data = data; s.valid = true; s.dirty = true;
        ++misses;
        return t + MISS_CYCLES;
    }
    s.data = data; s.dirty = true;
    return 1;
}

int PixelBuffer::flush() {
    int t = 0;
    for (auto& s : e_) {
        if (s.valid && s.dirty) {
            uint32_t a = ((uint32_t)s.page << 16) | s.tag;
            mem_.write16(a, s.data);
            ++writebacks;
            t += MISS_CYCLES;
        }
        s = BufferEntry{};
    }
    return t ? t : 1;
}

} // namespace lx