#include "gpu.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace lx {

// 36 бит: w0 = [15:0], w1 = [31:16], w2 = [35:32]
void MicroWord::unpack(const uint16_t w[3]) {
    uint64_t v = (uint64_t)w[0] | ((uint64_t)w[1] << 16) | ((uint64_t)w[2] << 32);
    imm      =  v        & 0xFFF;
    cond     = (v >> 12) & 0x7;
    mask     = (v >> 15) & 0x7;
    reg_addr = (v >> 18) & 0x1F;
    ctrl_op  = (v >> 23) & 0x7;    // 3 бита
    alu_op   = (v >> 26) & 0x1F;
    reg_op   = (v >> 31) & 0x1F;   // 5 бит
}
void MicroWord::pack(uint16_t w[3]) const {
    uint64_t v = (uint64_t)imm
               | ((uint64_t)cond     << 12)
               | ((uint64_t)mask     << 15)
               | ((uint64_t)reg_addr << 18)
               | ((uint64_t)ctrl_op  << 23)
               | ((uint64_t)alu_op   << 26)
               | ((uint64_t)reg_op   << 31);
    w[0] = v & 0xFFFF;
    w[1] = (v >> 16) & 0xFFFF;
    w[2] = (v >> 32) & 0xFFFF;
}

GPU::GPU(Memory16& m, uint32_t n)
    : mem(m), buffer_(m), packer_(buffer_), code_(n ? n : 1) { reset(); }

void GPU::reset() {
    state_ = IDLE; pc_ = 0;
    call_stack_.clear();
    loop_stack_.clear();
    acc_ = opr_ = 0; z_ = n_ = c_ = false; wait_ = 0;
    std::memset(r_, 0, sizeof(r_));
    sidx_ = didx_ = dptr_ = 0;
    for (auto& m : ch_mode_) m = ChannelMode{};
    mask_ = 0xFFFF;
    irq_pending_ = false; irq_code_ = 0; error_code_ = 0;
    buffer_.reset();
}

void GPU::start(uint32_t entry) {
    if (state_ == ERROR) return;
    pc_ = entry & 0xFFFF;
    state_ = RUN;
}

uint32_t GPU::read_operand(uint8_t a) {
    switch (a) {
    case OperandMap::ZERO: return 0;
    case OperandMap::ACC:  return acc_;
    case OperandMap::SPIX: {
        uint16_t px = 0;
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(sidx_ & 0xFFF, (sidx_ >> 12) & 0xFFF,
                            r_[R_SBASE], r_[R_SSTRIDE],
                            (PixFmt)ch_mode_[CH_SOURCE].fmt, addr, pos);
        packer_.read_pixel(CH_SOURCE, addr, (PixFmt)ch_mode_[CH_SOURCE].fmt, pos, px);
        return px;
    }
    case OperandMap::DPIX: {
        uint16_t px = 0;
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(didx_ & 0xFFF, (didx_ >> 12) & 0xFFF,
                            r_[R_DBASE], r_[R_DSTRIDE],
                            (PixFmt)ch_mode_[CH_DEST].fmt, addr, pos);
        packer_.read_pixel(CH_DEST, addr, (PixFmt)ch_mode_[CH_DEST].fmt, pos, px);
        return px;
    }
    case OperandMap::SIDX: return sidx_;
    case OperandMap::DIDX: return didx_;
    case OperandMap::DPTR: return dptr_;
    case OperandMap::FLAG: return flags();
    case OperandMap::CNT:  return loop_stack_.empty() ? 0 : loop_stack_.back();
    case OperandMap::FLAG_BUSY:     return (wait_ > 0) ? 1 : 0;
    case OperandMap::FLAG_IRQ:      return irq_pending_ ? 1 : 0;
    case OperandMap::FLAG_DMA:      return 0;   // заглушка, пока нет DMA
    case OperandMap::FLAG_CNT_ZERO: return (loop_stack_.empty() || loop_stack_.back() == 0) ? 1 : 0;
    default: return a < 32 ? r_[a] : 0;
    }
}
void GPU::write_reg(uint8_t a, uint32_t v) {
    if (a < 32) { r_[a] = v & 0xFFFFFF; return; }
    switch (a) {
    case OperandMap::SIDX: sidx_ = v & 0xFFFFFF; break;
    case OperandMap::DIDX: didx_ = v & 0xFFFFFF; break;
    case OperandMap::DPTR: dptr_ = v & 0xFFFFFF; break;
    case OperandMap::CNT:
        if (loop_stack_.size() >= LX_GPU_LOOP_DEPTH) loop_stack_.erase(loop_stack_.begin());
        loop_stack_.push_back(v & 0xFFFFFF);
        break;
    case OperandMap::DPIX: {
        uint32_t addr; uint8_t pos;
        AddrPacker::compute(didx_ & 0xFFF, (didx_ >> 12) & 0xFFF,
                            r_[R_DBASE], r_[R_DSTRIDE],
                            (PixFmt)ch_mode_[CH_DEST].fmt, addr, pos);
        packer_.write_pixel(CH_DEST, addr, (PixFmt)ch_mode_[CH_DEST].fmt, pos, (uint16_t)v, mask_);
        break;
    }
    default: break;
    }
}

