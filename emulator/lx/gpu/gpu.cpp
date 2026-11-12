#include "gpu.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace lx {

// ===== pack/unpack 32 бита в 2 слова =====
void MicroWord::unpack(const uint16_t w[2]) {
    uint32_t v = (uint32_t)w[0] | ((uint32_t)w[1] << 16);
    l         = (v >> 31) & 1;
    uint8_t J = (v >> 30) & 1;
    uint8_t I = (v >> 29) & 1;
    imm_class = (uint8_t)((J<<1) | I);
    dst       = (v >> 24) & 0x1F;
    ds        = (v >> 23) & 1;
    cond      = (v >> 19) & 0xF;
    f         = (v >> 18) & 1;
    reg_op    = (v >> 13) & 0x1F;
    sx        = (v >> 12) & 1;
    imm12 = v & 0xFFF;
    src = (v >> 7) & 0x3F;
    imm24     = v & 0xFFFFFF;
}
void MicroWord::pack(uint16_t w[2]) const {
    uint32_t v = ((uint32_t)l << 31)
               | ((uint32_t)((imm_class>>1)&1) << 30)
               | ((uint32_t)(imm_class&1) << 29)
               | ((uint32_t)(dst & 0x1F) << 24)
               | ((uint32_t)(ds & 1) << 23)
               | ((uint32_t)(cond & 0xF) << 19)
               | ((uint32_t)(f & 1) << 18)
               | ((uint32_t)(reg_op & 0x1F) << 13)
               | ((uint32_t)(sx & 1) << 12)
               | ((uint32_t)(imm12 & 0xFFF));
    // reg-reg / ctrl: src в [12:7], imm12 в [11:0] — перекрытие!
    // Для reg-reg: src[5:0] занимает [12:7], imm12 не используется.
    // Для ctrl:    reg[5:0] занимает [12:7], addr[11:0] — [11:0].
    // Для reg-imm: sx[12], imm12[11:0].
    // Для LDI24:   imm24[23:0] — [23:0].
    if (imm_class == 0) {          // reg-reg
        v = (v & ~0x1F80u) | ((uint32_t)(src & 0x3F) << 7);
    } else if (imm_class == 2) {   // ctrl
        if (src != 0) {
            v = (v & ~0x1F80u) | ((uint32_t)(src & 0x3F) << 7);
            // imm12 = 0
        } else {
            v = (v & ~0xFFFu) | (imm12 & 0xFFF);
        }
    } else if (imm_class == 1) {   // LDI24
        v = (v & 0xFF000000u) | (imm24 & 0xFFFFFF);
    }
    // imm_class == 3 (reg-imm12): sx[12], imm12[11:0] уже в v
    w[0] = v & 0xFFFF;
    w[1] = (v >> 16) & 0xFFFF;
}

GPU::GPU(Memory16& m, uint32_t n)
    : mem(m), buffer_(m), packer_(buffer_), code_(n ? n : 1) { reset(); }

void GPU::reset() {
    state_ = IDLE; pc_ = 0; shadow_ = 0; loop_reg_ = 0;
    A_ = B_ = C_ = R_ = 0;
    z_ = n_ = c_ = v_ = false;
    std::memset(r_, 0, sizeof(r_));
    std::memset(ch_mode_, 0, sizeof(ch_mode_));
    std::memset(ch_page_, 0, sizeof(ch_page_));
    std::memset(ch_idx_, 0, sizeof(ch_idx_));
    std::memset(ch_step_, 0, sizeof(ch_step_));
    error_code_ = 0;
    irq_pending_ = false;
    irq_code_ = 0;
    buffer_.reset();
}

void GPU::start(uint32_t entry) {
    if (state_ == ERROR) return;
    pc_ = entry & 0xFFF;
    state_ = RUN;
}

// ===== флаги =====
void GPU::set_flags(uint32_t res, bool carry, bool overflow) {
    z_ = (res & 0xFFFFFF) == 0;
    n_ = (res >> 23) & 1;
    c_ = carry;
    v_ = overflow;
}

