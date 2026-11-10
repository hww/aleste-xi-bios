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

static void acc_from_imm(std::vector<MicroWord>& c, uint16_t v) {
    c.push_back(W(R_LDI, A_NOP,  C_NOP, 0,0,Z, v));   // было C_LDI
    c.push_back(W(R_NOP, A_PASS, C_NOP));
}
static void set_mode_from_acc(std::vector<MicroWord>& c, uint8_t kchan = 0) {
    c.push_back(W(R_SETMODE, A_NOP, C_NOP, kchan,0,Z, 0));
}
static void set_reg(std::vector<MicroWord>& c, uint8_t idx, uint16_t v) {
    acc_from_imm(c, v);
    c.push_back(W(R_STR, A_NOP, C_NOP, idx));
}
static void set_reg24(std::vector<MicroWord>& c, uint8_t idx, uint32_t v) {
    acc_from_imm(c, (uint16_t)(v & 0xFFF));
    c.push_back(W(R_STR, A_NOP, C_NOP, idx));
    c.push_back(W(R_LDH, A_NOP, C_NOP, 0,0,Z, (uint16_t)((v >> 12) & 0xFFF)));
    c.push_back(W(R_NOP, A_OR, C_NOP));
    c.push_back(W(R_STR, A_NOP, C_NOP, idx));
}

