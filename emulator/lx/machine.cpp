// machine.cpp - Aleste LX (modo nativo), ver machine.h
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Los registros y su comportamiento siguen el RTL del core AlesteLX_MiSTer:
//   MMU          rtl/core/mmu_native.sv
//   PIC, timer   rtl/core/lx/lx_sys.sv        reloj  rtl/core/cpc/rtc146818.sv
//   paleta       rtl/core/color_palette.sv    CRTC   rtl/core/mc6845mod.sv
//   DMA          rtl/core/lx/lx_dma.sv        MS2    rtl/core/lx/lx_ms2.sv
//   teclado      rtl/core/lx/lx_input.sv      765    rtl/core/cpc/u765.sv
#include "machine.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <algorithm>

namespace lx {
#include "kbdmatrix.inc"
}

namespace lx {

// ---------------------------------------------------------------- z80ex
static Z80EX_BYTE cb_mread(Z80EX_CONTEXT*, Z80EX_WORD a, int m1, void* u)   { return ((Machine*)u)->mread(a, m1); }
static void       cb_mwrite(Z80EX_CONTEXT*, Z80EX_WORD a, Z80EX_BYTE v, void* u) { ((Machine*)u)->mwrite(a, v); }
static Z80EX_BYTE cb_pread(Z80EX_CONTEXT*, Z80EX_WORD p, void* u)           { return ((Machine*)u)->pread(p); }
static void       cb_pwrite(Z80EX_CONTEXT*, Z80EX_WORD p, Z80EX_BYTE v, void* u) { ((Machine*)u)->pwrite(p, v); }
static Z80EX_BYTE cb_intread(Z80EX_CONTEXT*, void* u)                       { return ((Machine*)u)->intread(); }

Machine::Machine() : mem(MEM_SIZE, 0), fb(SCREEN_W * SCREEN_H, 0xFF000000) {
    cpu = z80ex_create(cb_mread, this, cb_mwrite, this, cb_pread, this, cb_pwrite, this, cb_intread, this);
    reset();
}

Machine::~Machine() { if (cpu) z80ex_destroy(cpu); }

bool Machine::load_raw(const std::string& path, uint32_t phys) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    size_t n = fread(&mem[phys], 1, MEM_SIZE - phys, f);
    fclose(f);
    return n > 0;
}

// Como el cargador del core: los primeros 64 KB en C00000 y el resto en 400000
bool Machine::load_boot_rom(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    std::vector<uint8_t> d(1 << 20);
    size_t n = fread(d.data(), 1, d.size(), f);
    fclose(f);
    if (n == 0) return false;
    size_t a = std::min<size_t>(n, 0x10000);
    memcpy(&mem[ROM_BASE], d.data(), a);
    if (n > 0x10000) memcpy(&mem[ROM2_BASE], d.data() + 0x10000, n - 0x10000);
    return true;
}

void Machine::reset() {
    mmu_control = 0x13; mmu_page = 0; mmu_clock = 2;
    slot_super = 0xFF; slot_user = 0xFF;
    memset(bank, 0, sizeof(bank));
    supervisor = true; exit_pending = false;
    pic_mask = pic_pending = 0; pic_vector = 0; irq_lines = irq_prev = 0;
    tmr_ctrl = 0; tmr_reload = tmr_count = tmr_ticks = 0;
    rtc_ram[10] = 0x26; rtc_ram[11] = 0x06;             // como el core: binario, 24 h
    pal_index = pal_ctrl = pal_mod = 0; border = 0x888;
    memset(crtc, 0, sizeof(crtc));
    crtc[0] = 63; crtc[1] = 40; crtc[6] = 25; crtc[9] = 7;
    memset(dma_reg, 0, sizeof(dma_reg)); dma_ctrl = 0; dma_done = false;
    for (auto& c : ch) c = Ms2Ch();
    ms2_ctrl = 0; ms2_vol_l = ms2_vol_r = 255; ms2_irq_ctrl = ms2_irq_stat = 0;
    ms2_tmr_period = ms2_tmr_cnt = 0;
    kq.clear(); kq_ovf = false; kin_ctrl = 0;
    memset(kmatrix, 0, sizeof(kmatrix));
    fdc.reset();
    leg_reset();
    leg_since = 0; leg_key = false;
    stop_reason.clear();
    z80ex_reset(cpu);
}

