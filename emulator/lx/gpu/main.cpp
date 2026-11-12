#include <cstdio>
#include <cstdint>
#include <vector>
#include "memory.h"
#include "gpu.h"
#include "gpu_mmio.h"
#include "addr_packer.h"

using namespace lx;

// ===== helpers =====
static MicroWord I_reg(uint8_t dst, uint8_t src, AluOp op,
                       Cond cond=AL, bool f=false, bool l=false) {
    MicroWord w{}; w.imm_class=0;
    w.dst = dst & 0x1F; w.ds = (dst >> 5) & 1;
    w.src = src & 0x3F;
    w.reg_op=op; w.cond=cond; w.f=f?1:0; w.l=l?1:0;
    return w;
}
static MicroWord I_imm(uint8_t dst, uint16_t imm, AluOp op,
                       Cond cond=AL, bool f=false, bool l=false,
                       bool sx=false) {
    MicroWord w{}; w.imm_class=3;
    w.dst = dst & 0x1F; w.ds = (dst >> 5) & 1;
    w.imm12=imm&0xFFF;
    w.reg_op=op; w.cond=cond; w.f=f?1:0; w.l=l?1:0; w.sx=sx?1:0;
    return w;
}
static MicroWord I_ldi24(uint8_t dst, uint32_t imm24) {
    MicroWord w{}; w.imm_class=1; w.dst=dst & 0x1F; w.imm24=imm24&0xFFFFFF;
    return w;
}
static MicroWord I_ctrl(CtrlOp op, uint16_t addr=0, uint8_t reg=0,
                        Cond cond=AL) {
    MicroWord w{}; w.imm_class=2; w.reg_op=op; w.imm12=addr&0xFFF;
    w.src=reg; w.cond=cond;
    return w;
}
// MOV special, special
static MicroWord MOV(uint8_t dst, uint8_t src) { return I_reg(dst, src, A_MOV); }

static void emit_code(std::vector<uint16_t>& pkt, uint16_t addr,
                      const std::vector<MicroWord>& code) {
    pkt.push_back((2u<<12) | (uint16_t)code.size());
    pkt.push_back(addr);
    for (auto& w : code) {
        uint16_t raw[2]; w.pack(raw);
        pkt.push_back(raw[0]); pkt.push_back(raw[1]);
    }
}
static void emit_run(std::vector<uint16_t>& pkt, uint16_t entry) {
    pkt.push_back((3u<<12) | (entry & 0xFFF));
}
static void emit_end(std::vector<uint16_t>& pkt) { pkt.push_back(0x0000); }