// ====== test_addr: юнит-тест AddrPacker ======
static bool test_addr() {
    uint32_t a; uint8_t p;
    bool ok = true;
    // 4bpp: base=0x100, stride=2, x=0..7, y=0
    for (uint32_t x = 0; x < 8; ++x) {
        AddrPacker::compute(x, 0, 0x100, 2, FMT_4BPP, a, p);
        uint32_t want_a = 0x100 + (x >> 2) * 2;
        uint8_t  want_p = x & 3;
        if (a != want_a || p != want_p) {
            std::printf("  4bpp x=%u: addr=%03X pos=%u (want %03X/%u)\n", x, a, p, want_a, want_p);
            ok = false;
        }
    }
    // 8bpp: x=0..3 → pos 0/1, addr 0x100/0x102
    for (uint32_t x = 0; x < 4; ++x) {
        AddrPacker::compute(x, 0, 0x200, 2, FMT_8BPP, a, p);
        uint32_t want_a = 0x200 + (x >> 1) * 2;
        uint8_t  want_p = x & 1;
        if (a != want_a || p != want_p) {
            std::printf("  8bpp x=%u: addr=%03X pos=%u (want %03X/%u)\n", x, a, p, want_a, want_p);
            ok = false;
        }
    }
    // 1bpp: x=0..31
    for (uint32_t x = 0; x < 32; x += 5) {
        AddrPacker::compute(x, 0, 0x300, 2, FMT_1BPP, a, p);
        uint32_t want_a = 0x300 + (x >> 4) * 2;
        uint8_t  want_p = x & 15;
        if (a != want_a || p != want_p) {
            std::printf("  1bpp x=%u: addr=%03X pos=%u (want %03X/%u)\n", x, a, p, want_a, want_p);
            ok = false;
        }
    }
    // y: 4bpp, x=0, y=1 → addr 0x100 + 1*2 = 0x102
    AddrPacker::compute(0, 1, 0x100, 2, FMT_4BPP, a, p);
    if (a != 0x102 || p != 0) { std::printf("  y=1: addr=%03X pos=%u\n", a, p); ok = false; }
    std::printf("  %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

// ====== test_fill: 4 bpp, 2 слова = 0xAAAA ======
static bool test_fill(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    set_reg(c, R_DBASE, 0x100);
    set_reg(c, R_DSTRIDE, 2);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
    set_mode_from_acc(c, 1);
    for (int i = 0; i < 8; ++i) {
        acc_from_imm(c, (uint16_t)i);        // ACC = (y<<12)|x, y=0
        c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xA));
        c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    }
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint16_t w0 = mem.read16(0x100), w1 = mem.read16(0x102);
    std::printf("  words: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0xAAAA && w1 == 0xAAAA);

    return pass;
}

// ====== test_sprite: 4 bpp ======
static bool test_sprite(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    mem.write16(0x200, 0x0123);
    mem.write16(0x202, 0x4567);
    mem.write16(0x300, 0xFFFF);
    mem.write16(0x302, 0xFFFF);

    std::vector<MicroWord> c;
    set_reg(c, R_SBASE, 0x200);
    set_reg(c, R_SSTRIDE, 2);
    set_reg(c, R_DBASE, 0x300);
    set_reg(c, R_DSTRIDE, 2);

    for (int i = 0; i < 8; ++i) {
        acc_from_imm(c, (uint16_t)((0u << 3) | FMT_4BPP));
        set_mode_from_acc(c, 0);
        acc_from_imm(c, (uint16_t)i);
        c.push_back(W(R_PACKER_RD, A_NOP, C_NOP, 0,0,Z, 0));
        c.push_back(W(R_NOP, A_PASS, C_NOP));
        c.push_back(W(R_STR, A_NOP, C_NOP, R_T0));
        acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
        set_mode_from_acc(c, 1);
        acc_from_imm(c, (uint16_t)i);
        c.push_back(W(R_LDR, A_NOP, C_NOP, R_T0));
        c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    }
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint16_t w0 = mem.read16(0x300), w1 = mem.read16(0x302);
    std::printf("  dst: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0x0123 && w1 == 0x4567);

    return pass;
}

// ====== test_sprite8: 8 bpp ======
static bool test_sprite8(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    mem.write16(0x200, 0x3412);
    mem.write16(0x202, 0x7856);
    mem.write16(0x300, 0xFFFF);
    mem.write16(0x302, 0xFFFF);

    std::vector<MicroWord> c;
    set_reg(c, R_SBASE, 0x200);
    set_reg(c, R_SSTRIDE, 2);
    set_reg(c, R_DBASE, 0x300);
    set_reg(c, R_DSTRIDE, 2);

    for (int i = 0; i < 4; ++i) {
        acc_from_imm(c, (uint16_t)((0u << 3) | FMT_8BPP));
        set_mode_from_acc(c, 0);
        acc_from_imm(c, (uint16_t)i);
        c.push_back(W(R_PACKER_RD, A_NOP, C_NOP, 0,0,Z, 0));
        c.push_back(W(R_NOP, A_PASS, C_NOP));
        c.push_back(W(R_STR, A_NOP, C_NOP, R_T0));
        acc_from_imm(c, (uint16_t)((1u << 3) | FMT_8BPP));
        set_mode_from_acc(c, 1);
        acc_from_imm(c, (uint16_t)i);
        c.push_back(W(R_LDR, A_NOP, C_NOP, R_T0));
        c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    }
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint16_t w0 = mem.read16(0x300), w1 = mem.read16(0x302);
    std::printf("  dst: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0x3412 && w1 == 0x7856);

    return pass;
}

// ====== test_loop: с точным адресом PIX ======
static bool test_loop(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    set_reg(c, R_DBASE, 0x100);
    set_reg(c, R_DSTRIDE, 2);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
    set_mode_from_acc(c, 1);
    acc_from_imm(c, 0); c.push_back(W(R_SETD, A_NOP, C_NOP));
    acc_from_imm(c, 4); c.push_back(W(R_LDCNT, A_NOP, C_NOP));
    uint16_t pix = (uint16_t)c.size();
    // ACC = DIDX (координата x, y=0)
    c.push_back(W(R_LDX, A_NOP, C_NOP, 0,0,Z, OperandMap::DIDX));
    c.push_back(W(R_NOP, A_PASS, C_NOP));
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xA));   // OPR = 0xA
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    // DIDX += 1
    c.push_back(W(R_LDX, A_NOP, C_NOP, 0,0,Z, OperandMap::DIDX));
    c.push_back(W(R_NOP, A_PASS, C_NOP));
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 1));
    c.push_back(W(R_NOP, A_ADD, C_NOP));
    c.push_back(W(R_STX, A_NOP, C_NOP, 0,0,Z, OperandMap::DIDX));
    c.push_back(W(R_NOP, A_NOP, C_LOOP, 0,0,Z, pix));
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    std::printf("  DIDX=%u CNT=%u\n", gpu.reg(0), 0);  // нет, DIDX через OperandMap

    // 4 пикселя x=0..3 → одно слово 0x100 = 0xAAAA, 0x102 не тронут
    uint16_t w0 = mem.read16(0x100), w1 = mem.read16(0x102);
    std::printf("  words: %04X %04X\n", w0, w1);
    bool pass = (w0 == 0xAAAA && w1 == 0x0000);

    return pass;
}