// ---------------------------------------------------------------- MMU
uint8_t Machine::cur_slot(int page) const {
    uint8_t s = supervisor ? slot_super : slot_user;
    return (s >> (page * 2)) & 3;
}

uint32_t Machine::translate(uint16_t addr) const {
    int page = addr >> 14;
    uint8_t slot = cur_slot(page);
    return ((uint32_t)slot << 22) | ((uint32_t)bank[slot * 4 + page] << 14) | (addr & 0x3FFF);
}

uint8_t Machine::mread(uint16_t addr, int m1) {
    if (legacy()) return mem[leg_translate(addr, false)];
    if (m1) {
        if (exit_pending) { supervisor = false; exit_pending = false; }
        if (!supervisor && (mmu_control & 0x04) && (addr == 0x0038 || addr == 0x0066)) supervisor = true;
    }
    uint32_t p = translate(addr);
    if (p >= 0xFF0000) return 0xFF;                     // MMIO no se ve como memoria
    return mem[p];
}

void Machine::mwrite(uint16_t addr, uint8_t v) {
    if (legacy()) { mem[leg_translate(addr, true)] = v; return; }
    uint32_t p = translate(addr);
    if (p < 0xFF0000) mem[p] = v;
}

uint8_t Machine::mmu_read(uint8_t r, uint16_t) {
    switch (r) {
    case 0xF0: return mmu_control;
    case 0xF1: return mmu_page;
    case 0xF3: return mmu_clock;
    case 0xFA: return slot_super;
    case 0xFB: return slot_user;
    case 0xFC: case 0xFD: case 0xFE: case 0xFF: { int pg = r & 3; return bank[cur_slot(pg) * 4 + pg]; }
    default:
        if (r >= 0xE0 && r <= 0xEF) return bank[r & 15];
        return 0xFF;
    }
}

void Machine::mmu_write(uint8_t r, uint8_t v) {
    switch (r) {
    case 0xF0:
        mmu_control = v;
        if (!(v & 1) && supervisor) exit_pending = true;  // sale en el siguiente M1
        break;
    case 0xF1: mmu_page = v & 0x7F; break;
    case 0xF2: supervisor = true; break;                  // llamada al sistema
    case 0xF3: mmu_clock = v; break;
    case 0xFA: slot_super = v; break;
    case 0xFB: slot_user = v; break;
    case 0xFC: case 0xFD: case 0xFE: case 0xFF: { int pg = r & 3; bank[cur_slot(pg) * 4 + pg] = v; break; }
    default:
        if (r >= 0xE0 && r <= 0xEF) bank[r & 15] = v;
    }
}

uint8_t Machine::pread(uint16_t port) {
    if (legacy()) return leg_pread(port);
    uint8_t p = port & 0xFF;
    if (!supervisor && (mmu_control & 0x10)) return 0xFF;
    if (p < 0x80) return mmio_read(0xFF0000 + mmu_page * 128 + p);
    if (p >= 0xE0) return mmu_read(p, port);
    return 0xFF;
}

void Machine::pwrite(uint16_t port, uint8_t v) {
    if (legacy()) { leg_pwrite(port, v); return; }
    uint8_t p = port & 0xFF;
    if (p == 0xF2) { supervisor = true; return; }
    if (!supervisor && (mmu_control & 0x10)) return;
    if (p < 0x80) mmio_write(0xFF0000 + mmu_page * 128 + p, v);
    else if (p >= 0xE0) mmu_write(p, v);
}

// ---------------------------------------------------------------- PIC
void Machine::set_line(int n, bool level) {
    uint16_t b = 1u << n;
    if (level) irq_lines |= b; else irq_lines &= ~b;
    if ((irq_lines & ~irq_prev) & b) pic_pending |= b;     // flanco de subida
    irq_prev = irq_lines;
}

void Machine::pic_update() {
    set_line(1, dma_done && (dma_ctrl & 0x08));
    set_line(3, ms2_irq_stat != 0);
    set_line(7, (kin_ctrl & 1) && !kq.empty());
}

uint8_t Machine::intread() {
    uint16_t a = pic_pending & pic_mask;
    int top = 0;
    for (int i = 0; i < 16; i++) if (a & (1 << i)) top = i;
    return pic_vector + 2 * top;
}

