#pragma once
#include <cstdint>
#include <vector>
#include "memory.h"
#include "pixel_buffer.h"
#include "pixel_packer.h"
#include "addr_packer.h"

namespace lx {

struct MicroWord {
    uint8_t  reg_op   : 5;
    uint8_t  imm_class: 2;
    uint8_t  dst      : 5;
    uint8_t  ds       : 1;
    uint8_t  cond     : 4;
    uint8_t  f        : 1;
    uint8_t  l        : 1;
    uint8_t  sx       : 1;
    uint16_t imm12    : 12;
    uint8_t  src      : 6;
    uint32_t imm24    : 24;

    void unpack(const uint16_t w[2]);
    void pack(uint16_t w[2]) const;
};

enum AluOp : uint8_t {
    A_MOV=0, A_AND, A_OR, A_XOR, A_ANDN,
    A_ADD, A_SUB, A_CMP, A_TST,
    A_ROR, A_SHR, A_SAR,
    A_MUL
};

enum CtrlOp : uint8_t {
    C_NOP=0, C_JMP, C_CALL, C_RET, C_JMPR, C_HALT, C_IRQ,
};

enum Cond : uint8_t {
    AL=0, EQ, NE, CS, CC, MI, PL, VS, VC, HI, LS, GE, LT, GT, LE,
    COND_RSV=15,
};

enum Operand : uint8_t {
    OP_ZERO       = 32,
    OP_FLAGS      = 33,
    OP_SHADOW     = 34,
    OP_LOOP       = 35,
    OP_BUSY_SRC   = 36,
    OP_BUSY_DST   = 37,
    OP_BUSY_STM   = 38,
    OP_SRC_IDX    = 39,
    OP_SRC_PIX    = 40,
    OP_SRC_W16    = 41,
    OP_SRC_STEP   = 42,
    OP_SRC_MODE   = 43,
    OP_SRC_PAGE   = 44,
    OP_DST_IDX    = 45,
    OP_DST_PIX    = 46,
    OP_DST_W16    = 47,
    OP_DST_STEP   = 48,
    OP_DST_MODE   = 49,
    OP_DST_PAGE   = 50,
    OP_STM_PTR    = 51,
    OP_STM_W16    = 52,
    OP_STM_W24    = 53,
    OP_STM_STEP   = 54,
    OP_STM_MODE   = 55,
    OP_STM_PAGE   = 56,
    OP_FLUSH      = 57,
    OP_STM_PIX    = 58,
};

enum RIdx : uint8_t {
    R_DX=0, R_DY, R_DW, R_DH, R_SX, R_SY, R_SW, R_SH,
    R_DSTRIDE, R_SSTRIDE, R_DBASE, R_SBASE, R_FG, R_BG, R_MODE,
    R_CX0, R_CY0, R_CX1, R_CY1, R_SDX, R_SDY, R_DDX, R_DDY,
    R_T0=23, R_T1, R_T2, R_T3, R_T4, R_T5, R_T6, R_T7, R_T8,
};

class GPU {
public:
    GPU(Memory16& mem, uint32_t microcode_words = 1024);

    void reset();
    void start(uint32_t entry = 0);
    bool busy() const   { return state_ == RUN; }
    bool halted() const { return state_ == IDLE; }
    bool error() const  { return state_ == ERROR; }

    int  tick(int cpu_cycles);
    bool run_until_halt(uint64_t limit = 1'000'000);
    bool load_packet(const uint16_t* words, size_t count);

    uint32_t a() const { return A_; }
    uint32_t b() const { return B_; }
    uint32_t c() const { return C_; }
    uint8_t  flags() const { return (z_?1:0) | (n_?2:0) | (c_?4:0) | (v_?8:0); }

    uint16_t error_code() const { return error_code_; }
    bool     irq_pending() const { return irq_pending_; }
    uint16_t irq_code() const    { return irq_code_; }
    void     clear_irq()         { irq_pending_ = false; }

    void set_ch_mode(int ch, uint8_t fmt) { if (ch>=0 && ch<4) ch_mode_[ch] = fmt & 7; }
    uint8_t ch_mode(int ch) const { return (ch>=0 && ch<4) ? ch_mode_[ch] : 0; }
    void set_page(Channel ch, uint8_t page) { buffer_.set_page(ch, page); }

    bool buffer_error() const  { return buffer_.error(); }
    void clear_buffer_error()  { buffer_.clear_error(); }

    uint32_t reg(int idx) const { return (idx>=0 && idx<32) ? r_[idx] : 0; }
    uint32_t shadow() const { return shadow_; }
    uint32_t loop_reg() const { return loop_reg_; }

    void set_trace(bool on) { trace_ = on; }
    bool trace() const { return trace_; }

    Memory16& mem;
    PixelBuffer buffer_;
    PixelPacker packer_;

private:
    enum State { IDLE, RUN, ERROR };
    State state_ = IDLE;

    std::vector<MicroWord> code_;
    uint16_t pc_ = 0;
    uint32_t shadow_ = 0;
    uint32_t loop_reg_ = 0;

    uint32_t A_ = 0, B_ = 0, C_ = 0;
    uint32_t R_ = 0;
    bool z_=false, n_=false, c_=false, v_=false;

    uint32_t r_[32] = {};
    uint8_t  ch_mode_[4] = {};
    uint8_t  ch_page_[4] = {};
    uint32_t ch_idx_[4] = {};
    uint32_t ch_step_[4] = {};

    uint16_t error_code_ = 0;
    bool     irq_pending_ = false;
    uint16_t irq_code_ = 0;
    bool trace_ = false;

    int      tick_one();
    uint32_t read_operand(uint8_t op);
    void     write_operand(uint8_t op, uint32_t v);
    uint32_t exec_alu(AluOp op, uint32_t a, uint32_t b);
    void     exec_ctrl(CtrlOp op, uint16_t addr, uint8_t reg);
    bool     cond_true(Cond c) const;
    void     set_flags(uint32_t res, bool carry, bool overflow);
    void     push(uint32_t v) { C_=B_; B_=A_; A_=v; }
    void     pop()            { A_=B_; B_=C_; C_=0; }
};

} // namespace lx