// ====== test_setpage ======
static bool test_setpage(bool trace) {
    Memory16 mem(0x20000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    set_reg(c, R_DSTRIDE, 2);
    set_reg24(c, R_DBASE, 0x100);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
    set_mode_from_acc(c, 1);
    acc_from_imm(c, 0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xA));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    // set_page(DEST, 0x10000)
    c.push_back(W(R_LDH, A_NOP, C_NOP, 0,0,Z, 0x010));
    c.push_back(W(R_NOP, A_PASS, C_NOP));
    c.push_back(W(R_SETPAGE, A_NOP, C_NOP, 1,0,Z, 0));
    // DBASE = 0x10100
    set_reg24(c, R_DBASE, 0x10100);
    acc_from_imm(c, 0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xB));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint16_t p0 = mem.read16(0x00100);
    uint16_t p1 = mem.read16(0x10100);
    std::printf("  page0[0x100]=%04X page1[0x10100]=%04X\n", p0, p1);
    bool pass = (p0 == 0xA000 && p1 == 0xB000 && !gpu.buffer_error());

    return pass;
}

// ====== test_page_error ======
static bool test_page_error(bool trace) {
    Memory16 mem(0x20000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    set_reg(c, R_DSTRIDE, 2);
    set_reg24(c, R_DBASE, 0x100);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
    set_mode_from_acc(c, 1);
    acc_from_imm(c, 0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xA));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    // без set_page — DBASE в другой странице
    set_reg24(c, R_DBASE, 0x10100);
    acc_from_imm(c, 0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xB));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    gpu.run_until_halt(500000);

    uint16_t p1 = mem.read16(0x10100);
    bool pass = gpu.buffer_error() && (p1 == 0x0000);
    std::printf("  err=%d page1[0x10100]=%04X\n", (int)gpu.buffer_error(), p1);

    return pass;
}