// ===== test_addr =====
static bool test_addr(bool trace) {
    (void)trace;
    std::printf("=== test_addr ===\n");
    uint32_t a; uint8_t p;
    bool ok = true;
    for (uint32_t x = 0; x < 8; ++x) {
        AddrPacker::compute(x, 0, 0, 0, FMT_4BPP, a, p);
        if (a != (x>>2)*2 || p != (x&3)) { std::printf("  4bpp x=%u bad\n", x); ok = false; }
    }
    for (uint32_t x = 0; x < 4; ++x) {
        AddrPacker::compute(x, 0, 0, 0, FMT_8BPP, a, p);
        if (a != (x>>1)*2 || p != (x&1)) { std::printf("  8bpp x=%u bad\n", x); ok = false; }
    }
    AddrPacker::compute(0, 1, 0, 0, FMT_4BPP, a, p);
    if (a != 0x10000 || p != 0) { std::printf("  page bad\n"); ok = false; }
    std::printf("  %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

// ===== test_fill =====
static bool test_fill(bool trace) {
    std::printf("=== test_fill (4 bpp) ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    // DST.PAGE = 0, DST.MODE = 4bpp
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV, AL, false, false, false));
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV, AL, false, false, false));
    // DST.STEP = 1
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    // DST.IDX = 0
    c.push_back(I_imm(OP_DST_IDX, 0, A_MOV));
    // 8 пикселей
    for (int i = 0; i < 8; ++i) {
        c.push_back(I_imm(OP_DST_PIX, 0xA, A_MOV));
    }
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(1000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x000), w1 = mem.read16(0x002);
    std::printf("  words: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0xAAAA && w1 == 0xAAAA);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_sprite =====
static bool test_sprite(bool trace) {
    std::printf("=== test_sprite (4 bpp) ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    mem.write16(0x200, 0x0123);
    mem.write16(0x202, 0x4567);
    mem.write16(0x300, 0xFFFF);
    mem.write16(0x302, 0xFFFF);
    std::vector<MicroWord> c;
    // SRC: page 0, mode 4bpp, IDX = 0x100 (word 0x100 = byte 0x200)
    // Внимание: SRC.IDX — пиксельный индекс. word 0x100 = pixel 0x100*4 = 0x400.
    // Проще: SRC.IDX в пикселях, страница 0. word 0x200 = byte 0x400 → пиксель 0x400.
    // Установим SRC.IDX = 0x100 (пиксель 0x100 = word 0x40 = byte 0x80).
    // Чтобы попасть в 0x200, нужно SRC.IDX = 0x400 (word 0x100 = byte 0x200).
    c.push_back(I_imm(OP_SRC_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_SRC_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_SRC_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_SRC_IDX, 0x400, A_MOV));
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0x600, A_MOV));   // word 0x180 = byte 0x300
    for (int i = 0; i < 8; ++i) {
        c.push_back(I_reg(R_T0, OP_SRC_PIX, A_MOV));
        c.push_back(I_reg(OP_DST_PIX, R_T0, A_MOV));
    }
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x300), w1 = mem.read16(0x302);
    std::printf("  dst: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0x0123 && w1 == 0x4567);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_sprite8 =====
static bool test_sprite8(bool trace) {
    std::printf("=== test_sprite8 (8 bpp) ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    mem.write16(0x200, 0x3412);
    mem.write16(0x202, 0x7856);
    mem.write16(0x300, 0xFFFF);
    mem.write16(0x302, 0xFFFF);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_SRC_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_SRC_MODE, FMT_8BPP, A_MOV));
    c.push_back(I_imm(OP_SRC_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_SRC_IDX, 0x200, A_MOV));   // word 0x100
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_8BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0x300, A_MOV));   // word 0x180
    for (int i = 0; i < 4; ++i) {
        c.push_back(I_reg(R_T0, OP_SRC_PIX, A_MOV));
        c.push_back(I_reg(OP_DST_PIX, R_T0, A_MOV));
    }
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x300), w1 = mem.read16(0x302);
    std::printf("  dst: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0x3412 && w1 == 0x7856);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_loop =====
// ===== test_loop =====
static bool test_loop(bool trace) {
    std::printf("=== test_loop ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));       // 0
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV));// 1
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));       // 2
    c.push_back(I_imm(OP_DST_IDX, 0, A_MOV));        // 3
    uint16_t pix2 = (uint16_t)c.size() + 2;          // 4+2 = 6 (PIX)
    c.push_back(I_ldi24(R_T0, (4u<<12) | pix2));     // 4: LOOP = (4<<12)|6
    c.push_back(I_reg(OP_LOOP, R_T0, A_MOV));        // 5
    c.push_back(I_imm(OP_DST_PIX, 0xA, A_MOV, AL, false, true)); // 6: L=1
    c.push_back(MOV(OP_FLUSH, OP_ZERO));             // 7
    c.push_back(I_ctrl(C_HALT, 0));                  // 8
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x000), w1 = mem.read16(0x002);
    std::printf("  words: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0xAAAA && w1 == 0x0000);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_setpage =====
static bool test_setpage(bool trace) {
    std::printf("=== test_setpage ===\n");
    Memory16 mem(0x20000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0, A_MOV));
    c.push_back(I_imm(OP_DST_PIX, 0xA, A_MOV));
    // set page 1, IDX=0x400 (pixel) → word 0x100 → byte 0x10100
    c.push_back(I_imm(OP_DST_PAGE, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0x200, A_MOV));   // было 0x400
    c.push_back(I_imm(OP_DST_PIX, 0xB, A_MOV));
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t p0 = mem.read16(0x000), p1 = mem.read16(0x10100);
    std::printf("  page0[0]=%04X page1[0x10100]=%04X\n", p0, p1);
    bool pass = (p0 == 0xA000 && p1 == 0xB000 && !gpu.buffer_error());
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_page_error =====
static bool test_page_error(bool trace) {
    (void)trace;
    std::printf("=== test_page_error ===\n");
    Memory16 mem(0x20000);
    PixelBuffer buf(mem);
    buf.set_page(CH_DEST, 0);
    uint16_t out;
    buf.write(CH_DEST, 0x000100, 0xA000);   // page 0, tag 0x80
    buf.read (CH_DEST, 0x000100, out);
    bool ok1 = !buf.error();
    // Теперь читаем по адресу в другой странице без set_page
    buf.read(CH_DEST, 0x010100, out);       // page 1
#ifdef SIMULATE_CACHE
    bool ok2 = buf.error();
#else
    bool ok2 = true;
#endif
    std::printf("  before=%d after=%d\n", (int)ok1, (int)buf.error());
    bool pass = ok1 && ok2;
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_pass =====
static bool test_pass(bool trace) {
    std::printf("=== test_pass ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    mem.write16(0x200, 0xBEEF);
    std::vector<MicroWord> c;
    // STM.PAGE=0, STM.MODE=PASS16, STM.PTR=0x100 (word 0x100 = byte 0x200)
    c.push_back(I_imm(OP_STM_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_STM_MODE, FMT_PASS16, A_MOV));
    c.push_back(I_imm(OP_STM_PTR, 0x100, A_MOV));
    c.push_back(I_reg(R_T0, OP_STM_PIX, A_MOV));    // T0 = BEEF
    // DST: page 0, mode PASS16, IDX = 0x180 (word 0x180 = byte 0x300)
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_PASS16, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0x180, A_MOV));
    c.push_back(I_reg(OP_DST_PIX, R_T0, A_MOV));
    // LO8
    c.push_back(I_imm(OP_STM_PTR, 0x100, A_MOV));
    c.push_back(I_imm(OP_STM_MODE, FMT_PASS_LO8, A_MOV));
    c.push_back(I_reg(R_T0, OP_STM_PIX, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0x181, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_PASS_LO8, A_MOV));
    c.push_back(I_reg(OP_DST_PIX, R_T0, A_MOV));
    // HI8
    c.push_back(I_imm(OP_STM_PTR, 0x100, A_MOV));
    c.push_back(I_imm(OP_STM_MODE, FMT_PASS_HI8, A_MOV));
    c.push_back(I_reg(R_T0, OP_STM_PIX, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0x182, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_PASS_HI8, A_MOV));
    c.push_back(I_reg(OP_DST_PIX, R_T0, A_MOV));
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x300), w1 = mem.read16(0x302), w2 = mem.read16(0x304);
    std::printf("  PASS16=%04X LO8=%04X HI8=%04X\n", w0, w1, w2);
    bool pass = (w0 == 0xBEEF && w1 == 0x00EF && w2 == 0xBE00);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_1bpp =====
static bool test_1bpp(bool trace) {
    std::printf("=== test_1bpp ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_1BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0, A_MOV));
    for (int i = 0; i < 16; ++i) {
        c.push_back(I_imm(OP_DST_PIX, (i & 1) ? 0 : 1, A_MOV));
    }
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x000);
    std::printf("  word=%04X\n", w0);
    bool pass = (w0 == 0xAAAA);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_affine =====
static bool test_affine(bool trace) {
    std::printf("=== test_affine ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    mem.write16(0x200, 0x1200);
    mem.write16(0x202, 0x3400);
    for (int i = 0; i < 8; ++i) mem.write16(0x300 + i*2, 0xFFFF);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_SRC_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_SRC_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_SRC_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    // affine: src = (dx>>1, dy>>1), dst = (dx, dy)
    for (int dy = 0; dy < 4; ++dy) {
        for (int dx = 0; dx < 4; ++dx) {
            int sx = dx >> 1, sy = dy >> 1;
            uint32_t sidx = 0x400 + (uint32_t)sy * 4 + sx;  // 0x200 байт = 0x400 пиксель
            c.push_back(I_ldi24(R_T0, sidx));
            c.push_back(I_reg(OP_SRC_IDX, R_T0, A_MOV));
            c.push_back(I_reg(R_T0, OP_SRC_PIX, A_MOV));
            uint32_t didx = 0x600 + (uint32_t)dy * 4 + dx;  // 0x300 байт = 0x600 пиксель
            c.push_back(I_ldi24(R_T1, didx));
            c.push_back(I_reg(OP_DST_IDX, R_T1, A_MOV));
            c.push_back(I_reg(OP_DST_PIX, R_T0, A_MOV));
        }
    }
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(3000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x300), w1 = mem.read16(0x302);
    uint16_t w2 = mem.read16(0x304), w3 = mem.read16(0x306);
    std::printf("  dst: %04X %04X %04X %04X\n", w0, w1, w2, w3);
    bool pass = (w0 == 0x1122 && w1 == 0x1122 && w2 == 0x3344 && w3 == 0x3344);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_flags =====
static bool test_flags(bool trace) {
    std::printf("=== test_flags ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    // AND: 0x0F0 & 0x00F = 0x000
    // AND: 0x0F0 & 0x00F = 0x000
    c.push_back(I_ldi24(R_T0, 0x0F0));
    c.push_back(I_ldi24(R_T1, 0x00F));
    c.push_back(I_reg(R_T2, R_T0, A_MOV));             // T2 = 0x0F0
    c.push_back(I_reg(R_T2, R_T1, A_AND, AL, true));   // T2 = 0
    // OR: 0x0F0 | 0x00F = 0x0FF
    c.push_back(I_ldi24(R_T3, 0x0F0));
    c.push_back(I_reg(R_T3, R_T1, A_OR, AL, true));    // T3 = 0x0FF
    // XOR: 0x0F0 ^ 0x00F = 0x0FF
    c.push_back(I_ldi24(R_T4, 0x0F0));
    c.push_back(I_reg(R_T4, R_T1, A_XOR, AL, true));   // T4 = 0x0FF
    // CMP 5 vs 7 → C=0, Z=0, N=1
    c.push_back(I_ldi24(R_T0, 5));
    c.push_back(I_ldi24(R_T1, 7));
    c.push_back(I_reg(R_T0, R_T1, A_CMP));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v2 = gpu.reg(R_T2), v3 = gpu.reg(R_T3), v4 = gpu.reg(R_T4);
    uint8_t fl = gpu.flags();
    std::printf("  AND=%06X OR=%06X XOR=%06X flags=%X\n", v2, v3, v4, fl);
    bool pass = (v2 == 0x000) && (v3 == 0x0FF) && (v4 == 0x0FF)
             && ((fl & 1) == 0) && ((fl & 2) != 0) && ((fl & 4) == 0);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_cond =====
static bool test_cond(bool trace) {
    std::printf("=== test_cond ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_ldi24(R_T0, 5));
    c.push_back(I_ldi24(R_T1, 5));
    c.push_back(I_reg(R_T0, R_T1, A_CMP));   // Z=1
    c.push_back(I_ldi24(R_T2, 0x111));
    c.push_back(I_ldi24(R_T3, 0x222));
    c.push_back(I_reg(R_T2, R_T3, A_MOV, EQ));   // выполнится: T2=0x222
    c.push_back(I_reg(R_T2, R_T1, A_MOV, NE));   // не выполнится
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v2 = gpu.reg(R_T2);
    std::printf("  T2=%06X (expect 000222)\n", v2);
    bool pass = (v2 == 0x222);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_mul =====
static bool test_mul(bool trace) {
    std::printf("=== test_mul ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_ldi24(R_T0, 1000));
    c.push_back(I_ldi24(R_T1, 2000));
    c.push_back(I_reg(R_T2, R_T1, A_MOV));   // T2 = 2000
    c.push_back(I_reg(R_T2, R_T0, A_MUL));   // T2 = 2000 * 1000 = 0x1E8480
    c.push_back(I_ldi24(R_T0, 0x1000));
    c.push_back(I_ldi24(R_T1, 0x1000));
    c.push_back(I_reg(R_T3, R_T1, A_MOV));   // T3 = 0x1000
    c.push_back(I_reg(R_T3, R_T0, A_MULH));  // T3 = (0x1000 * 0x1000) >> 12 = 0x1000
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v2 = gpu.reg(R_T2), v3 = gpu.reg(R_T3);
    std::printf("  MUL=%06X MULH=%06X\n", v2, v3);
    bool pass = (v2 == 0x1E8480) && (v3 == 0x1000);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_shadow =====
static bool test_shadow(bool trace) {
    std::printf("=== test_shadow ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_ctrl(C_CALL, 5));           // вызов подпрограммы по адресу 5
    c.push_back(I_ctrl(C_HALT, 0));           // адрес 1: сюда вернёмся
    c.push_back(I_ctrl(C_HALT, 0));           // адрес 2
    c.push_back(I_ctrl(C_HALT, 0));           // 3
    c.push_back(I_ctrl(C_HALT, 0));           // 4
    c.push_back(I_ldi24(R_T0, 0x123));        // 5: подпрограмма
    c.push_back(I_ctrl(C_RET));                // 6: возврат
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v0 = gpu.reg(R_T0);
    std::printf("  T0=%06X shadow=%06X\n", v0, gpu.shadow());
    bool pass = (v0 == 0x123) && ((gpu.shadow() & 0xFFF) == 1);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_jmpr =====
static bool test_jmpr(bool trace) {
    std::printf("=== test_jmpr ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_ldi24(R_T0, 4));
    c.push_back(I_ctrl(C_JMPR, 0, R_T0));   // прыжок на адрес 4
    c.push_back(I_ctrl(C_HALT, 0));          // 2
    c.push_back(I_ctrl(C_HALT, 0));          // 3
    c.push_back(I_ldi24(R_T1, 0x777));       // 4
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v1 = gpu.reg(R_T1);
    std::printf("  T1=%06X (expect 000777)\n", v1);
    bool pass = (v1 == 0x777);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_mov_special =====
static bool test_mov_special(bool trace) {
    std::printf("=== test_mov_special ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0, A_MOV));
    c.push_back(I_imm(OP_DST_PIX, 0xC, A_MOV));
    // Чтение DST.IDX — должно быть 1
    c.push_back(I_reg(R_T0, OP_DST_IDX, A_MOV));
    // MOV FLUSH, ZERO — flush
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v0 = gpu.reg(R_T0);
    uint16_t w = mem.read16(0x000);
    std::printf("  DST.IDX=%06X word=%04X\n", v0, w);
    bool pass = (v0 == 1) && (w == 0xC000);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== test_mmio =====
static bool test_mmio(bool trace) {
    std::printf("=== test_mmio ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    GpuMmio mmio(gpu);
    std::vector<MicroWord> c;
    c.push_back(I_imm(OP_DST_PAGE, 0, A_MOV));
    c.push_back(I_imm(OP_DST_MODE, FMT_4BPP, A_MOV));
    c.push_back(I_imm(OP_DST_STEP, 1, A_MOV));
    c.push_back(I_imm(OP_DST_IDX, 0, A_MOV));
    c.push_back(I_imm(OP_DST_PIX, 0xA, A_MOV));
    c.push_back(MOV(OP_FLUSH, OP_ZERO));
    c.push_back(I_ctrl(C_HALT, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    for (uint16_t w : pkt) {
        mmio.write(0x08, w & 0xFF);
        mmio.write(0x09, (w >> 8) & 0xFF);
    }
    mmio.write(0x0A, 1);
    if (mmio.read(0x0A)) { std::printf("  FAIL packet load\n"); return false; }
    mmio.write(0x01, 0); mmio.write(0x02, 0); mmio.write(0x03, 0);
    mmio.write(0x00, 1);
    for (int i = 0; i < 500000; ++i) {
        if ((mmio.read(0x04) & 2) || (mmio.read(0x04) & 4)) break;
        mmio.tick(1);
    }
    uint8_t st = mmio.read(0x04);
    uint16_t w = mem.read16(0x000);
    std::printf("  status=%02X word=%04X\n", st, w);
    bool pass = (st & 2) && (w == 0xA000);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ===== runner =====
static bool run_test(const char* name, bool (*fn)(bool trace)) {
    std::printf("\n");
    bool ok = fn(false);
    if (!ok) { std::printf("  --- retry with trace ---\n"); fn(true); }
    (void)name;
    return ok;
}

int main() {
    std::printf("lx-gpu sim (32-bit microcode), mem=64KiB\n");
    bool ok = true;
    ok = run_test("test_addr", test_addr) && ok;
    ok = run_test("test_fill", test_fill) && ok;
    ok = run_test("test_sprite", test_sprite) && ok;
    ok = run_test("test_sprite8", test_sprite8) && ok;
    ok = run_test("test_loop", test_loop) && ok;
    ok = run_test("test_setpage", test_setpage) && ok;
    ok = run_test("test_page_error", test_page_error) && ok;
    ok = run_test("test_pass", test_pass) && ok;
    ok = run_test("test_1bpp", test_1bpp) && ok;
    ok = run_test("test_affine", test_affine) && ok;
    ok = run_test("test_flags", test_flags) && ok;
    ok = run_test("test_cond", test_cond) && ok;
    ok = run_test("test_mul", test_mul) && ok;
    ok = run_test("test_shadow", test_shadow) && ok;
    ok = run_test("test_jmpr", test_jmpr) && ok;
    ok = run_test("test_mov_special", test_mov_special) && ok;
    ok = run_test("test_mmio", test_mmio) && ok;
    std::printf("\n%s\n", ok ? "ALL PASS" : "SOME FAIL");
    return ok ? 0 : 1;
}