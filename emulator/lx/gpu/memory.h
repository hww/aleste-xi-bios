#pragma once
#include <cstdint>

namespace lx {

class Memory16 {
public:
    explicit Memory16(uint32_t size_bytes = 0x1000000);
    Memory16(uint8_t* base, uint32_t size) { attach(base, size); }
    ~Memory16();

    Memory16(const Memory16&) = delete;
    Memory16& operator=(const Memory16&) = delete;

    uint16_t read16(uint32_t addr24) const;
    void     write16(uint32_t addr24, uint16_t v);
    uint8_t  read8(uint32_t addr24) const;
    void     write8(uint32_t addr24, uint8_t v);

    void     attach(uint8_t* base, uint32_t size);
    uint8_t* raw() { return data_; }
    uint32_t size() const { return size_; }
    bool     owns() const { return owns_; }

    mutable uint64_t reads = 0, writes = 0;

private:
    uint8_t* data_ = nullptr;
    uint32_t size_ = 0;
    bool owns_ = false;
};

} // namespace lx