// ====== test_mul ======
static bool test_mul(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 512);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    acc_from_imm(c, 0x123);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0x456));
    c.push_back(W(R_NOP, A_MUL, C_NOP));
    c.push_back(W(R_STR, A_NOP, C_NOP, R_T0));
    c.push_back(W(R_NOP, A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    uint64_t total = 0;
    while (gpu.busy() && total < 1000) total += gpu.tick(1);
    if (!gpu.halted()) { std::printf("  FAIL halt\n"); return false; }

    uint32_t got = gpu.acc();
    uint32_t want = 0x123u * 0x456u;
    std::printf("  acc=%06X want=%06X ticks=%llu\n", got, want, (unsigned long long)total);
    bool pass = (got == want) && (total >= 6);

    return pass;
}

// ====== test_pass: PASS16/LO8/HI8 через CH_DATA ======
static bool test_pass(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    mem.write16(0x200, 0xBEEF);

    std::vector<MicroWord> c;
    set_reg(c, R_DSTRIDE, 2);
    set_reg24(c, R_DBASE, 0x300);
    set_mode_from_acc(c, 1);   // fmt=0

    // read_word через CH_DATA (DPTR)
    auto read_word = [&](uint16_t fmt) {
        acc_from_imm(c, (uint16_t)((2u << 3) | fmt));   // ch=DATA
        set_mode_from_acc(c, 2);
        // DPTR = 0x200
        set_reg24(c, R_SBASE, 0x200);   // заглушка, не нужно
        acc_from_imm(c, 0x200);
        c.push_back(W(R_SETP, A_NOP, C_NOP));   // DPTR = 0x200
        c.push_back(W(R_PACKER_RD, A_NOP, C_NOP, 2,0,Z, 0));
        c.push_back(W(R_NOP, A_PASS, C_NOP));
        c.push_back(W(R_STR, A_NOP, C_NOP, R_T0));
            std::printf("  fmt=%u T0=%06X\n", fmt, gpu.reg(R_T0));
    };
    auto write_word = [&](uint16_t fmt, uint32_t dbase) {
        acc_from_imm(c, (uint16_t)((1u << 3) | fmt));
        set_mode_from_acc(c, 1);
        set_reg24(c, R_DBASE, dbase);
        acc_from_imm(c, 0);   // x=0, y=0
        c.push_back(W(R_LDR, A_NOP, C_NOP, R_T0));
        c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    };

    read_word(FMT_PASS16); write_word(FMT_PASS16, 0x300);

    read_word(FMT_PASS_LO8); write_word(FMT_PASS_LO8, 0x302);

    read_word(FMT_PASS_HI8); write_word(FMT_PASS_HI8, 0x304);

    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint16_t w0 = mem.read16(0x300);
    uint16_t w1 = mem.read16(0x302);
    uint16_t w2 = mem.read16(0x304);
    std::printf("  PASS16=%04X PASS_LO8=%04X PASS_HI8=%04X\n", w0, w1, w2);
    bool pass = (w0 == 0xBEEF && w1 == 0x00EF && w2 == 0xBE00);

    return pass;
}

// ====== test_mask ======
static bool test_mask(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    mem.write16(0x300, 0xFFFF);

    std::vector<MicroWord> c;
    set_reg(c, R_DSTRIDE, 2);
    set_reg24(c, R_DBASE, 0x300);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
    set_mode_from_acc(c, 1);
    // MASK = 0b0011 (разрешены pos 0,1)
    acc_from_imm(c, 0x0003);
    c.push_back(W(R_SETMASK, A_NOP, C_NOP, 0,0,Z, 0));
    // pos=0 → 0xA (разрешено)
    acc_from_imm(c, 0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xA));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    // pos=1 → 0xB (разрешено)
    acc_from_imm(c, 1);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xB));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    // pos=2 → 0xC (запрещено)
    acc_from_imm(c, 2);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xC));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint16_t w0 = mem.read16(0x300);
    std::printf("  word=%04X\n", w0);
    bool pass = (w0 == 0xABFF);   // pos0=A, pos1=B, pos2,3=F

    return pass;
}