void Machine::timer_us(int n) {
    while (n--) {
        if (++ms_acc >= 1000) { ms_acc = 0; tmr_ticks++; }
        if (tmr_ctrl & 1) {
            if (tmr_count == 1) {
                pic_pending |= 1u << 6;
                if (tmr_ctrl & 2) tmr_count = tmr_reload; else { tmr_count = 0; tmr_ctrl &= ~1; }
            } else tmr_count--;
        }
    }
}

// ---------------------------------------------------------------- reloj
static uint8_t to_bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static int from_bcd(uint8_t v) { return (v >> 4) * 10 + (v & 15); }

uint8_t Machine::rtc_read(uint8_t r) {
    int64_t now = (int64_t)time(nullptr);
    if (now != rtc_last_sec) { rtc_last_sec = now; rtc_uf = true; }  // una actualizacion por segundo
    if (r >= 10) {
        if (r == 10) return rtc_ram[10] & 0x7F;            // sin UIP
        if (r == 12) { uint8_t v = rtc_uf ? 0x10 : 0; rtc_uf = false; return v; }   // UF, se borra al leer
        if (r == 13) return 0x80;                           // VRT: pila bien
        return rtc_ram[r];
    }
    time_t t = time(nullptr) + rtc_offset;
    struct tm tm = *localtime(&t);
    int v;
    switch (r) {
    case 0: v = tm.tm_sec; break;
    case 2: v = tm.tm_min; break;
    case 4: v = tm.tm_hour; break;
    case 6: v = tm.tm_wday + 1; break;
    case 7: v = tm.tm_mday; break;
    case 8: v = tm.tm_mon + 1; break;
    case 9: v = tm.tm_year + 1900 - 1980; break;        // anos desde 1980 (SETUP del 520EX, core)
    default: return rtc_ram[r];                             // alarmas
    }
    return (rtc_ram[11] & 0x04) ? (uint8_t)v : to_bcd(v);
}

void Machine::rtc_write(uint8_t r, uint8_t v) {
    if (r >= 10 || r == 1 || r == 3 || r == 5) {
        if (r >= 10 && rtc_ram[r] != v) cmos_dirty = true;
        rtc_ram[r] = v;
        return;
    }
    int val = (rtc_ram[11] & 0x04) ? v : from_bcd(v);
    time_t now = time(nullptr);
    time_t t = now + rtc_offset;
    struct tm tm = *localtime(&t);
    switch (r) {
    case 0: tm.tm_sec = val; break;
    case 2: tm.tm_min = val; break;
    case 4: tm.tm_hour = val; break;
    case 6: return;
    case 7: tm.tm_mday = val; break;
    case 8: tm.tm_mon = val - 1; break;
    case 9: tm.tm_year = 80 + val; break;
    }
    tm.tm_isdst = -1;
    rtc_offset = (int64_t)mktime(&tm) - (int64_t)now;
}

bool Machine::load_cmos(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    uint8_t buf[64];
    size_t n = fread(buf, 1, 64, f);
    fclose(f);
    if (n < 64) return false;
    for (int i = 10; i < 64; i++) rtc_ram[i] = buf[i];      // la hora es la del PC
    cmos_dirty = false;
    return true;
}

bool Machine::save_cmos(const std::string& path) const {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    uint8_t buf[64] = {};                                   // 0-9: la hora no se guarda
    for (int i = 10; i < 64; i++) buf[i] = rtc_ram[i];
    fwrite(buf, 1, 64, f);
    fclose(f);
    return true;
}

// ---------------------------------------------------------------- DMA
void Machine::dma_start(uint8_t c) {
    auto r24 = [&](int i) { return (uint32_t)dma_reg[i] | ((uint32_t)dma_reg[i + 1] << 8) | ((uint32_t)dma_reg[i + 2] << 16); };
    auto r16 = [&](int i) { return (uint16_t)(dma_reg[i] | (dma_reg[i + 1] << 8)); };
    uint32_t src = r24(2), dst = r24(5);
    uint32_t w = r16(8) ? r16(8) : 0x10000;
    uint32_t h = r16(10) ? r16(10) : 1;
    int32_t ss = (int16_t)r16(12), ds = (int16_t)r16(14);
    uint8_t key = dma_reg[0x10], mode = (c >> 1) & 3;
    auto rd = [&](uint32_t a) -> uint8_t { a &= 0xFFFFFF; return a < 0xFF0000 ? mem[a] : 0xFF; };
    auto wr = [&](uint32_t a, uint8_t v) { a &= 0xFFFFFF; if (a < 0xFF0000) mem[a] = v; };
    for (uint32_t y = 0; y < h; y++) {
        uint32_t s = src + y * ss, d = dst + y * ds;
        for (uint32_t x = 0; x < w; x++) {
            if (mode == 1) wr(d + x, key);
            else {
                uint8_t v = rd(s + x);
                if (mode != 2 || v != key) wr(d + x, v);
            }
        }
    }
    dma_ctrl = c & 0x0F;
    if (c & 0x08) dma_done = true;
    // el tiempo: dos accesos por byte a ~50 ns, para quien mida
    cycles += 0;
}