uint32_t GPU::sat_add(uint32_t a, uint32_t b, uint8_t width) {
    uint32_t max = (width >= 24) ? 0xFFFFFF : ((1u << width) - 1);
    uint32_t s = a + b;
    return s > max ? max : s;
}
uint32_t GPU::sat_sub(uint32_t a, uint32_t b, uint8_t width) {
    (void)width;
    return a < b ? 0 : a - b;
}

uint32_t GPU::exec_alu(AluOp op) {
    uint32_t a = acc_, b = opr_, res = a;
    PixFmt fmt = (PixFmt)ch_mode_[CH_DEST].fmt;
    uint8_t width = (fmt == FMT_4BPP) ? 4 : (fmt == FMT_8BPP) ? 8
                  : (fmt == FMT_1BPP) ? 1 : 24;
    switch (op) {
    case A_NOP:  return a;
    case A_PASS: return b & 0xFFFFFF;
    case A_ADD:  res=a+b; z_=((res&0xFFFFFF)==0); n_=(res>>23)&1; c_=res>0xFFFFFF; return res&0xFFFFFF;
    case A_SUB:  { uint32_t d=(a-b)&0xFFFFFF; z_=(d==0); n_=(d>>23)&1; c_=a<b; return d; }
    case A_AND:  res=a&b; z_=(res==0); n_=(res>>23)&1; return res&0xFFFFFF;
    case A_OR:   res=a|b; z_=(res==0); n_=(res>>23)&1; return res&0xFFFFFF;
    case A_XOR:  res=a^b; z_=(res==0); n_=(res>>23)&1; return res&0xFFFFFF;
    case A_SHL:  res=(a<<(b&31))&0xFFFFFF; z_=(res==0); return res;
    case A_SHR:  res=(a>>(b&31))&0xFFFFFF; z_=(res==0); return res;
    case A_SAR:  { int32_t s=(int32_t)(a<<8)>>8; res=(uint32_t)(s>>(b&31))&0xFFFFFF; z_=(res==0); return res; }
    case A_MIN:  { int32_t sa=(int32_t)(a<<8)>>8, sb=(int32_t)(b<<8)>>8; res=(sa<sb)?a:b; z_=((res&0xFFFFFF)==0); return res&0xFFFFFF; }
    case A_MAX:  { int32_t sa=(int32_t)(a<<8)>>8, sb=(int32_t)(b<<8)>>8; res=(sa>sb)?a:b; z_=((res&0xFFFFFF)==0); return res&0xFFFFFF; }
    case A_MUL:  res=(a&0xFFF)*(b&0xFFF); z_=(res==0); return res&0xFFFFFF;
    case A_ADDS: res=sat_add(a,b,width); z_=(res==0); return res & 0xFFFFFF;
    case A_SUBS: res=sat_sub(a,b,width); z_=(res==0); return res & 0xFFFFFF;
    }
    return res & 0xFFFFFF;
}

void GPU::exec_ctrl(CtrlOp op, uint16_t imm) {
    switch (op) {
    case C_NOP:  break;
    case C_JMP:  pc_ = imm & 0xFFFF; break;
    case C_CALL:
        if (call_stack_.size() >= LX_GPU_CALL_DEPTH) call_stack_.erase(call_stack_.begin());
        call_stack_.push_back(pc_);
        pc_ = imm & 0xFFFF;
        break;
    case C_RET:
        if (!call_stack_.empty()) { pc_ = call_stack_.back(); call_stack_.pop_back(); }
        break;
    case C_LOOP: {
        if (loop_stack_.empty()) break;
        if (--loop_stack_.back() == 0) loop_stack_.pop_back();
        else pc_ = imm & 0xFFFF;
        break;
    }
    case C_JMPA: pc_ = acc_ & 0x1FF; break;
    case C_HALT: {
        if (imm == 0) { state_ = IDLE; }
        else          { state_ = ERROR; error_code_ = imm & 0xFFF; }
        break;
    }
    case C_WAIT_FLAG: {
        // IMM[5:0] = operand, IMM[6] = inv
        uint8_t opnum = imm & 0x3F;
        bool    inv   = (imm >> 6) & 1;
        uint32_t v = read_operand(opnum);
        bool ok = inv ? (v == 0) : (v != 0);
        if (ok) { /* готово, pc_ уже next_pc */ }
        else    { pc_ = (uint16_t)(pc_ - 1); }   // остаться на этой команде
        break;
    }
    }
}