// ====== test_comb_alu: AND/OR/XOR ======
static bool test_comb_alu(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 512);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    // AND: 0x0F0 & 0x00F = 0x000
    acc_from_imm(c, 0x0F0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0x00F));
    c.push_back(W(R_NOP, A_AND, C_NOP));
    c.push_back(W(R_STR, A_NOP, C_NOP, R_T0));
    // OR: 0x0F0 | 0x00F = 0x0FF
    acc_from_imm(c, 0x0F0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0x00F));
    c.push_back(W(R_NOP, A_OR, C_NOP));
    c.push_back(W(R_STR, A_NOP, C_NOP, R_T1));
    // XOR: 0x0F0 ^ 0x00F = 0x0FF
    acc_from_imm(c, 0x0F0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0x00F));
    c.push_back(W(R_NOP, A_XOR, C_NOP));
    c.push_back(W(R_STR, A_NOP, C_NOP, R_T2));
    c.push_back(W(R_NOP, A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    uint32_t and_v = gpu.reg(R_T0);
    uint32_t or_v  = gpu.reg(R_T1);
    uint32_t xor_v = gpu.reg(R_T2);
    std::printf("  AND=%06X OR=%06X XOR=%06X\n", and_v, or_v, xor_v);
    bool pass = (and_v == 0x000) && (or_v == 0x0FF) && (xor_v == 0x0FF);

    return pass;
}

// ====== test_mmio ======
static bool test_mmio(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    GpuMmio mmio(gpu);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    set_reg(c, R_DBASE, 0x100);
    set_reg(c, R_DSTRIDE, 2);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_4BPP));
    set_mode_from_acc(c, 1);
    acc_from_imm(c, 0);
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 0xA));
    c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);

    for (uint16_t w : pkt) {
        mmio.write(0x08, w & 0xFF);
        mmio.write(0x08, (w >> 8) & 0xFF);
    }
    mmio.write(0x0A, 1);
    if (mmio.read(0x0C)) { std::printf("  FAIL packet load\n"); return false; }

    mmio.write(0x01, 0); mmio.write(0x02, 0); mmio.write(0x03, 0);
    mmio.write(0x00, 1);

    for (int i = 0; i < 500000; ++i) {
        if (mmio.read(0x04) & 2) break;
        mmio.tick(1);
    }

    uint8_t st = mmio.read(0x04);
    uint16_t w = mem.read16(0x100);
    std::printf("  status=%02X word=%04X irq=%d\n", st, w, (int)mmio.irq());
    bool pass = (st & 2) && (w == 0xA000);

    return pass;
}

// ====== test_1bpp: 1 bpp, 16 пикселей в слове ======
static bool test_1bpp(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    set_reg(c, R_DBASE, 0x100);
    set_reg(c, R_DSTRIDE, 2);
    acc_from_imm(c, (uint16_t)((1u << 3) | FMT_1BPP));   // DEST fmt=0 (1bpp)
    set_mode_from_acc(c, 1);
    // пишем 16 пикселей x=0..15: чётные — 1, нечётные — 0
    // ожидание: биты 15,13,11,...,1 (старший = левый)
    for (int i = 0; i < 16; ++i) {
        acc_from_imm(c, (uint16_t)i);              // x=i, y=0
        c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, (i & 1) ? 0 : 1));
        c.push_back(W(R_PACKER_WR, A_NOP, C_NOP, 1,0,Z, 0));
    }
    c.push_back(W(R_FLUSH, A_NOP, C_NOP));
    c.push_back(W(R_NOP,   A_NOP, C_HALT));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(500000)) { std::printf("  FAIL halt\n"); return false; }

    // чётные x → 1, нечётные x → 0
    // бит 15 = x=0 (левый) = 1, бит 14 = x=1 = 0, ..., бит 0 = x=15 = 0
    // ожидание: 1010 1010 1010 1010 = 0xAAAA
    uint16_t w0 = mem.read16(0x100);
    std::printf("  word=%04X\n", w0);
    bool pass = (w0 == 0xAAAA);

    return pass;
}

