#include <cstdio>
#include <cstdint>
#include <vector>
#include "memory.h"
#include "gpu.h"
#include "gpu_mmio.h"
#include "addr_packer.h"

using namespace lx;

static MicroWord W(RegOp r, AluOp a, CtrlOp c,
                   uint8_t reg_addr=0, uint8_t mask=0, Cond cond=Z, uint16_t imm=0) {
    MicroWord w{};
    w.reg_op=r; w.alu_op=a; w.ctrl_op=c;
    w.reg_addr=reg_addr; w.mask=mask; w.cond=cond; w.imm=imm&0xFFF;
    return w;
}
static void emit_code(std::vector<uint16_t>& pkt, uint16_t addr,
                      const std::vector<MicroWord>& code) {
    pkt.push_back((2u<<12) | (uint16_t)code.size());
    pkt.push_back(addr);
    for (auto& w : code) {
        uint16_t raw[3]; w.pack(raw);
        pkt.push_back(raw[0]); pkt.push_back(raw[1]); pkt.push_back(raw[2]);
    }
}
static void emit_run(std::vector<uint16_t>& pkt, uint16_t entry) {
    pkt.push_back((3u<<12) | (entry & 0xFFF));
}
static void emit_end(std::vector<uint16_t>& pkt) { pkt.push_back(0x0000); }

static void a_imm(std::vector<MicroWord>& c, uint16_t v) {
    c.push_back(W(R_LDI, A_LOGIC_12, C_NOP, 0,0,Z, v));
}
static void reg_a(std::vector<MicroWord>& c, uint8_t idx) {
    c.push_back(W(R_STR, A_LOGIC_12, C_NOP, idx));
}
static void set_mode(std::vector<MicroWord>& c, uint8_t ch, uint8_t fmt) {
    a_imm(c, fmt);
    c.push_back(W(R_SETMODE, A_LOGIC_12, C_NOP, ch));
}

// ====== test_addr ======
static bool test_addr(bool trace) {
    (void)trace;
    std::printf("=== test_addr ===\n");
    uint32_t a; uint8_t p;
    bool ok = true;
    for (uint32_t x = 0; x < 8; ++x) {
        AddrPacker::compute(x, 0, 0x100, 2, FMT_4BPP, a, p);
        if (a != 0x100 + (x >> 2) * 2 || p != (x & 3)) { std::printf("  4bpp x=%u bad\n", x); ok = false; }
    }
    for (uint32_t x = 0; x < 4; ++x) {
        AddrPacker::compute(x, 0, 0x200, 2, FMT_8BPP, a, p);
        if (a != 0x200 + (x >> 1) * 2 || p != (x & 1)) { std::printf("  8bpp x=%u bad\n", x); ok = false; }
    }
    AddrPacker::compute(0, 1, 0x100, 2, FMT_4BPP, a, p);
    if (a != 0x102 || p != 0) { std::printf("  y=1 bad\n"); ok = false; }
    std::printf("  %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

// ====== test_fill ======
static bool test_fill(bool trace) {
    std::printf("=== test_fill (4 bpp) ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    a_imm(c, 0x100); reg_a(c, R_DBASE);
    a_imm(c, 2);     reg_a(c, R_DSTRIDE);
    set_mode(c, CH_DEST, FMT_4BPP);
    for (int i = 0; i < 8; ++i) {
        a_imm(c, (uint16_t)i);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
        a_imm(c, 0xA);
        c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    }
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(1000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x100), w1 = mem.read16(0x102);
    std::printf("  words: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0xAAAA && w1 == 0xAAAA);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== test_sprite ======
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
    a_imm(c, 0x200); reg_a(c, R_SBASE);
    a_imm(c, 2);     reg_a(c, R_SSTRIDE);
    a_imm(c, 0x300); reg_a(c, R_DBASE);
    a_imm(c, 2);     reg_a(c, R_DSTRIDE);
    set_mode(c, CH_SOURCE, FMT_4BPP);
    set_mode(c, CH_DEST, FMT_4BPP);
    for (int i = 0; i < 8; ++i) {
        a_imm(c, (uint16_t)i);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_SOURCE));
        c.push_back(W(R_PACKER_RD, A_LOGIC_12, C_NOP, CH_SOURCE));
        c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T0));
        a_imm(c, (uint16_t)i);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
        c.push_back(W(R_LDR, A_LOGIC_12, C_NOP, R_T0));
        c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    }
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
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

// ====== test_sprite8 ======
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
    a_imm(c, 0x200); reg_a(c, R_SBASE);
    a_imm(c, 2);     reg_a(c, R_SSTRIDE);
    a_imm(c, 0x300); reg_a(c, R_DBASE);
    a_imm(c, 2);     reg_a(c, R_DSTRIDE);
    set_mode(c, CH_SOURCE, FMT_8BPP);
    set_mode(c, CH_DEST, FMT_8BPP);
    for (int i = 0; i < 4; ++i) {
        a_imm(c, (uint16_t)i);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_SOURCE));
        c.push_back(W(R_PACKER_RD, A_LOGIC_12, C_NOP, CH_SOURCE));
        c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T0));
        a_imm(c, (uint16_t)i);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
        c.push_back(W(R_LDR, A_LOGIC_12, C_NOP, R_T0));
        c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    }
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
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