bool GPU::cond_true(Cond c) const {
    switch (c) {
    case Z: return z_; case NZ: return !z_;
    case N: return n_; case NN: return !n_;
    case C: return c_; case NC: return !c_;
    default: return false;
    }
}

int GPU::tick_one() {
    if (state_ != RUN && state_ != WAIT) return 1;
    if (wait_ > 0) { --wait_; state_ = (wait_>0)?WAIT:RUN; return 1; }

    MicroWord w = code_[pc_ & (code_.size()-1)];
    uint16_t next_pc = (uint16_t)(pc_+1);
    uint32_t old_acc = acc_, old_opr = opr_;
    bool active = (w.mask == 0) || !cond_true((Cond)w.cond);
    uint32_t new_acc = old_acc;
    int extra = 0;

    if (trace_) {
        std::printf("PC=%04X R=%X A=%X C=%X ra=%u imm=%03X | ACC=%06X OPR=%06X "
                    "DIDX=%06X SIDX=%06X DPTR=%06X CNT=%u\n",
                    pc_, w.reg_op, w.alu_op, w.ctrl_op, w.reg_addr, w.imm,
                    acc_, opr_, didx_, sidx_, dptr_, loop_stack_.empty() ? 0 : loop_stack_.back());
    }

    if (active && w.reg_op != R_NOP) {
        switch (w.reg_op) {
        case R_PASS: break;
        case R_LDR:  if (w.reg_addr < 32) opr_ = r_[w.reg_addr]; break;
        case R_STR:  if (w.reg_addr < 32) r_[w.reg_addr] = old_acc & 0xFFFFFF; break;
        case R_LDX:  opr_ = read_operand((uint8_t)(w.imm & 0x3F)); break;
        case R_STX:  write_reg((uint8_t)(w.imm & 0x3F), old_acc); break;
        case R_LDD:  opr_ = mem.read16(dptr_); dptr_=(dptr_+2)&0xFFFFFF; break;
        case R_LDDS: opr_ = (uint16_t)(int16_t)mem.read16(dptr_); dptr_=(dptr_+2)&0xFFFFFF; break;
        case R_SETS: sidx_ = old_acc & 0xFFFFFF; break;
        case R_SETD: didx_ = old_acc & 0xFFFFFF; break;
        case R_SETP: dptr_ = old_acc & 0xFFFFFF; break;
        case R_LDCNT:
            if (loop_stack_.size() >= LX_GPU_LOOP_DEPTH) loop_stack_.erase(loop_stack_.begin());
            loop_stack_.push_back(old_acc & 0xFFFFFF);
            break;
        case R_FLUSH: wait_ += buffer_.flush(); break;
        case R_LDI:  opr_ = (uint32_t)(int32_t)(int16_t)((w.imm&0xFFF)|((w.imm&0x800)?0xF000:0)); break;
        case R_LDH:  opr_ = (uint32_t)(w.imm&0xFFF) << 12; break;
        case R_LDL:  opr_ = (opr_ & 0xFFFFF000) | (w.imm & 0xFFF); break;
        case R_PACKER_RD: {
            Channel ch = (Channel)(w.reg_addr & 3);   // канал из REG_ADDR[1:0]
            uint16_t px = 0;
            if (ch == CH_DATA) {
                uint16_t word = mem.read16(dptr_);
                dptr_ = (dptr_ + 2) & 0xFFFFFF;
                switch (ch_mode_[CH_DATA].fmt) {
                case FMT_PASS_LO8: px = word & 0xFF; break;
                case FMT_PASS_HI8: px = (word >> 8) & 0xFF; break;
                default:           px = word; break;
                }
                wait_ += 1;
            } else {
                uint32_t x_int = acc_ & 0xFFF, y_int = (acc_ >> 12) & 0xFFF;
                uint32_t base   = (ch == CH_SOURCE) ? r_[R_SBASE] : r_[R_DBASE];
                uint32_t stride = (ch == CH_SOURCE) ? r_[R_SSTRIDE] : r_[R_DSTRIDE];
                uint32_t addr; uint8_t pos;
                AddrPacker::compute(x_int, y_int, base, stride,
                                    (PixFmt)ch_mode_[ch].fmt, addr, pos);
                wait_ += packer_.read_pixel(ch, addr, (PixFmt)ch_mode_[ch].fmt, pos, px);
            }
            opr_ = px;
            break;
        }
        case R_PACKER_WR: {
            Channel ch = (Channel)(w.reg_addr & 3);
            if (ch == CH_DATA) {
                uint16_t word = (uint16_t)opr_;
                switch (ch_mode_[CH_DATA].fmt) {
                case FMT_PASS_LO8: word = (word & 0xFF00) | (opr_ & 0xFF); break;
                case FMT_PASS_HI8: word = (word & 0x00FF) | ((opr_ & 0xFF) << 8); break;
                default: break;
                }
                mem.write16(dptr_, word);
                dptr_ = (dptr_ + 2) & 0xFFFFFF;
                wait_ += 1;
            } else {
                uint32_t x_int = acc_ & 0xFFF, y_int = (acc_ >> 12) & 0xFFF;
                uint32_t base   = (ch == CH_SOURCE) ? r_[R_SBASE] : r_[R_DBASE];
                uint32_t stride = (ch == CH_SOURCE) ? r_[R_SSTRIDE] : r_[R_DSTRIDE];
                uint32_t addr; uint8_t pos;
                AddrPacker::compute(x_int, y_int, base, stride,
                                    (PixFmt)ch_mode_[ch].fmt, addr, pos);
                wait_ += packer_.write_pixel(ch, addr, (PixFmt)ch_mode_[ch].fmt, pos,
                                             (uint16_t)opr_, mask_);
            }
            break;
        }
        case R_SETMODE: {
            // канал из REG_ADDR[1:0], fmt из ACC[2:0]
            uint8_t kchan = w.reg_addr & 3;
            ChannelMode m{};
            m.fmt = acc_ & 7;
            ch_mode_[kchan] = m;
            break;
        }
        case R_SETPAGE: {
            // канал из REG_ADDR[1:0], страница из ACC
            Channel ch = (Channel)(w.reg_addr & 3);
            wait_ += buffer_.set_page(ch, acc_ & 0xFFFFFF);
            break;
        }
        case R_ADDR: {
            // канал из REG_ADDR[1:0], x/y из ACC
            Channel ch = (Channel)(w.reg_addr & 3);
            uint32_t x_int = acc_ & 0xFFF, y_int = (acc_ >> 12) & 0xFFF;
            uint32_t base   = (ch == CH_SOURCE) ? r_[R_SBASE] : r_[R_DBASE];
            uint32_t stride = (ch == CH_SOURCE) ? r_[R_SSTRIDE] : r_[R_DSTRIDE];
            uint32_t addr; uint8_t pos;
            AddrPacker::compute(x_int, y_int, base, stride,
                                (PixFmt)ch_mode_[ch].fmt, addr, pos);
            acc_ = addr; opr_ = pos;
            break;
        }
        case R_IRQ: irq_pending_ = true; irq_code_ = acc_ & 0xFFF; break;
        case R_SETMASK: mask_ = (uint16_t)(acc_ & 0xFFFF); break;
        }
    }
    if (active && w.alu_op != A_NOP) {
        uint32_t save = opr_; opr_ = old_opr;
        new_acc = exec_alu((AluOp)w.alu_op);
        opr_ = save;
        if (w.alu_op == A_MUL) extra = 1;
    }
    if (active && w.ctrl_op != C_NOP) {
        pc_ = next_pc;
        exec_ctrl((CtrlOp)w.ctrl_op, w.imm);
        acc_ = new_acc;
        return 1 + extra;
    }
    acc_ = new_acc;
    pc_ = next_pc;
    if (extra) { wait_ = extra; state_ = WAIT; }
    return 1 + extra;
}

int GPU::tick(int cpu_cycles) {
    if (cpu_cycles <= 0) return 0;
    int gpu_cycles = (int)((int64_t)cpu_cycles * clk_num_ / clk_den_);
    if (gpu_cycles <= 0) gpu_cycles = 1;
    int done = 0;
    while (done < gpu_cycles && (state_ == RUN || state_ == WAIT)) {
        done += tick_one();
    }
    return done;
}

bool GPU::run_until_halt(uint64_t limit) {
    while (busy() && limit--) tick_one();
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
                uint16_t raw[3];
                if (!next(raw[0])||!next(raw[1])||!next(raw[2])) return false;
                MicroWord mw; mw.unpack(raw);
                if ((uint32_t)addr+k < code_.size()) code_[addr+k]=mw;
            }
            break;
        }
        case 3: start(arg); break;
        case 4: { uint16_t lo,hi; if(!next(lo)||!next(hi)) return false;
                  dptr_ = lo | ((uint32_t)hi<<16); break; }
        default: return false;
        }
    }
    return true;
}

} // namespace lx