// ====== test_wait: WAIT_FLAG ждёт, пока флаг не станет ======
static bool test_wait(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 1024);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    // CNT = 2
    acc_from_imm(c, 2);
    c.push_back(W(R_LDCNT, A_NOP, C_NOP));
    // WAIT_FLAG FLAG_CNT_ZERO (opnum=44, inv=0) — ждёт, пока CNT не 0
    // но CNT не меняется, значит, зациклится. Надо уменьшать CNT в цикле.
    // Проще: WAIT_FLAG FLAG_IRQ, а IRQ выставляем позже — но это не в одном потоке.
    // Ещё проще: WAIT_FLAG FLAG_BUSY (wait_ = 0 → сразу готово).
    // Тест: WAIT_FLAG FLAG_BUSY (inv=0) — wait_=0, v=0, inv=0 → ok=false, зациклится.
    // Надо inv=1: ждать, пока v == 0 (уже 0) → ok=true, сразу.
    c.push_back(W(R_NOP, A_NOP, C_WAIT_FLAG, 0,0,Z, (1u<<6) | OperandMap::FLAG_BUSY));
    // HALT 0
    c.push_back(W(R_NOP, A_NOP, C_HALT, 0,0,Z, 0));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(1000)) { std::printf("  FAIL halt\n"); return false; }

    std::printf("  halted=%d error=%d\n", (int)gpu.halted(), (int)gpu.error());
    bool pass = gpu.halted() && !gpu.error();

    return pass;
}

// ====== test_fault: HALT 1 → ERROR, error_code = 1 ======
static bool test_fault(bool trace) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 512);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    c.push_back(W(R_NOP, A_NOP, C_HALT, 0,0,Z, 1));   // HALT 1 → ERROR, код 1

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    gpu.run_until_halt(1000);

    std::printf("  error=%d error_code=%u\n", (int)gpu.error(), gpu.error_code());
    bool pass = gpu.error() && (gpu.error_code() == 1);

    return pass;
}

static bool test_call_ret(bool trace = false) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 512);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    // main: CALL f1; HALT 0
    uint16_t call1_pc = (uint16_t)c.size();
    c.push_back(W(R_NOP, A_NOP, C_CALL, 0,0,Z, 0));   // imm = f1
    c.push_back(W(R_NOP, A_NOP, C_HALT, 0,0,Z, 0));

    // f1: CALL f2; RET
    uint16_t f1_pc = (uint16_t)c.size();
    c.push_back(W(R_NOP, A_NOP, C_CALL, 0,0,Z, 0));   // imm = f2
    c.push_back(W(R_NOP, A_NOP, C_RET));

    // f2: CALL f3; RET
    uint16_t f2_pc = (uint16_t)c.size();
    c.push_back(W(R_NOP, A_NOP, C_CALL, 0,0,Z, 0));   // imm = f3
    c.push_back(W(R_NOP, A_NOP, C_RET));

    // f3: CALL f4; RET
    uint16_t f3_pc = (uint16_t)c.size();
    c.push_back(W(R_NOP, A_NOP, C_CALL, 0,0,Z, 0));   // imm = f4
    c.push_back(W(R_NOP, A_NOP, C_RET));

    // f4: CALL f5; RET
    uint16_t f4_pc = (uint16_t)c.size();
    c.push_back(W(R_NOP, A_NOP, C_CALL, 0,0,Z, 0));   // imm = f5
    c.push_back(W(R_NOP, A_NOP, C_RET));

    // f5: CALL f6; RET
    uint16_t f5_pc = (uint16_t)c.size();
    c.push_back(W(R_NOP, A_NOP, C_CALL, 0,0,Z, 0));   // imm = f6
    c.push_back(W(R_NOP, A_NOP, C_RET));

    // f6: ACC = 0x42; RET
    uint16_t f6_pc = (uint16_t)c.size();
    acc_from_imm(c, 0x42);
    c.push_back(W(R_NOP, A_NOP, C_RET));

    // заполнить CALL imm
    c[call1_pc].imm = f1_pc;
    c[f1_pc + 0].imm = f2_pc;
    c[f2_pc + 0].imm = f3_pc;
    c[f3_pc + 0].imm = f4_pc;
    c[f4_pc + 0].imm = f5_pc;
    c[f5_pc + 0].imm = f6_pc;

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(50000)) { std::printf("  FAIL halt\n"); return false; }

    uint32_t got = gpu.acc();
    std::printf("  acc=%06X (want 000042)\n", got);
    bool pass = (got == 0x42);
    return pass;
}