// ---------------------------------------------------------------- Magic Sound 2
uint8_t Machine::ms2_read(uint8_t r) {
    if (r >= 0x20 && r < 0x60) {
        Ms2Ch& c = ch[(r >> 4) - 2];
        switch (r & 15) {
        case 0x0: return c.ctrl & 0x0F;
        case 0x1: return c.start; case 0x2: return c.start >> 8; case 0x3: return c.start >> 16;
        case 0x4: return c.end;   case 0x5: return c.end >> 8;   case 0x6: return c.end >> 16;
        case 0x7: return c.loop;  case 0x8: return c.loop >> 8;  case 0x9: return c.loop >> 16;
        case 0xA: return c.vol_l; case 0xB: return c.vol_r;
        case 0xC: return c.step;  case 0xD: return c.step >> 8;  case 0xE: return c.step >> 16;
        case 0xF: return ((c.pos >> 36) & 0xF0) | (c.eof ? 2 : 0) | (c.ctrl & 1);
        }
    }
    switch (r) {
    case 0x00: return ms2_ctrl;
    case 0x01: return ms2_vol_l;
    case 0x02: return ms2_vol_r;
    case 0x03: {
        uint8_t v = 0;
        for (int i = 0; i < 4; i++) { if (ch[i].ctrl & 1) v |= 0x10 << i; if (ch[i].eof) v |= 1 << i; }
        return v;
    }
    case 0x04: return ms2_irq_ctrl;
    case 0x05: return ms2_irq_stat;
    case 0x10: return ms2_tmr_period;
    case 0x11: return ms2_tmr_period >> 8;
    }
    return 0xFF;
}


void Machine::ms2_write(uint8_t r, uint8_t v) {
    if (r >= 0x20 && r < 0x60) {
        Ms2Ch& c = ch[(r >> 4) - 2];
        switch (r & 15) {
        case 0x0: {
            bool start = (v & 1) && !(c.ctrl & 1);
            c.ctrl = v & 0x0F;
            if (start) {
                c.pos = (uint64_t)c.start << 16;
                c.eof = false;
                uint32_t a = c.start;
                c.smp = (c.ctrl & 4) ? (int16_t)(mem[a & 0xFFFFFF] | (mem[(a + 1) & 0xFFFFFF] << 8))
                                     : (int16_t)(mem[a & 0xFFFFFF] << 8);
            }
            if (!(v & 1)) c.smp = 0;
            break;
        }
        case 0x1: case 0x4: case 0x7: case 0xC: c.tmp = (c.tmp & 0xFF00) | v; break;
        case 0x2: case 0x5: case 0x8: case 0xD: c.tmp = (c.tmp & 0x00FF) | (v << 8); break;
        case 0x3: c.start = (v << 16) | c.tmp; break;
        case 0x6: c.end   = (v << 16) | c.tmp; break;
        case 0x9: c.loop  = (v << 16) | c.tmp; break;
        case 0xA: c.vol_l = v; break;
        case 0xB: c.vol_r = v; break;
        case 0xE: c.step  = (v << 16) | c.tmp; break;
        }
        return;
    }
    switch (r) {
    case 0x00: ms2_ctrl = v; break;
    case 0x01: ms2_vol_l = v; break;
    case 0x02: ms2_vol_r = v; break;
    case 0x03: for (int i = 0; i < 4; i++) if (v & (1 << i)) ch[i].eof = false; break;
    case 0x04: ms2_irq_ctrl = v; break;
    case 0x05: ms2_irq_stat &= ~v; break;
    case 0x10: ms2_tmr_period = (ms2_tmr_period & 0xFF00) | v; break;
    case 0x11: ms2_tmr_period = (ms2_tmr_period & 0x00FF) | (v << 8); ms2_tmr_cnt = ms2_tmr_period; break;
    }
}

