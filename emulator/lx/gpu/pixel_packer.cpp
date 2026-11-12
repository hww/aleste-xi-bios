#include "pixel_packer.h"

namespace lx {

static uint16_t do_unpack(uint16_t word, PixFmt fmt, uint8_t pos) {
    switch (fmt) {
    case FMT_1BPP: { unsigned s = 15 - (pos & 15); return (word >> s) & 1; }
    case FMT_PASS16: return word;
    case FMT_4BPP:   { unsigned s = (3u - (pos & 3)) * 4; return (word >> s) & 0x0F; }
    case FMT_8BPP:   { unsigned s = (1u - (pos & 1)) * 8; return (word >> s) & 0xFF; }
    case FMT_16BPP:  return word;
    case FMT_PASS_LO8: return word & 0xFF;
    case FMT_PASS_HI8: return (word >> 8) & 0xFF;
    case FMT_RSV:      break;      // reserved format: reads as 0
    }
    return 0;
}

static uint16_t do_pack(uint16_t word, uint16_t pixel, PixFmt fmt, uint8_t pos) {
    switch (fmt) {
    case FMT_1BPP: {
        unsigned s = 15 - (pos & 15);
        uint16_t m = (uint16_t)(1u << s);
        return (word & ~m) | ((pixel & 1) << s);
    }
    case FMT_PASS16: return pixel;
    case FMT_4BPP: {
        unsigned s = (3u - (pos & 3)) * 4; uint16_t m = (uint16_t)(0x0F << s);
        return (word & ~m) | ((pixel & 0x0F) << s);
    }
    case FMT_8BPP: {
        unsigned s = (1u - (pos & 1)) * 8; uint16_t m = (uint16_t)(0xFF << s);
        return (word & ~m) | ((pixel & 0xFF) << s);
    }
    case FMT_16BPP: return pixel;
    case FMT_PASS_LO8: return (word & 0xFF00) | (pixel & 0x00FF);
    case FMT_PASS_HI8: return (word & 0x00FF) | ((pixel & 0x00FF) << 8);
    case FMT_RSV:      break;      // reserved format: the word is left unchanged
    }
    return word;
}

int PixelPacker::read_pixel(Channel ch, uint32_t addr24, PixFmt fmt, uint8_t pos, uint16_t& out) {
    uint16_t word = 0;
    int t = buf_.read(ch, addr24, word);
    out = do_unpack(word, fmt, pos);
    return t;
}

int PixelPacker::write_pixel(Channel ch, uint32_t addr24, PixFmt fmt, uint8_t pos, uint16_t pixel) {
    uint16_t word = 0;
    int t = buf_.read(ch, addr24, word);
    word = do_pack(word, pixel, fmt, pos);
    int t2 = buf_.write(ch, addr24, word);
    return t + t2;
}

} // namespace lx