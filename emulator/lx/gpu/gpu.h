#pragma once
#include <cstdint>
#include <vector>
#include "memory.h"
#include "pixel_buffer.h"
#include "pixel_packer.h"
#include "addr_packer.h"

// --- глубина стеков (софт). В RTL: 1 (или 2) ---
#ifndef LX_GPU_LOOP_DEPTH
#define LX_GPU_LOOP_DEPTH 8
#endif
#ifndef LX_GPU_CALL_DEPTH
#define LX_GPU_CALL_DEPTH 8
#endif

namespace lx {

struct MicroWord {
    uint8_t  reg_op   : 5;
    uint8_t  alu_op   : 5;
    uint8_t  ctrl_op  : 3;
    uint8_t  reg_addr : 5;
    uint8_t  mask     : 3;
    uint8_t  cond     : 3;
    uint16_t imm      : 12;
    void unpack(const uint16_t w[3]);
    void pack(uint16_t w[3]) const;
};

enum RegOp : uint8_t {
    R_NOP=0, R_PASS, R_LDR, R_STR, R_LDD, R_LDDS,
    R_SETS, R_SETD, R_SETP, R_LDCNT, R_FLUSH,
    R_LDX, R_STX,
    R_LDI, R_LDH, R_LDL,
    R_PACKER_RD, R_PACKER_WR,
    R_SETMODE, R_SETPAGE,
    R_ADDR, R_SETMASK,
    R_IRQ,
};

enum AluOp : uint8_t {
    A_NOP=0, A_PASS, A_ADD, A_SUB, A_AND, A_OR, A_XOR, A_SHL,
    A_SHR, A_SAR, A_MIN, A_MAX, A_MUL, A_ADDS, A_SUBS,
};

enum CtrlOp : uint8_t {
    C_NOP=0, C_JMP, C_CALL, C_RET, C_LOOP, C_JMPA, C_HALT, C_WAIT_FLAG,
};
enum Cond : uint8_t { Z=0, NZ, N, NN, C, NC, RES0, RES1 };

enum RIdx : uint8_t {
    R_DX=0, R_DY, R_DW, R_DH, R_SX, R_SY, R_SW, R_SH,
    R_DSTRIDE, R_SSTRIDE, R_DBASE, R_SBASE, R_FG, R_BG, R_MODE,
    R_CX0, R_CY0, R_CX1, R_CY1, R_SDX, R_SDY, R_DDX, R_DDY,
    R_T0=23, R_T1, R_T2, R_T3, R_T4, R_T5, R_T6, R_T7, R_T8,
};

struct OperandMap {
    static constexpr uint8_t ZERO=32, ACC=33, SPIX=34, DPIX=35,
        SIDX=36, DIDX=37, DPTR=38, FLAG=39, CNT=40,
        FLAG_BUSY=41, FLAG_IRQ=42, FLAG_DMA=43, FLAG_CNT_ZERO=44;
};

class GPU {
public:
    GPU(Memory16& mem, uint32_t microcode_words = 1024);

    void reset();
    void start(uint32_t entry = 0);
    bool busy() const   { return state_ == RUN || state_ == WAIT; }
    bool halted() const { return state_ == IDLE; }
    bool error() const  { return state_ == ERROR; }

    int  tick(int cpu_cycles);
    bool run_until_halt(uint64_t limit = 1'000'000);
    bool load_packet(const uint16_t* words, size_t count);

    void set_clock_ratio(uint32_t num, uint32_t den) { clk_num_ = num; clk_den_ = den ? den : 1; }

    uint32_t acc() const { return acc_; }
    uint32_t opr() const { return opr_; }
    uint8_t  flags() const { return (z_?1:0) | (n_?2:0) | (c_?4:0); }
    uint16_t irq_code() const { return irq_code_; }
    bool     irq() const { return irq_pending_; }
    void     clear_irq() { irq_pending_ = false; }
    uint16_t error_code() const { return error_code_; }

    void set_ch_mode(int ch, ChannelMode m) { if (ch >= 0 && ch < 4) ch_mode_[ch] = m; }
    ChannelMode ch_mode(int ch) const { return (ch >= 0 && ch < 4) ? ch_mode_[ch] : ChannelMode{}; }
    void set_page(Channel ch, uint32_t addr24) { buffer_.set_page(ch, addr24); }
    void set_mask(uint16_t m) { mask_ = m; }
    uint16_t mask() const { return mask_; }

    bool buffer_error() const   { return buffer_.error(); }
    void clear_buffer_error()   { buffer_.clear_error(); }

    uint32_t reg(int idx) const { return (idx >= 0 && idx < 32) ? r_[idx] : 0; }
    uint32_t didx() const { return didx_; }
    uint32_t sidx() const { return sidx_; }
    uint32_t dptr() const { return dptr_; }
    uint32_t cnt()  const { return loop_stack_.empty() ? 0 : loop_stack_.back(); }

    void set_trace(bool on) { trace_ = on; }
    bool trace() const { return trace_; }

    Memory16& mem;
    PixelBuffer buffer_;
    PixelPacker packer_;

private:
    enum State { IDLE, RUN, WAIT, ERROR };
    State state_ = IDLE;

    std::vector<MicroWord> code_;
    uint16_t pc_ = 0;
    std::vector<uint16_t> call_stack_;      // CALL/RET, до LX_GPU_CALL_DEPTH
    std::vector<uint32_t> loop_stack_;      // LOOP/LDCNT, до LX_GPU_LOOP_DEPTH

    uint32_t acc_ = 0, opr_ = 0;
    bool z_ = false, n_ = false, c_ = false;
    int  wait_ = 0;

    uint32_t r_[32] = {};
    uint32_t sidx_ = 0, didx_ = 0, dptr_ = 0;
    ChannelMode ch_mode_[4] = {};
    uint16_t mask_ = 0xFFFF;

    bool irq_pending_ = false;
    uint16_t irq_code_ = 0;
    uint16_t error_code_ = 0;

    uint32_t clk_num_ = 1, clk_den_ = 1;
    bool trace_ = false;

    int      tick_one();
    uint32_t read_operand(uint8_t addr);
    void     write_reg(uint8_t addr, uint32_t v);
    uint32_t exec_alu(AluOp op);
    void     exec_ctrl(CtrlOp op, uint16_t imm);
    static uint32_t sat_add(uint32_t a, uint32_t b, uint8_t width);
    static uint32_t sat_sub(uint32_t a, uint32_t b, uint8_t width);
    bool cond_true(Cond c) const;
};

} // namespace lx