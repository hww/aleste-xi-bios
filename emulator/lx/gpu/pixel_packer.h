#pragma once
#include <cstdint>
#include "pixel_buffer.h"
#include "addr_packer.h"

namespace lx {

class PixelPacker {
public:
    explicit PixelPacker(PixelBuffer& buf) : buf_(buf) {}

    int read_pixel (Channel ch, uint32_t addr24, PixFmt fmt, uint8_t pos, uint16_t& out);
    int write_pixel(Channel ch, uint32_t addr24, PixFmt fmt, uint8_t pos, uint16_t pixel,
                    uint16_t mask = 0xFFFF);

private:
    PixelBuffer& buf_;
};

} // namespace lx