bool GPU::cond_true(Cond c) const {
    switch (c) {
    case AL: return true;
    case EQ: return z_;
    case NE: return !z_;
    case CS: return c_;
    case CC: return !c_;
    case MI: return n_;
    case PL: return !n_;
    case VS: return v_;
    case VC: return !v_;
    case HI: return c_ && !z_;
    case LS: return !c_ || z_;
    case GE: return n_ == v_;
    case LT: return n_ != v_;
    case GT: return !z_ && (n_ == v_);
    case LE: return z_ || (n_ != v_);
    default: return false;
    }
}

// ===== ALU =====
uint32_t GPU::exec_alu(AluOp op, uint32_t a, uint32_t b) {
    a &= 0xFFFFFF; b &= 0xFFFFFF;
    uint32_t res = a;
    bool carry = false, overflow = false;
    switch (op) {
    case A_MOV:  res = b; break;
    case A_AND:  res = a & b; break;
    case A_OR:   res = a | b; break;
    case A_XOR:  res = a ^ b; break;
    case A_ANDN: res = a & ~b; break;
    case A_ADD: {
        uint64_t s = (uint64_t)a + (uint64_t)b;
        res = (uint32_t)(s & 0xFFFFFF);
        carry = (s >> 24) & 1;
        overflow = (((a ^ res) & (b ^ res)) >> 23) & 1;
        break;
    }
    case A_SUB: {
        uint64_t d = (uint64_t)a - (uint64_t)b;
        res = (uint32_t)(d & 0xFFFFFF);
        carry = (a >= b);
        overflow = (((a ^ b) & (a ^ res)) >> 23) & 1;
        break;
    }
    case A_CMP: {
        uint64_t d = (uint64_t)a - (uint64_t)b;
        res = (uint32_t)(d & 0xFFFFFF);
        carry = (a >= b);
        overflow = (((a ^ b) & (a ^ res)) >> 23) & 1;
        // результат отбрасывается, но флаги пишем всегда
        set_flags(res, carry, overflow);
        return a;
    }
    case A_TST: {
        res = a & b;
        set_flags(res, false, false);
        return a;
    }
    case A_ROR: {
        uint32_t s = b % 24;
        res = (s == 0) ? a : ((a >> s) | (a << (24 - s))) & 0xFFFFFF;
        break;
    }
    case A_SHR: {
        uint32_t s = b & 0x1F;
        res = (s >= 24) ? 0 : (a >> s);
        break;
    }
    case A_SAR: {
        uint32_t s = b & 0x1F;
        int32_t sa = (int32_t)(a << 8) >> 8;
        res = (uint32_t)(sa >> (s > 23 ? 23 : s)) & 0xFFFFFF;
        break;
    }
    case A_MUL: {
        int64_t p = (int64_t)(int32_t)(a << 8 >> 8) * (int64_t)(int32_t)(b << 8 >> 8);
        res = (uint32_t)(p & 0xFFFFFF);
        break;
    }
    case A_MULH: {
        int64_t p = (int64_t)(int32_t)(a << 8 >> 8) * (int64_t)(int32_t)(b << 8 >> 8);
        res = (uint32_t)((p >> 12) & 0xFFFFFF);
        break;
    }
    default: return a;
    }
    return res;
}

// ===== CTRL =====
void GPU::exec_ctrl(CtrlOp op, uint16_t addr, uint8_t reg) {
    switch (op) {
    case C_NOP: break;
    case C_JMP: pc_ = addr & 0xFFF; break;
    case C_CALL:
        shadow_ = (uint32_t)((pc_ + 1) & 0xFFF)
                | ((uint32_t)((z_?1:0)|(n_?2:0)|(c_?4:0)|(v_?8:0)) << 12);
        pc_ = addr & 0xFFF;
        break;
    case C_RET: pc_ = shadow_ & 0xFFF; break;
    case C_JMPR: pc_ = read_operand(reg) & 0xFFF; break;
    case C_IRQ: irq_pending_ = true; irq_code_ = addr & 0xFFF; break;    
    case C_HALT:
        if (addr == 0) { state_ = IDLE; }
        else { state_ = ERROR; error_code_ = addr & 0xFFF; }
        break;
    }
}