// Una muestra de salida (48 kHz): mezcla lo actual, avanza y lee las nuevas
void Machine::ms2_tick() {
    int32_t acc_l = 0, acc_r = 0;
    for (auto& c : ch) { acc_l += c.smp * c.vol_l; acc_r += c.smp * c.vol_r; }
    int32_t out_l = 0, out_r = 0;
    if (ms2_ctrl & 1) {
        out_l = ((acc_l >> 10) * ms2_vol_l) >> 8;
        out_r = ((acc_r >> 10) * ms2_vol_r) >> 8;
    }
    // como en el core: la MS2 entra a la mitad en la mezcla, con el AY
    int ay_l = 0, ay_r = 0;
    if (legacy()) ay_sample(ay_l, ay_r);
    audio_buf.push_back((int16_t)std::clamp((out_l >> 1) + ay_l, -32768, 32767));
    audio_buf.push_back((int16_t)std::clamp((out_r >> 1) + ay_r, -32768, 32767));

    if (ms2_tmr_period) {
        if (ms2_tmr_cnt <= 1) { ms2_tmr_cnt = ms2_tmr_period; if (ms2_irq_ctrl & 2) ms2_irq_stat |= 2; }
        else ms2_tmr_cnt--;
    }
    if (!(ms2_ctrl & 1)) return;
    for (auto& c : ch) {
        if (!(c.ctrl & 1)) continue;
        uint64_t np = c.pos + c.step;
        uint32_t ip = (uint32_t)(np >> 16);
        if (ip >= c.end) {
            c.eof = true;
            if ((c.ctrl & 8) && (ms2_irq_ctrl & 1)) ms2_irq_stat |= 1;
            if ((c.ctrl & 2) && c.loop < c.end) np = ((uint64_t)(c.loop + (ip - c.end)) << 16) | (np & 0xFFFF);
            else { c.ctrl &= ~1; c.smp = 0; continue; }
        }
        c.pos = np;
        uint32_t a = (uint32_t)(c.pos >> 16) & 0xFFFFFF;
        c.smp = (c.ctrl & 4) ? (int16_t)(mem[a] | (mem[(a + 1) & 0xFFFFFF] << 8)) : (int16_t)(mem[a] << 8);
    }
}

// ---------------------------------------------------------------- teclado
// Cola de eventos (lo que lee la BIOS) y matriz del 520EX/CPC (para juegos),
// como lx_input.sv y kbd_520ex.sv
void Machine::key_event(uint8_t code, bool pressed, bool extended) {
    for (auto& k : kbd_matrix_keys)
        if (k.code == code && k.ext == (extended ? 1 : 0)) {
            if (pressed) kmatrix[k.row] |= 1 << k.bit; else kmatrix[k.row] &= ~(1 << k.bit);
            if (k.fshift) kfshift = pressed;
        }
    if (pressed) leg_key = true;
    if (kq.size() >= 16) { kq_ovf = true; return; }
    kq.push_back({code, pressed, extended});
}

