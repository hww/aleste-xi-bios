#include "memory.h"
#include <cstring>
#include <algorithm>

namespace lx {

Memory16::Memory16(uint32_t size_bytes)
    : size_(std::min<uint32_t>(size_bytes, 0x1000000)) {
    data_ = new uint8_t[size_];
    std::memset(data_, 0, size_);
    owns_ = true;
}

Memory16::~Memory16() {
    if (owns_ && data_) delete[] data_;
}

void Memory16::attach(uint8_t* base, uint32_t size) {
    if (owns_ && data_) delete[] data_;
    data_ = base;
    size_ = std::min<uint32_t>(size, 0x1000000);
    owns_ = false;
}

uint16_t Memory16::read16(uint32_t a) const {
    a &= 0xFFFFFF;
    ++reads;
    if (a + 1 >= size_) return 0xFFFF;
    return (uint16_t)(data_[a] | (data_[a + 1] << 8));
}
void Memory16::write16(uint32_t a, uint16_t v) {
    a &= 0xFFFFFF;
    ++writes;
    if (a + 1 >= size_) return;
    data_[a] = v & 0xFF;
    data_[a + 1] = v >> 8;
}
uint8_t Memory16::read8(uint32_t a) const {
    a &= 0xFFFFFF;
    ++reads;
    return a < size_ ? data_[a] : 0xFF;
}
void Memory16::write8(uint32_t a, uint8_t v) {
    a &= 0xFFFFFF;
    ++writes;
    if (a < size_) data_[a] = v;
}

} // namespace lx