// ===== Операнды =====
uint32_t GPU::read_operand(uint8_t op) {
    if (op < 32) return r_[op];
    switch (op) {
    case OP_ZERO:   return 0;
    case OP_FLAGS:  return (uint32_t)((z_?1:0)|(n_?2:0)|(c_?4:0)|(v_?8:0));
    case OP_SHADOW: return shadow_;
    case OP_LOOP:   return loop_reg_;
    case OP_BUSY_SRC: case OP_BUSY_DST: case OP_BUSY_STM: return 0;
    case OP_SRC_IDX: return ch_idx_[CH_SOURCE];
    case OP_SRC_STEP:return ch_step_[CH_SOURCE];
    case OP_SRC_MODE:return ch_mode_[CH_SOURCE];
    case OP_SRC_PAGE:return ch_page_[CH_SOURCE];
    case OP_DST_IDX: return ch_idx_[CH_DEST];
    case OP_DST_STEP:return ch_step_[CH_DEST];
    case OP_DST_MODE:return ch_mode_[CH_DEST];
    case OP_DST_PAGE:return ch_page_[CH_DEST];
    case OP_STM_PTR: return ch_idx_[CH_STM];
    case OP_STM_STEP:return ch_step_[CH_STM];
    case OP_STM_MODE:return ch_mode_[CH_STM];
    case OP_STM_PAGE:return ch_page_[CH_STM];
    case OP_SRC_PIX: {
        // read pixel from SRC, advance IDX by STEP
        Channel ch = CH_SOURCE;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(idx, page, 0, 0, fmt, addr, pos);
        uint16_t px = 0;
        packer_.read_pixel(ch, addr, fmt, pos, px);
        ch_idx_[ch] = idx + (int32_t)ch_step_[ch];
        return px;
    }
    case OP_DST_PIX: {
        Channel ch = CH_DEST;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(idx, page, 0, 0, fmt, addr, pos);
        uint16_t px = 0;
        packer_.read_pixel(ch, addr, fmt, pos, px);
        return px;   // IDX не меняется на read
    }
    case OP_SRC_W16: {
        Channel ch = CH_SOURCE;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        uint32_t addr = ((uint32_t)page << 16) | ((idx & 0x7FFF) << 1);
        uint16_t w = mem.read16(addr);
        ch_idx_[ch] = idx + 1;
        return w;
    }
    case OP_DST_W16: {
        Channel ch = CH_DEST;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        uint32_t addr = ((uint32_t)page << 16) | ((idx & 0x7FFF) << 1);
        uint16_t w = mem.read16(addr);
        return w;   // IDX не меняется
    }
    case OP_STM_W16: {
        Channel ch = CH_STM;
        uint32_t ptr = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr = ((uint32_t)page << 16) | ((ptr & 0x7FFF) << 1);
        uint16_t w = mem.read16(addr);
        ch_idx_[ch] = ptr + 1;
        // применить fmt
        switch (fmt) {
        case FMT_PASS_LO8: return w & 0xFF;
        case FMT_PASS_HI8: return (w >> 8) & 0xFF;
        case FMT_PASS16:   return w;
        default:           return w;   // для пиксельных fmt — raw word
        }
    }
    case OP_STM_W24: {
        Channel ch = CH_STM;
        uint32_t ptr = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        uint32_t addr0 = ((uint32_t)page << 16) | ((ptr & 0x7FFF) << 1);
        uint32_t addr1 = ((uint32_t)page << 16) | (((ptr+1) & 0x7FFF) << 1);
        uint16_t w0 = mem.read16(addr0);
        uint16_t w1 = mem.read16(addr1);
        ch_idx_[ch] = ptr + 2;
        return (uint32_t)w0 | ((uint32_t)(w1 & 0xFF) << 16);
    }
    case OP_STM_PIX: {
        Channel ch = CH_STM;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(idx, page, 0, 0, fmt, addr, pos);
        uint16_t px = 0;
        packer_.read_pixel(ch, addr, fmt, pos, px);
        ch_idx_[ch] = idx + (int32_t)ch_step_[ch];
        return px;
    }    
    default: return 0;
    }
}