static bool test_loop_nested(bool trace = false) {
    Memory16 mem(0x10000);
    GPU gpu(mem, 512);
    gpu.set_trace(trace);

    std::vector<MicroWord> c;
    // CNT = 2 (внешний)
    acc_from_imm(c, 2); c.push_back(W(R_LDCNT, A_NOP, C_NOP));
    uint16_t outer = (uint16_t)c.size();
    // CNT = 3 (внутренний)
    acc_from_imm(c, 3); c.push_back(W(R_LDCNT, A_NOP, C_NOP));
    uint16_t inner = (uint16_t)c.size();
    // ACC += 1, T0 = ACC
    c.push_back(W(R_LDI, A_NOP, C_NOP, 0,0,Z, 1));
    c.push_back(W(R_NOP, A_ADD, C_NOP));
    c.push_back(W(R_STR, A_NOP, C_NOP, R_T0));
    c.push_back(W(R_NOP, A_NOP, C_LOOP, 0,0,Z, inner));
    c.push_back(W(R_NOP, A_NOP, C_LOOP, 0,0,Z, outer));
    c.push_back(W(R_NOP, A_NOP, C_HALT, 0,0,Z, 0));

    std::vector<uint16_t> pkt;
    emit_code(pkt, 0, c); emit_run(pkt, 0); emit_end(pkt);
    if (!gpu.load_packet(pkt.data(), pkt.size())) { std::printf("  FAIL load\n"); return false; }

    gpu.start(0);
    if (!gpu.run_until_halt(50000)) { std::printf("  FAIL halt\n"); return false; }

    uint32_t t0 = gpu.reg(R_T0);
    std::printf("  T0=%06X (want 000006)\n", t0);
    bool pass = (t0 == 6);
    return pass;
}

// helper: запустить тест; если FAIL — повторить с trace
static bool run_test(const char* name, bool (*fn)(bool)) {
    std::printf("=== %s ===\n", name);
    bool ok = fn(false);
    if (!ok) {
        std::printf("  --- retry with trace ---\n");
        fn(true);   // второй прогон с trace, результат не важен
    }
    std::printf("  %s\n", ok ? "PASS" : "FAIL");
    return ok;
}

int main() {
    std::printf("lx-gpu sim, mem=64KiB\n\n");
    bool ok = test_addr();
    std::printf("\n"); ok = run_test("test_fill",      test_fill)      && ok;
    std::printf("\n"); ok = run_test("test_sprite",    test_sprite)    && ok;
    std::printf("\n"); ok = run_test("test_sprite8",   test_sprite8)   && ok;
    std::printf("\n"); ok = run_test("test_loop",      test_loop)      && ok;
    std::printf("\n"); ok = run_test("test_setpage",   test_setpage)   && ok;
    std::printf("\n"); ok = run_test("test_page_error",test_page_error)&& ok;
    std::printf("\n"); ok = run_test("test_mul",       test_mul)       && ok;
    std::printf("\n"); ok = run_test("test_pass",      test_pass)      && ok;
    std::printf("\n"); ok = run_test("test_mask",      test_mask)      && ok;
    std::printf("\n"); ok = run_test("test_comb_alu",  test_comb_alu)  && ok;
    std::printf("\n"); ok = run_test("test_mmio",      test_mmio)      && ok;
    std::printf("\n"); ok = run_test("test_1bpp",      test_1bpp)      && ok;
    std::printf("\n"); ok = run_test("test_wait",      test_wait)      && ok;
    std::printf("\n"); ok = run_test("test_fault",     test_fault)     && ok;
    std::printf("\n"); ok = run_test("test_call_ret",     test_call_ret)     && ok;
    std::printf("\n"); ok = run_test("test_loop_nested",  test_loop_nested)     && ok;
    std::printf("\n%s\n", ok ? "ALL PASS" : "SOME FAIL");
    return ok ? 0 : 1;
}