// ====== test_loop ======
static bool test_loop(bool trace) {
    std::printf("=== test_loop ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    a_imm(c, 0x100); reg_a(c, R_DBASE);
    a_imm(c, 2);     reg_a(c, R_DSTRIDE);
    set_mode(c, CH_DEST, FMT_4BPP);
    a_imm(c, 0); c.push_back(W(R_SETD, A_LOGIC_12, C_NOP));
    a_imm(c, 4); c.push_back(W(R_LDCNT, A_LOGIC_12, C_NOP));
    uint16_t pix = (uint16_t)c.size();
    c.push_back(W(R_LDX, A_LOGIC_12, C_NOP, DeviceMap::DIDX));
    c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
    a_imm(c, 0xA);
    c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    c.push_back(W(R_LDX, A_LOGIC_12, C_NOP, DeviceMap::DIDX));
    c.push_back(W(R_LDI_PUSH, A_LOGIC_12, C_NOP, 0,0,Z, 1));
    c.push_back(W(R_NOP, A_ADD, C_NOP));
    c.push_back(W(R_STX, A_LOGIC_12, C_NOP, DeviceMap::DIDX));
    c.push_back(W(R_NOP, A_LOGIC_12, C_LOOP, 0,0,Z, pix));
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x100), w1 = mem.read16(0x102);
    std::printf("  words: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0xAAAA && w1 == 0x0000);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== test_setpage ======
static bool test_setpage(bool trace) {
    std::printf("=== test_setpage ===\n");
    Memory16 mem(0x20000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    a_imm(c, 2); reg_a(c, R_DSTRIDE);
    a_imm(c, 0x100); reg_a(c, R_DBASE);
    set_mode(c, CH_DEST, FMT_4BPP);
    a_imm(c, 0);
    c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
    a_imm(c, 0xA);
    c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    // SETPAGE: A = 0x10000
    a_imm(c, 0x000);
    c.push_back(W(R_LDH, A_LOGIC_12, C_NOP, 0,0,Z, 0x010));
    c.push_back(W(R_SETPAGE, A_LOGIC_12, C_NOP, CH_DEST));
    // DBASE = 0x10100
    a_imm(c, 0x100);
    c.push_back(W(R_LDH, A_LOGIC_12, C_NOP, 0,0,Z, 0x010));
    c.push_back(W(R_LDL, A_LOGIC_12, C_NOP, 0,0,Z, 0x100));
    reg_a(c, R_DBASE);
    a_imm(c, 0);
    c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
    a_imm(c, 0xB);
    c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t p0 = mem.read16(0x00100), p1 = mem.read16(0x10100);
    std::printf("  page0[0x100]=%04X page1[0x10100]=%04X\n", p0, p1);
    bool pass = (p0 == 0xA000 && p1 == 0xB000 && !gpu.buffer_error());
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== test_page_error ======
static bool test_page_error(bool trace) {
    std::printf("=== test_page_error ===\n");
    Memory16 mem(0x20000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    a_imm(c, 2); reg_a(c, R_DSTRIDE);
    a_imm(c, 0x100); reg_a(c, R_DBASE);
    set_mode(c, CH_DEST, FMT_4BPP);
    a_imm(c, 0);
    c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
    a_imm(c, 0xA);
    c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    // Без SETPAGE, DBASE = 0x10100 (другая страница)
    a_imm(c, 0x100);
    c.push_back(W(R_LDH, A_LOGIC_12, C_NOP, 0,0,Z, 0x010));
    c.push_back(W(R_LDL, A_LOGIC_12, C_NOP, 0,0,Z, 0x100));
    reg_a(c, R_DBASE);
    a_imm(c, 0);
    c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
    a_imm(c, 0xB);
    c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    gpu.run_until_halt(2000000);
    uint16_t p1 = mem.read16(0x10100);
    bool pass = gpu.buffer_error() && (p1 == 0x0000);
    std::printf("  err=%d page1[0x10100]=%04X\n", (int)gpu.buffer_error(), p1);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== test_pass ======
static bool test_pass(bool trace) {
    std::printf("=== test_pass ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    mem.write16(0x200, 0xBEEF);
    std::vector<MicroWord> c;

    auto read_word = [&](uint16_t fmt, uint32_t dbase) {
        a_imm(c, 0x200); c.push_back(W(R_SETP, A_LOGIC_12, C_NOP));
        set_mode(c, CH_DATA, fmt);
        c.push_back(W(R_PACKER_RD, A_LOGIC_12, C_NOP, CH_DATA));
        c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T0));
        a_imm(c, dbase & 0xFFF);
        c.push_back(W(R_LDH, A_LOGIC_12, C_NOP, 0,0,Z, (dbase >> 12) & 0xFFF));
        c.push_back(W(R_LDL, A_LOGIC_12, C_NOP, 0,0,Z, dbase & 0xFFF));
        reg_a(c, R_DBASE);
        set_mode(c, CH_DEST, fmt);
        a_imm(c, 0);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
        c.push_back(W(R_LDR, A_LOGIC_12, C_NOP, R_T0));
        c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    };
    read_word(FMT_PASS16, 0x300);
    read_word(FMT_PASS_LO8, 0x302);
    read_word(FMT_PASS_HI8, 0x304);

    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
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

// ====== test_mask (заглушка) ======
static bool test_mask(bool trace) {
    (void)trace;
    std::printf("=== test_mask ===\n");
    std::printf("  SKIP — SETMASK убран, маска будет через SETMODE/регистр (TBD)\n");
    return true;
}

// ====== test_1bpp ======
static bool test_1bpp(bool trace) {
    std::printf("=== test_1bpp ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    a_imm(c, 2); reg_a(c, R_DSTRIDE);
    a_imm(c, 0x100); reg_a(c, R_DBASE);
    set_mode(c, CH_DEST, FMT_1BPP);
    for (int i = 0; i < 16; ++i) {
        a_imm(c, (uint16_t)i);
        c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
        a_imm(c, (i & 1) ? 0 : 1);
        c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    }
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(2000000)) { std::printf("  FAIL halt\n"); return false; }
    uint16_t w0 = mem.read16(0x100);
    std::printf("  word=%04X\n", w0);
    bool pass = (w0 == 0xAAAA);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== test_affine ======
static bool test_affine(bool trace) {
    std::printf("=== test_affine ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    mem.write16(0x200, 0x1200);
    mem.write16(0x202, 0x3400);
    for (int i = 0; i < 8; ++i) mem.write16(0x300 + i*2, 0xFFFF);
    std::vector<MicroWord> c;
    a_imm(c, 0x200); reg_a(c, R_SBASE);
    a_imm(c, 2);     reg_a(c, R_SSTRIDE);
    a_imm(c, 0x300); reg_a(c, R_DBASE);
    a_imm(c, 2);     reg_a(c, R_DSTRIDE);
    set_mode(c, CH_SOURCE, FMT_4BPP);
    set_mode(c, CH_DEST, FMT_4BPP);
    for (int dy = 0; dy < 4; ++dy) {
        for (int dx = 0; dx < 4; ++dx) {
            int sx_int = dx >> 1, sy_int = dy >> 1;
            // source: A = (sy<<12)|sx
            a_imm(c, (uint16_t)sx_int);
            c.push_back(W(R_LDH, A_LOGIC_12, C_NOP, 0,0,Z, (uint16_t)sy_int));
            c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_SOURCE));
            c.push_back(W(R_PACKER_RD, A_LOGIC_12, C_NOP, CH_SOURCE));
            c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T0));
            // dest: A = (dy<<12)|dx
            a_imm(c, (uint16_t)dx);
            c.push_back(W(R_LDH, A_LOGIC_12, C_NOP, 0,0,Z, (uint16_t)dy));
            c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
            c.push_back(W(R_LDR, A_LOGIC_12, C_NOP, R_T0));
            c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
        }
    }
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
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

// ====== test_comb_alu ======
static bool test_comb_alu(bool trace) {
    std::printf("=== test_comb_alu ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 512);
    gpu.set_trace(trace);
    std::vector<MicroWord> c;
    // AND: 0x0F0 & 0x00F = 0
    a_imm(c, 0x0F0);
    c.push_back(W(R_LDI_PUSH, A_LOGIC_12, C_NOP, 0,0,Z, 0x00F));
    c.push_back(W(R_NOP, A_LOGIC_8, C_NOP));
    c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T0));
    // OR: 0x0F0 | 0x00F = 0x0FF
    a_imm(c, 0x0F0);
    c.push_back(W(R_LDI_PUSH, A_LOGIC_12, C_NOP, 0,0,Z, 0x00F));
    c.push_back(W(R_NOP, A_LOGIC_14, C_NOP));
    c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T1));
    // XOR: 0x0F0 ^ 0x00F = 0x0FF
    a_imm(c, 0x0F0);
    c.push_back(W(R_LDI_PUSH, A_LOGIC_12, C_NOP, 0,0,Z, 0x00F));
    c.push_back(W(R_NOP, A_LOGIC_6, C_NOP));
    c.push_back(W(R_STR, A_LOGIC_12, C_NOP, R_T2));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }
    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }
    uint32_t v0 = gpu.reg(R_T0), v1 = gpu.reg(R_T1), v2 = gpu.reg(R_T2);
    std::printf("  AND=%06X OR=%06X XOR=%06X\n", v0, v1, v2);
    bool pass = (v0 == 0x000) && (v1 == 0x0FF) && (v2 == 0x0FF);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== test_mmio ======
static bool test_mmio(bool trace) {
    std::printf("=== test_mmio ===\n");
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);
    GpuMmio mmio(gpu);
    std::vector<MicroWord> c;
    a_imm(c, 2); reg_a(c, R_DSTRIDE);
    a_imm(c, 0x100); reg_a(c, R_DBASE);
    set_mode(c, CH_DEST, FMT_4BPP);
    a_imm(c, 0);
    c.push_back(W(R_ADDR, A_LOGIC_12, C_NOP, CH_DEST));
    a_imm(c, 0xA);
    c.push_back(W(R_PACKER_WR, A_LOGIC_12, C_NOP, CH_DEST));
    c.push_back(W(R_FLUSH, A_LOGIC_12, C_NOP));
    c.push_back(W(R_NOP, A_LOGIC_12, C_HALT, 0,0,Z, 0));
    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    for (uint16_t w : pkt) { mmio.write(0x08, w & 0xFF); mmio.write(0x08, (w >> 8) & 0xFF); }
    mmio.write(0x0A, 1);
    if (mmio.read(0x0C)) { std::printf("  FAIL packet load\n"); return false; }
    mmio.write(0x01, 0); mmio.write(0x02, 0); mmio.write(0x03, 0);
    mmio.write(0x00, 1);
    for (int i = 0; i < 500000; ++i) { if (mmio.read(0x04) & 2) break; mmio.tick(1); }
    uint8_t st = mmio.read(0x04);
    uint16_t w = mem.read16(0x100);
    std::printf("  status=%02X word=%04X\n", st, w);
    bool pass = (st & 2) && (w == 0xA000);
    std::printf("  %s\n", pass ? "PASS" : "FAIL");
    return pass;
}

