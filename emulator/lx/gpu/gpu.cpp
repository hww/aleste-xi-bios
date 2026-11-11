#include "gpu.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace lx {

void MicroWord::unpack(const uint16_t w[3]) {
    uint64_t v = (uint64_t)w[0] | ((uint64_t)w[1] << 16) | ((uint64_t)w[2] << 32);
    imm      =  v        & 0xFFF;
    cond     = (v >> 12) & 0x7;
    mask     = (v >> 15) & 0x7;
    reg_addr = (v >> 18) & 0x1F;
    ctrl_op  = (v >> 23) & 0x7;
    alu_op   = (v >> 26) & 0x1F;
    reg_op   = (v >> 31) & 0x1F;
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
    state_ = IDLE; pc_ = 0; call_stack_.clear(); loop_cnt_ = 0;
    A_ = B_ = C_ = R_ = 0;
    z_ = n_ = c_ = false;
    std::memset(r_, 0, sizeof(r_));
    sidx_ = didx_ = dptr_ = 0;
    for (auto& m : ch_mode_) m = ChannelMode{};
    mask_ = 0xFFFF;
    ADDR_ = 0; ADDR_pos_ = 0; ADDR_ch_ = 0; ADDR_valid_ = false;
    irq_pending_ = false; irq_code_ = 0; error_code_ = 0;
    buffer_.reset();
}

void GPU::start(uint32_t entry) {
    if (state_ == ERROR) return;
    pc_ = entry & 0xFFFF;
    state_ = RUN;
}

uint32_t GPU::read_device(uint8_t dev) {
    switch (dev) {
    case DeviceMap::ZERO:   return 0;
    case DeviceMap::A:      return A_;
    case DeviceMap::B:      return B_;
    case DeviceMap::C:      return C_;
    case DeviceMap::SIDX:   return sidx_;
    case DeviceMap::DIDX:   return didx_;
    case DeviceMap::DPTR:   return dptr_;
    case DeviceMap::FLAG:   return flags();
    case DeviceMap::CNT:    return loop_cnt_;
    case DeviceMap::FLAG_BUSY:          return 0;  // модель: память мгновенная
    case DeviceMap::FLAG_IRQ:           return irq_pending_ ? 1 : 0;
    case DeviceMap::FLAG_DMA:           return 0;
    case DeviceMap::FLAG_CNT_ZERO:      return (loop_cnt_ == 0) ? 1 : 0;
    case DeviceMap::FLAG_BUSY_NOT:      return 1;
    case DeviceMap::FLAG_IRQ_NOT:       return irq_pending_ ? 0 : 1;
    case DeviceMap::FLAG_DMA_NOT:       return 1;
    case DeviceMap::FLAG_CNT_ZERO_NOT:  return (loop_cnt_ == 0) ? 0 : 1;
    default: return 0;
    }
}
void GPU::write_device(uint8_t dev, uint32_t v) {
    switch (dev) {
    case DeviceMap::SIDX: sidx_ = v & 0xFFFFFF; break;
    case DeviceMap::DIDX: didx_ = v & 0xFFFFFF; break;
    case DeviceMap::DPTR: dptr_ = v & 0xFFFFFF; break;
    case DeviceMap::CNT:  loop_cnt_ = v & 0xFFFFFF; break;
    default: break;
    }
}

static uint32_t logic16(uint8_t fn, uint32_t a, uint32_t b) {
    uint32_t res = 0;
    for (int i = 0; i < 24; ++i) {
        uint32_t ab = ((a >> i) & 1) | (((b >> i) & 1) << 1);
        if (fn & (1u << ab)) res |= (1u << i);
    }
    return res & 0xFFFFFF;
}

uint32_t GPU::exec_alu(AluOp op) {
    uint32_t a = A_, b = B_;
    uint32_t res = a;
    switch (op) {
    case A_LOGIC_0:  res = logic16(0x0, a, b); z_ = (res == 0); return res;
    case A_LOGIC_1:  res = logic16(0x1, a, b); z_ = (res == 0); return res;
    case A_LOGIC_2:  res = logic16(0x2, a, b); z_ = (res == 0); return res;
    case A_LOGIC_3:  res = logic16(0x3, a, b); z_ = (res == 0); return res;
    case A_LOGIC_4:  res = logic16(0x4, a, b); z_ = (res == 0); return res;
    case A_LOGIC_5:  res = logic16(0x5, a, b); z_ = (res == 0); return res;
    case A_LOGIC_6:  res = logic16(0x6, a, b); z_ = (res == 0); return res;
    case A_LOGIC_7:  res = logic16(0x7, a, b); z_ = (res == 0); return res;
    case A_LOGIC_8:  res = logic16(0x8, a, b); z_ = (res == 0); return res;
    case A_LOGIC_9:  res = logic16(0x9, a, b); z_ = (res == 0); return res;
    case A_LOGIC_10: res = logic16(0xA, a, b); z_ = (res == 0); return res;
    case A_LOGIC_11: res = logic16(0xB, a, b); z_ = (res == 0); return res;
    case A_LOGIC_12: res = logic16(0xC, a, b); z_ = (res == 0); return res;
    case A_LOGIC_13: res = logic16(0xD, a, b); z_ = (res == 0); return res;
    case A_LOGIC_14: res = logic16(0xE, a, b); z_ = (res == 0); return res;
    case A_LOGIC_15: res = logic16(0xF, a, b); z_ = (res == 0); return res;

    case A_ADD: res = a + b; z_ = ((res & 0xFFFFFF) == 0); n_ = (res >> 23) & 1; c_ = res > 0xFFFFFF; return res & 0xFFFFFF;
    case A_SUB: { uint32_t d = (a - b) & 0xFFFFFF; z_ = (d == 0); n_ = (d >> 23) & 1; c_ = a < b; return d; }
    case A_CMP: { uint32_t d = (a - b) & 0xFFFFFF; z_ = (d == 0); n_ = (d >> 23) & 1; c_ = a < b; return a; }
    case A_SHL: res = (a << (b & 31)) & 0xFFFFFF; z_ = (res == 0); return res;
    case A_SHR: res = (a >> (b & 31)) & 0xFFFFFF; z_ = (res == 0); return res;
    case A_SAR: { int32_t s = (int32_t)(a << 8) >> 8; res = (uint32_t)(s >> (b & 31)) & 0xFFFFFF; z_ = (res == 0); return res; }
    default: return a;
    }
}