void GPU::write_operand(uint8_t op, uint32_t v) {
    if (op < 32) { r_[op] = v & 0xFFFFFF; return; }
    switch (op) {
    case OP_ZERO:   break;
    case OP_FLAGS:  z_=(v&1); n_=(v>>1)&1; c_=(v>>2)&1; v_=(v>>3)&1; break;
    case OP_SHADOW: shadow_ = v & 0xFFFF; break;
    case OP_LOOP:   loop_reg_ = v & 0xFFFFFF; break;
    case OP_SRC_IDX: ch_idx_[CH_SOURCE] = v & 0xFFFFFF; break;
    case OP_SRC_STEP:ch_step_[CH_SOURCE] = v; break;
    case OP_SRC_MODE:ch_mode_[CH_SOURCE] = v & 7; break;
    case OP_SRC_PAGE:ch_page_[CH_SOURCE] = v & 0xFF; buffer_.set_page(CH_SOURCE, v & 0xFF); break;
    case OP_DST_IDX: ch_idx_[CH_DEST] = v & 0xFFFFFF; break;
    case OP_DST_STEP:ch_step_[CH_DEST] = v; break;
    case OP_DST_MODE:ch_mode_[CH_DEST] = v & 7; break;
    case OP_DST_PAGE:ch_page_[CH_DEST] = v & 0xFF; buffer_.set_page(CH_DEST, v & 0xFF); break;
    case OP_STM_PTR: ch_idx_[CH_STM] = v & 0xFFFFFF; break;
    case OP_STM_STEP:ch_step_[CH_STM] = v; break;
    case OP_STM_MODE:ch_mode_[CH_STM] = v & 7; break;
    case OP_STM_PAGE:ch_page_[CH_STM] = v & 0xFF; buffer_.set_page(CH_STM, v & 0xFF); break;
    case OP_SRC_PIX: {
        Channel ch = CH_SOURCE;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(idx, page, 0, 0, fmt, addr, pos);
        packer_.write_pixel(ch, addr, fmt, pos, (uint16_t)v);
        ch_idx_[ch] = idx + (int32_t)ch_step_[ch];
        break;
    }
    case OP_DST_PIX: {
        Channel ch = CH_DEST;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(idx, page, 0, 0, fmt, addr, pos);
        packer_.write_pixel(ch, addr, fmt, pos, (uint16_t)v);
        ch_idx_[ch] = idx + (int32_t)ch_step_[ch];
        break;
    }
    case OP_SRC_W16: {
        Channel ch = CH_SOURCE;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        uint32_t addr = ((uint32_t)page << 16) | ((idx & 0x7FFF) << 1);
        mem.write16(addr, (uint16_t)v);
        ch_idx_[ch] = idx + 1;
        break;
    }
    case OP_DST_W16: {
        Channel ch = CH_DEST;
        uint32_t idx = ch_idx_[ch];
        uint8_t page = ch_page_[ch];
        PixFmt fmt = (PixFmt)ch_mode_[ch];
        uint32_t addr = ((uint32_t)page << 16) | ((idx & 0x7FFF) << 1);
        uint16_t cur = mem.read16(addr);
        uint16_t out = cur;
        switch (fmt) {
        case FMT_PASS_LO8: out = (cur & 0xFF00) | (v & 0xFF); break;
        case FMT_PASS_HI8: out = (cur & 0x00FF) | ((v & 0xFF) << 8); break;
        case FMT_PASS16:   out = (uint16_t)v; break;
        default:           out = (uint16_t)v; break;
        }
        mem.write16(addr, out);
        ch_idx_[ch] = idx + 1;
        break;
    }
    case OP_FLUSH: buffer_.flush(); break;
    default: break;
    }
}