// ====== runner ======
static bool run_test(const char* name, bool (*fn)(bool trace)) {
    std::printf("\n");
    bool ok = fn(false);
    if (!ok) {
        std::printf("  --- retry with trace ---\n");
        fn(true);
    }
    (void)name;
    return ok;
}

int main() {
    std::printf("lx-gpu sim (stack A/B/C), mem=64KiB\n");
    bool ok = true;
    ok = run_test("test_addr", test_addr) && ok;
    ok = run_test("test_fill", test_fill) && ok;
    ok = run_test("test_sprite", test_sprite) && ok;
    ok = run_test("test_sprite8", test_sprite8) && ok;
    ok = run_test("test_loop", test_loop) && ok;
    ok = run_test("test_setpage", test_setpage) && ok;
    ok = run_test("test_page_error", test_page_error) && ok;
    ok = run_test("test_pass", test_pass) && ok;
    ok = run_test("test_mask", test_mask) && ok;
    ok = run_test("test_1bpp", test_1bpp) && ok;
    ok = run_test("test_affine", test_affine) && ok;
    ok = run_test("test_comb_alu", test_comb_alu) && ok;
    ok = run_test("test_mmio", test_mmio) && ok;
    std::printf("\n%s\n", ok ? "ALL PASS" : "SOME FAIL");
    return ok ? 0 : 1;
}