void GPU::exec_ctrl(CtrlOp op, uint16_t imm) {
    switch (op) {
    case C_NOP:  break;
    case C_JMP:  pc_ = imm & 0xFFFF; break;
    case C_CALL: call_stack_.push_back(pc_); pc_ = imm & 0xFFFF; break;
    case C_RET:  if (!call_stack_.empty()) { pc_ = call_stack_.back(); call_stack_.pop_back(); } break;
    case C_LOOP: if (--loop_cnt_) pc_ = imm & 0xFFFF; break;
    case C_JMPA: pc_ = A_ & 0x1FF; break;
    case C_HALT: if (imm == 0) { state_ = IDLE; } else { state_ = ERROR; error_code_ = imm & 0xFFF; } break;
    case C_WAIT_FLAG: {
        uint8_t dev = (uint8_t)(imm & 0x1F);
        uint32_t v = read_device(dev);
        if (v == 0) pc_ = (uint16_t)(pc_ - 1);  // крутится
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
    if (state_ != RUN) return 1;

    MicroWord w = code_[pc_ & (code_.size() - 1)];
    uint16_t next_pc = (uint16_t)(pc_ + 1);
    bool active = (w.mask == 0) || !cond_true((Cond)w.cond);

    if (trace_) {
        std::printf("PC=%04X R=%X A=%X C=%X ra=%u imm=%03X | A=%06X B=%06X C=%06X\n",
                    pc_, w.reg_op, w.alu_op, w.ctrl_op, w.reg_addr, w.imm, A_, B_, C_);
    }

    uint32_t old_a = A_;
    bool do_alu = active && (w.alu_op != A_LOGIC_12);
    bool do_reg = active && (w.reg_op != R_NOP);
    bool do_ctrl = active && (w.ctrl_op != C_NOP);

    if (do_reg) {
        switch (w.reg_op) {
        case R_LDR:       A_ = (w.reg_addr < 32) ? r_[w.reg_addr] : 0; break;
        case R_LDR_PUSH:  push((w.reg_addr < 32) ? r_[w.reg_addr] : 0); break;
        case R_STR:       if (w.reg_addr < 32) r_[w.reg_addr] = old_a & 0xFFFFFF; break;
        case R_STR_POP:   if (w.reg_addr < 32) r_[w.reg_addr] = old_a & 0xFFFFFF; pop(); break;
        case R_CLR_A:     A_ = 0; break;
        case R_CLR_AB:    A_ = 0; B_ = 0; break;
        case R_CLR_C:     C_ = 0; break;
        case R_LDD:       A_ = mem.read16(dptr_); dptr_ = (dptr_ + 2) & 0xFFFFFF; break;
        case R_LDD_PUSH:  push(mem.read16(dptr_)); dptr_ = (dptr_ + 2) & 0xFFFFFF; break;
        case R_LDDS:      A_ = (uint16_t)(int16_t)mem.read16(dptr_); dptr_ = (dptr_ + 2) & 0xFFFFFF; break;
        case R_LDDS_PUSH: push((uint16_t)(int16_t)mem.read16(dptr_)); dptr_ = (dptr_ + 2) & 0xFFFFFF; break;
        case R_SETS:      sidx_ = old_a & 0xFFFFFF; break;
        case R_SETD:      didx_ = old_a & 0xFFFFFF; break;
        case R_SETP:      dptr_ = old_a & 0xFFFFFF; break;
        case R_LDCNT:     loop_cnt_ = old_a & 0xFFFFFF; break;
        case R_FLUSH:     buffer_.flush(); break;
        case R_LDX:       A_ = read_device((uint8_t)(w.reg_addr & 0x1F)); break;
        case R_LDX_PUSH:  push(read_device((uint8_t)(w.reg_addr & 0x1F))); break;
        case R_STX:       write_device((uint8_t)(w.reg_addr & 0x1F), old_a); break;
        case R_STX_POP:   write_device((uint8_t)(w.reg_addr & 0x1F), old_a); pop(); break;
        case R_LDI:       A_ = (uint32_t)(int32_t)(int16_t)((w.imm & 0xFFF) | ((w.imm & 0x800) ? 0xF000 : 0)); break;
        case R_LDI_PUSH:  push((uint32_t)(int32_t)(int16_t)((w.imm & 0xFFF) | ((w.imm & 0x800) ? 0xF000 : 0))); break;
        case R_LDH:       A_ = (A_ & 0xFFF) | ((uint32_t)(w.imm & 0xFFF) << 12); break;
        case R_LDL:       A_ = (A_ & 0xFFFFF000) | (w.imm & 0xFFF); break;
        case R_PACKER_RD: {
            Channel ch = (Channel)(w.reg_addr & 3);
            uint16_t px = 0;
            if (ADDR_valid_ && ADDR_ch_ == (uint8_t)ch) {
                packer_.read_pixel(ch, ADDR_, (PixFmt)ch_mode_[ch].fmt, ADDR_pos_, px);
                ADDR_valid_ = false;
            } else if (ch == CH_DATA) {
                uint16_t word = mem.read16(dptr_);
                dptr_ = (dptr_ + 2) & 0xFFFFFF;
                switch (ch_mode_[CH_DATA].fmt) {
                case FMT_PASS_LO8: px = word & 0xFF; break;
                case FMT_PASS_HI8: px = (word >> 8) & 0xFF; break;
                default:           px = word; break;
                }
            }
            A_ = px;
            break;
        }
        case R_PACKER_RD_PUSH: {
            Channel ch = (Channel)(w.reg_addr & 3);
            uint16_t px = 0;
            if (ADDR_valid_ && ADDR_ch_ == (uint8_t)ch) {
                packer_.read_pixel(ch, ADDR_, (PixFmt)ch_mode_[ch].fmt, ADDR_pos_, px);
                ADDR_valid_ = false;
            }
            push(px);
            break;
        }
        case R_PACKER_WR: {
            Channel ch = (Channel)(w.reg_addr & 3);
            if (ADDR_valid_ && ADDR_ch_ == (uint8_t)ch) {
                packer_.write_pixel(ch, ADDR_, (PixFmt)ch_mode_[ch].fmt, ADDR_pos_,
                                    (uint16_t)old_a, mask_);
                ADDR_valid_ = false;
            } else if (ch == CH_DATA) {
                mem.write16(dptr_, (uint16_t)old_a);
                dptr_ = (dptr_ + 2) & 0xFFFFFF;
            }
            break;
        }
        case R_PACKER_WR_POP: {
            Channel ch = (Channel)(w.reg_addr & 3);
            if (ADDR_valid_ && ADDR_ch_ == (uint8_t)ch) {
                packer_.write_pixel(ch, ADDR_, (PixFmt)ch_mode_[ch].fmt, ADDR_pos_,
                                    (uint16_t)old_a, mask_);
                ADDR_valid_ = false;
            }
            pop();
            break;
        }
        case R_SETMODE: {
            uint8_t kchan = w.reg_addr & 3;
            ChannelMode m{};
            m.fmt = old_a & 7;
            ch_mode_[kchan] = m;
            break;
        }
        case R_SETPAGE: {
            Channel ch = (Channel)(w.reg_addr & 3);
            buffer_.set_page(ch, old_a & 0xFFFFFF);
            break;
        }
        case R_ADDR: {
            Channel ch = (Channel)(w.reg_addr & 3);
            uint32_t x_int = A_ & 0xFFF, y_int = (A_ >> 12) & 0xFFF;
            uint32_t base   = (ch == CH_SOURCE) ? r_[R_SBASE] : r_[R_DBASE];
            uint32_t stride = (ch == CH_SOURCE) ? r_[R_SSTRIDE] : r_[R_DSTRIDE];
            uint32_t addr; uint8_t pos;
            AddrPacker::compute(x_int, y_int, base, stride,
                                (PixFmt)ch_mode_[ch].fmt, addr, pos);
            ADDR_ = addr; ADDR_pos_ = pos; ADDR_ch_ = (uint8_t)ch; ADDR_valid_ = true;
            pop(); pop();  // A (x), B (y)
            break;
        }
        case R_IRQ: irq_pending_ = true; irq_code_ = old_a & 0xFFF; break;
        }
    }

    if (do_alu) {
        R_ = exec_alu((AluOp)w.alu_op);
        if (w.alu_op != A_CMP) A_ = R_;
    }

    if (do_ctrl) {
        pc_ = next_pc;
        exec_ctrl((CtrlOp)w.ctrl_op, w.imm);
        return 1;
    }

    pc_ = next_pc;
    return 1;
}

int GPU::tick(int cpu_cycles) {
    if (cpu_cycles <= 0) return 0;
    int gpu_cycles = (int)((int64_t)cpu_cycles * clk_num_ / clk_den_);
    if (gpu_cycles <= 0) gpu_cycles = 1;
    int done = 0;
    while (done < gpu_cycles && state_ == RUN) {
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