// ===== tick =====
int GPU::tick_one() {
    if (state_ != RUN) return 1;

    MicroWord w = code_[pc_ & (code_.size() - 1)];
    uint16_t next_pc = (uint16_t)((pc_ + 1) & 0xFFF);

    if (trace_) {
        std::printf("PC=%03X cls=%u op=%u dst=%u src=%u imm=%03X A=%06X B=%06X C=%06X Z%d N%d C%d V%d\n",
                    pc_, w.imm_class, w.reg_op, w.dst, w.src, w.imm12,
                    A_, B_, C_, (int)z_, (int)n_, (int)c_, (int)v_);
    }

    bool active = cond_true((Cond)w.cond);
    bool is_ctrl = (w.imm_class == 2);

    if (!active) {
        pc_ = next_pc;
        if (w.l) {
            uint32_t cnt = (loop_reg_ >> 12) & 0xFFF;
            if (cnt > 1) {
                loop_reg_ = ((cnt - 1) << 12) | (loop_reg_ & 0xFFF);
                pc_ = loop_reg_ & 0xFFF;
            }
        }
        return 1;
    }

    if (is_ctrl) {
        exec_ctrl((CtrlOp)w.reg_op, w.imm12, w.src);
        if (state_ != RUN) return 1;
        if (!(w.reg_op == C_JMP || w.reg_op == C_CALL ||
              w.reg_op == C_RET || w.reg_op == C_JMPR)) {
            pc_ = next_pc;
        }
        if (w.l) {
            uint32_t cnt = (loop_reg_ >> 12) & 0xFFF;
            if (cnt > 1) {
                loop_reg_ = ((cnt - 1) << 12) | (loop_reg_ & 0xFFF);
                pc_ = loop_reg_ & 0xFFF;
            }
        }
        return 1;
    }

    if (w.imm_class == 1) { // LDI24
        if (w.dst < 32) r_[w.dst] = w.imm24 & 0xFFFFFF;
        pc_ = next_pc;
        return 1;
    }

    // reg-reg (00) или reg-imm12 (11)
    uint32_t a = read_operand((uint8_t)(w.dst | (w.ds << 5)));
    uint32_t b;
    if (w.imm_class == 0) {
        b = read_operand(w.src);
    } else {
        uint32_t imm = w.imm12;
        if (w.sx && (imm & 0x800)) imm |= 0xFFFFF000u;
        b = imm & 0xFFFFFF;
    }

    AluOp op = (AluOp)w.reg_op;
    uint32_t res = exec_alu(op, a, b);

    bool write = !(op == A_CMP || op == A_TST);
    if (write) {
        write_operand((uint8_t)(w.dst | (w.ds << 5)), res);
    }
    if (w.f || op == A_CMP || op == A_TST) {
        // флаги уже выставлены внутри CMP/TST; для остальных — здесь
        if (op != A_CMP && op != A_TST) {
            bool carry = false, overflow = false;
            if (op == A_ADD) { uint64_t s=(uint64_t)a+(uint64_t)b; carry=(s>>24)&1; overflow=(((a^res)&(b^res))>>23)&1; }
            else if (op == A_SUB) { carry=(a>=b); overflow=(((a^b)&(a^res))>>23)&1; }
            set_flags(res, carry, overflow);
        }
    }

    // L
    if (w.l) {
        uint32_t cnt = (loop_reg_ >> 12) & 0xFFF;
        if (cnt > 1) { loop_reg_ = ((cnt - 1) << 12) | (loop_reg_ & 0xFFF); pc_ = loop_reg_ & 0xFFF; return 1; }
    }
    pc_ = next_pc;
    return 1;
}

int GPU::tick(int cpu_cycles) {
    if (cpu_cycles <= 0) return 0;
    int done = 0;
    while (done < cpu_cycles && state_ == RUN) {
        done += tick_one();
    }
    return done;
}

bool GPU::run_until_halt(uint64_t limit) {
    while (state_ == RUN && limit--) tick_one();
    return state_ == IDLE;
}

bool GPU::load_packet(const uint16_t* w, size_t n) {
    size_t i = 0;
    auto next = [&](uint16_t& v){ if (i>=n) return false; v=w[i++]; return true; };
    while (i < n) {
        uint16_t h; if (!next(h)) return false;
        uint8_t kind = (h>>12)&0xF; uint16_t arg = h & 0xFFF;
        switch (kind) {
        case 0: return true;
        case 1: irq_pending_ = true; irq_code_ = arg; return true;
        case 2: {
            uint16_t addr; if (!next(addr)) return false;
            for (uint16_t k=0;k<arg;++k) {
                uint16_t raw[2];
                if (!next(raw[0])||!next(raw[1])) return false;
                MicroWord mw; mw.unpack(raw);
                if ((uint32_t)addr+k < code_.size()) code_[addr+k]=mw;
            }
            break;
        }
        case 3: start(arg); break;
        case 4: { uint16_t lo,hi; if(!next(lo)||!next(hi)) return false;
                  ch_idx_[CH_STM] = lo | ((uint32_t)hi<<16); break; }
        default: return false;
        }
    }
    return true;
}

} // namespace lx