// ---------------------------------------------------------------- MMIO
uint8_t Machine::mmio_read(uint32_t a) {
    uint16_t off = a & 0xFFFF;
    uint8_t page = off >> 7, r = off & 0x7F;
    switch (page) {
    case 0:
        if (r < 0x20) {
            switch (r) {
            case 0: return pic_mask; case 1: return pic_mask >> 8; case 2: return pic_vector;
            case 3: return pic_pending & pic_mask; case 4: return (pic_pending & pic_mask) >> 8;
            case 5: return pic_pending; case 6: return pic_pending >> 8;
            }
            return 0xFF;
        }
        if (r >= 0x40 && r < 0x60) {
            switch (r) {
            case 0x40: return tmr_ctrl; case 0x41: return tmr_reload; case 0x42: return tmr_reload >> 8;
            case 0x43: tmr_cnt_hi_l = tmr_count >> 8; return tmr_count;
            case 0x44: return tmr_cnt_hi_l;
            case 0x45: tmr_ticks_hi_l = tmr_ticks >> 8; return tmr_ticks;
            case 0x46: return tmr_ticks_hi_l;
            }
            return 0xFF;
        }
        if (r == 0x60) return rtc_idx;
        if (r == 0x61) return rtc_read(rtc_idx);
        return 0xFF;
    case 2:
        switch (r) {
        case 0x00: return pal_index;
        case 0x01: return palette[pal_index];
        case 0x02: return palette[pal_index] >> 8;
        case 0x03: return border; case 0x04: return border >> 8;
        case 0x05: return pal_ctrl; case 0x06: return pal_mod;
        case 0x20: return crtc_idx;
        case 0x21: return crtc[crtc_idx & 31];
        }
        return 0xFF;
    case 6:
        if (r == 0x00) return dma_ctrl & 0x0E;                 // nunca ocupada: se hace al momento
        if (r == 0x01) return dma_done;
        if (r <= 0x10) return dma_reg[r];
        return 0xFF;
    case 7: return ms2_read(r);
    case 8:
        switch (r) {
        case 0x00: { uint8_t v = (kq_ovf ? 2 : 0) | (kq.empty() ? 0 : 1); kq_ovf = false; return v; }
        case 0x01: return kq.empty() ? 0 : ((kq.front().extended ? 2 : 0) | (kq.front().pressed ? 1 : 0));
        case 0x02: { if (kq.empty()) return 0; uint8_t c = kq.front().code; kq.pop_front(); return c; }
        case 0x03: return kin_ctrl;
        case 0x04: return layout_es ? 1 : 0;
        }
        if (r >= 0x10 && r <= 0x1A) {                          // matriz, activo a 0 (cols2)
            uint8_t k = kmatrix[r - 0x10];
            if (r == 0x19) k |= joy;
            return ~k;
        }
        return 0xFF;
    case 9:
        if (r == 0x00) return fdc.read_status();
        if (r == 0x01) return fdc.read_data();
        return 0xFF;
    }
    return 0xFF;
}

void Machine::mmio_write(uint32_t a, uint8_t v) {
    uint16_t off = a & 0xFFFF;
    uint8_t page = off >> 7, r = off & 0x7F;
    switch (page) {
    case 0:
        if (r < 0x20) {
            switch (r) {
            case 0: pic_mask = (pic_mask & 0xFF00) | v; break;
            case 1: pic_mask = (pic_mask & 0x00FF) | (v << 8); break;
            case 2: pic_vector = v; break;
            case 7: pic_pending &= ~v; break;
            case 8: pic_pending &= ~(v << 8); break;
            }
        } else if (r >= 0x40 && r < 0x60) {
            switch (r) {
            case 0x40: if ((v & 1) && !(tmr_ctrl & 1)) tmr_count = tmr_reload; tmr_ctrl = v; break;
            case 0x41: tmr_reload = (tmr_reload & 0xFF00) | v; break;
            case 0x42: tmr_reload = (tmr_reload & 0x00FF) | (v << 8); break;
            }
        } else if (r == 0x60) rtc_idx = v & 63;
        else if (r == 0x61) rtc_write(rtc_idx, v);
        break;
    case 2:
        switch (r) {
        case 0x00: pal_index = v; break;
        case 0x01: pal_lo = v; break;
        case 0x02:
            palette[pal_index] = ((v & 0x0F) << 8) | pal_lo;
            if (pal_ctrl & 0x20) pal_index++;
            break;
        case 0x03: border = (border & 0xF00) | v; break;
        case 0x04: border = (border & 0x0FF) | ((v & 0x0F) << 8); break;
        case 0x05: pal_ctrl = v; break;
        case 0x06: pal_mod = v; break;
        case 0x20: crtc_idx = v; break;
        case 0x21: crtc[crtc_idx & 31] = v; break;
        }
        break;
    case 6:
        if (r == 0x00) { if (v & 1) dma_start(v); else dma_ctrl = v & 0x0F; }
        else if (r == 0x01) { if (v & 1) dma_done = false; }
        else if (r <= 0x10) dma_reg[r] = v;
        break;
    case 7: ms2_write(r, v); break;
    case 8: if (r == 0x03) kin_ctrl = v; break;
    case 9:
        if (r == 0x01) fdc.write_data(v);
        else if (r == 0x02) fdc.motor(v & 1);
        break;
    }
}

// ---------------------------------------------------------------- video
uint32_t Machine::rgb12(uint16_t c) const {
    uint32_t r = (c >> 8) & 15, g = (c >> 4) & 15, b = c & 15;
    return 0xFF000000 | (r * 17 << 16) | (g * 17 << 8) | (b * 17);
}

// Modos lineales del CRTC del LX: R18 bits 1-0 = bits por pixel (00 1, 01 2,
// 10 4), R19 = byte alto de la direccion, R12/R13 = inicio (en palabras),
// R1 = bytes por linea / 2, R6 * (R9 + 1) = lineas
void Machine::render() {
    if (legacy()) return;                                       // legacy: leg_fb, al acabar cada cuadro
    uint32_t bc = rgb12(border);
    std::fill(fb.begin(), fb.end(), bc);
    int bpp_sel = crtc[0x12] & 3;
    int bpp = bpp_sel == 0 ? 1 : bpp_sel == 1 ? 2 : 4;
    uint32_t stride = crtc[1] * 2;
    int lines = crtc[6] * ((crtc[9] & 31) + 1);
    if (!stride || !lines) return;
    uint32_t base = ((uint32_t)crtc[0x13] << 16) + ((((crtc[12] & 0x3F) << 8) | crtc[13]) << 1);
    int width = stride * 8 / bpp;
    int sx = std::max(1, SCREEN_W / width);
    int sy = std::max(1, SCREEN_H / std::max(1, lines));
    int ox = (SCREEN_W - width * sx) / 2, oy = (SCREEN_H - lines * sy) / 2;
    for (int y = 0; y < lines && y * sy < SCREEN_H; y++) {
        const uint8_t* row = &mem[(base + y * stride) & 0xFFFFFF];
        for (int x = 0; x < width; x++) {
            int bit = x * bpp;
            uint8_t byte = row[bit >> 3];
            int idx = (byte >> (8 - bpp - (bit & 7))) & ((1 << bpp) - 1);
            uint32_t c = rgb12(palette[idx]);
            for (int yy = 0; yy < sy; yy++) {
                uint32_t* p = &fb[(oy + y * sy + yy) * SCREEN_W + ox + x * sx];
                for (int xx = 0; xx < sx; xx++) p[xx] = c;
            }
        }
    }
}

// ---------------------------------------------------------------- bucle
int Machine::step() {
    double hz = cpu_hz();
    int t = 0;
    if (legacy() && !was_legacy) { leg_since = 0; leg_key = false; }   // entrada en el 520EX
    was_legacy = legacy();
    if (legacy()) {
        // la INT es la del Gate Array; al reconocerla se borra y se quita el
        // bit 5 del contador (como el 40010)
        if (ms.nmi && !z80ex_last_op_type(cpu) && z80ex_nmi_possible(cpu)) { ms.nmi = false; t = z80ex_nmi(cpu); }
        else if (ga_irq && z80ex_int_possible(cpu) && (t = z80ex_int(cpu)) > 0) { ga_irq = false; irq_cnt &= 0x1F; leg_irqs++; }
        else t = z80ex_step(cpu);
        if (t <= 0) t = 4;
        leg_time(t);
        ms_time(t);
        serial_time(t);
        tape.run(t, 4e6, ppi_c & 0x10);
        leg_since += t;
    } else {
        pic_update();
        if ((pic_pending & pic_mask) && z80ex_int_possible(cpu)) t = z80ex_int(cpu);
        else t = z80ex_step(cpu);
        if (t <= 0) t = 4;
    }
    cycles += t;
    frame_acc += t;
    us_acc += t;
    double us_cycles = hz / 1e6, smp_cycles = hz / AUDIO_RATE;
    if (us_acc >= us_cycles) { int n = (int)(us_acc / us_cycles); us_acc -= n * us_cycles; timer_us(n); }
    sample_acc += t;
    while (sample_acc >= smp_cycles) { sample_acc -= smp_cycles; ms2_tick(); }
    return t;
}

bool Machine::run_frame() {
    break_hit = false;
    if (!stop_reason.empty()) return false;
    double frame_cycles = cpu_hz() / 60.0;
    while (frame_acc < frame_cycles) {
        // z80ex trata los prefijos como pasos sueltos: el depurador solo mira
        // el principio de cada instruccion
        if (break_check && !z80ex_last_op_type(cpu) && break_check(z80ex_get_reg(cpu, regPC))) {
            break_hit = true;
            return true;
        }
        step();
    }
    frame_acc -= frame_cycles;
    pic_pending |= 1u << 2;                                      // VBlank
    return true;
}

} // namespace lx
