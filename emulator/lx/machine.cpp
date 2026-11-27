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
#include "gpu_boot_rom.h"
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
    gpu.attach_memory(mem.data(), MEM_SIZE);         // la GPU ve la misma memoria fisica (16 MB)
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
    has_rom520 = n > 0x10000;
    return true;
}

// La ROM del 520EX en un fichero aparte (AL512.BIN, 64 KB): donde la pondria
// el cargador del core si fuera detras de LX.ROM en boot.rom
bool Machine::load_rom520(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    size_t n = fread(&mem[ROM2_BASE], 1, 0x10000, f);
    fclose(f);
    if (n == 0) return false;
    has_rom520 = true;
    return true;
}

void Machine::reset() {
    mmu_control = 0x13; mmu_page = 0; mmu_clock = 2;
    slot_super = 0xFF; slot_user = 0xFF;
    memset(bank, 0, sizeof(bank));
    supervisor = true; exit_pending = false; int_hold = false;
    pic_pending = pic_enable = pic_repeat = pic_insvc = 0; pic_vector = 0xFF; irq_lines = irq_prev = 0;
    for (auto& q : pic_prio) q = 0;
    gpu.reset();                                        // LX-GPU parada; el cargador en ROM 0-31
    for (int i = 0; i < 32; i++) gpu.write_rom(i, GPU_BOOT_ROM[i]);
    rt = rt_snap_sup = rt_snap_usr = tmr_cmp = 0; tmr_buf_lo = 0; tmr_win_us = 0;
    tmr_window = tmr_armed = tmr_en = tmr_fired = tmr_due_q = false;
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
    if (legacy()) {
        // al pasar a legacy (el 520EX), como en mmu_native.sv: la salida
        // pendiente se completa en el M1 siguiente
        if (m1 && exit_pending) { supervisor = false; exit_pending = false; int_hold = false; }
        return mem[leg_translate(addr, false)];
    }
    // Como mmu_native.sv. La entrada por una trampa (M1 de 0038h/0066h, bit 2)
    // se registra en el M1, cuando la direccion de ese codigo de operacion ya ha
    // salido: el codigo de operacion se lee con el mapa anterior; sus operandos,
    // la pila y lo que sigue, con el nuevo.
    uint32_t p = translate(addr);
    if (m1) {
        bool trap = (mmu_control & 0x04) && (addr == 0x0038 || addr == 0x0066);
        if (!supervisor && trap) { supervisor = true; exit_pending = false; }
    }
    uint8_t v = p >= 0xFF0000 ? 0xFF : mem[p];          // MMIO no se ve como memoria
    // Salida del supervisor en el RET (propuesta de hww): OUT (F0h) con el bit
    // 0 a 0 solo la arma; sale en el M1 que lee un C9. Ese codigo de operacion
    // se ha leido con el mapa del supervisor; la pila del RET (la vuelta), ya
    // con el del usuario. Entre el OUT y el RET, interrupciones desactivadas.
    if (m1 && supervisor && exit_pending && v == 0xC9) { supervisor = false; exit_pending = false; int_hold = false; }
    return v;
}

void Machine::mwrite(uint16_t addr, uint8_t v) {
    if (legacy()) { mem[leg_translate(addr, true)] = v; return; }
    uint32_t p = translate(addr);
    if (p < 0xFF0000) mem[p] = v;
}

uint8_t Machine::mmu_read(uint8_t r, uint16_t) {
    switch (r) {
    case 0xF0: return (mmu_control & ~0x08) | (int_hold ? 0x08 : 0);   // bit 3: el bloqueo
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
        if (!(v & 1)) {
            if (supervisor) exit_pending = true;          // arma la salida (sale en el RET)
            int_hold = (v & 0x02) != 0;                   // bloqueo hasta el RET (no al pasar a legacy)
        } else
            int_hold = (v & 0x08) != 0;                   // el nucleo lo quita con el bit 3 a 0
        break;
    case 0xF1: mmu_page = v & 0x7F; break;
    case 0xF2: if (!supervisor) int_hold = true; supervisor = true; break;  // llamada al sistema
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
    if (p >= 0xF4 && p <= 0xF7) return tmr_read(p);     // temporizador: tambien con el bloqueo
    if (!supervisor && (mmu_control & 0x10)) return 0xFF;
    if (p < 0x80) return mmio_read(0xFF0000 + mmu_page * 128 + p);
    if (p >= 0xE0) return mmu_read(p, port);
    return 0xFF;
}

void Machine::pwrite(uint16_t port, uint8_t v) {
    if (legacy()) { leg_pwrite(port, v); return; }
    uint8_t p = port & 0xFF;
    // llamada al sistema: antes del bloqueo de puertos del usuario, como en
    // mmu_native.sv (OUT (F2h),A entra al supervisor aunque esten bloqueados)
    if (p == 0xF2) { if (!supervisor) int_hold = true; supervisor = true; return; }  // con bloqueo
    if (p >= 0xF4 && p <= 0xF7) { tmr_write(p, v); return; }   // solo el supervisor
    if (!supervisor && (mmu_control & 0x10)) return;
    if (p < 0x80) mmio_write(0xFF0000 + mmu_page * 128 + p, v);
    else if (p >= 0xE0) mmu_write(p, v);
}

// ---------------------------------------------------------------- PIC v2
void Machine::pic_raise(uint16_t b) {
    pic_repeat |= pic_pending & b;                         // otro flanco sin atender
    pic_pending |= b;
}

void Machine::set_line(int n, bool level) {
    uint16_t b = 1u << n;
    if (level) irq_lines |= b; else irq_lines &= ~b;
    if ((irq_lines & ~irq_prev) & b) pic_raise(b);         // flanco de subida
    irq_prev = irq_lines;
}

void Machine::pic_update() {
    set_line(0, fdc.irq());           // INT de la disquetera (como u765.sv: busqueda, ejecucion, resultado)
    set_line(1, dma_done && (dma_ctrl & 0x08));
    set_line(3, ms2_irq_stat != 0);
    set_line(5, gpu.irq());           // LX-GPU (instruccion IRQ del microcodigo)
    set_line(7, (kin_ctrl & 1) && !kq.empty());
}

// CLAIM: la de mas prioridad (a igual, la de numero menor) pasa a servicio
uint8_t Machine::pic_claim() {
    uint16_t e = pic_eligible();
    int best = -1;
    for (int k = 0; k < 16; k++)
        if ((e >> k) & 1) if (best < 0 || pic_prio[k] > pic_prio[best]) best = k;
    if (best < 0) return 0x00;
    uint16_t b = 1u << best;
    uint8_t v = 0x80 | ((pic_repeat & b) ? 0x40 : 0) | best;
    pic_pending &= ~b; pic_repeat &= ~b; pic_insvc |= b;
    return v;
}

void Machine::pic_command(uint8_t v) {
    uint16_t b = 1u << (v & 15);
    if (v & 0x80) { pic_prio[v & 15] = (v >> 4) & 7; return; }
    switch ((v >> 4) & 7) {
    case 1: pic_insvc &= ~b; break;                       // COMPLETE
    case 2: pic_enable |= b; break;                       // ENABLE
    case 3: pic_enable &= ~b; break;                      // DISABLE
    case 4: pic_pending &= ~b; break;                     // CLEAR_PENDING
    case 5: pic_repeat &= ~b; break;                      // CLEAR_REPEAT
    case 6: pic_pending &= ~b; pic_repeat &= ~b; break;   // CLEAR_BOTH
    }
}

// el vector del acuse (IM 2; en IM 0, FFh = RST 38h)
uint8_t Machine::intread() { return pic_vector; }

// ---------------------------------------------------------------- temporizador rev 6
uint8_t Machine::tmr_read(uint8_t p) {
    uint16_t& snap = supervisor ? rt_snap_sup : rt_snap_usr;
    switch (p) {
    case 0xF4: snap = rt; return (uint8_t)snap;
    case 0xF5: return (uint8_t)(snap >> 8);
    case 0xF6: return (tmr_fired ? 0x10 : 0) | (tmr_due() ? 0x08 : 0) | (tmr_window ? 0x04 : 0)
                    | (tmr_armed ? 0x02 : 0) | (tmr_en ? 0x01 : 0);
    }
    return 0xFF;
}

void Machine::tmr_write(uint8_t p, uint8_t v) {
    if (!supervisor) return;
    switch (p) {
    case 0xF4: tmr_buf_lo = v; tmr_window = true; tmr_win_us = 0; break;
    case 0xF5:
        tmr_cmp = (uint16_t)(v << 8 | tmr_buf_lo);
        tmr_window = false; tmr_win_us = 0;
        tmr_armed = tmr_en = true; tmr_fired = false;
        if (tmr_due()) tmr_fire();                        // objetivo ya pasado: al armar
        tmr_due_q = tmr_due();
        break;
    case 0xF6:
        tmr_en = v & 1;
        if (tmr_en && tmr_due() && !tmr_fired) tmr_fire();
        break;
    }
}

void Machine::timer_us(int n) {
    while (n--) {
        rt++;
        if (tmr_window && ++tmr_win_us >= 32) { tmr_window = false; tmr_win_us = 0; }
        bool d = tmr_due();
        if (tmr_en && d && !tmr_due_q && !tmr_fired) tmr_fire();   // una vez por armado
        tmr_due_q = d;
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
            case 0x01: return pic_claim();
            case 0x02: return pic_vector;
            case 0x03: return pic_pending;      case 0x04: return pic_pending >> 8;
            case 0x05: return pic_repeat;       case 0x06: return pic_repeat >> 8;
            case 0x07: return pic_insvc;        case 0x08: return pic_insvc >> 8;
            case 0x09: return pic_enable;       case 0x0A: return pic_enable >> 8;
            case 0x0B: return pic_eligible();   case 0x0C: return pic_eligible() >> 8;
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
    case 3:                                             // LX-GPU: 0-2 GPU_ADDR, 3 COMMAND, 4 STATUS
        if (r < 8) return gpu.read(r);
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
            if (!supervisor) break;                       // COMMAND y VECTOR: solo el supervisor
            if (r == 0x00) pic_command(v);
            else if (r == 0x02) pic_vector = v;
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
    case 3:
        if (r < 8) gpu.write(r, v);
        break;
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

// Video nativo, como mc6845mod.sv + pixel_pipeline.sv:
//   R18 bits 1-0 = bits por pixel (00 1, 01 2, 10 4, 11 8)
//   R20 bit 0 = direccion lineal (R19:R12:R13, en bytes, seguida de linea en
//       linea); bit 1 paso de 4 bytes (si no, 2); bit 2 dos pasos por caracter
//   R21 bits 1-0 = bytes por caracter: 00 2, 01 4, 10 8, 11 16
//   cada caracter son 16 pixeles de la salida (720x480); cada linea del CRTC
//   se ve dos veces. R1 = caracteres por linea, R6 * (R9 + 1) = lineas.
//   Sin R20 bit 0, direcciones del CPC: R19, MA13-12, RA2-0, MA9-0, 0.
void Machine::render() {
    if (legacy()) return;                                       // legacy: leg_fb, al acabar cada cuadro
    uint32_t bc = rgb12(border);
    std::fill(fb.begin(), fb.end(), bc);
    int bpp = 1 << (crtc[0x12] & 3);
    int chars = crtc[1];
    int lines = crtc[6] * ((crtc[9] & 31) + 1);
    if (!chars || !lines) return;
    bool linear = crtc[0x14] & 1;
    int bytes_char = 2 << (crtc[0x15] & 3);
    int px_char = bytes_char * 8 / bpp;                         // pixeles por caracter
    if (px_char > 16) px_char = 16;                             // el pipeline no da mas de 16
    int width = chars * px_char;
    // la salida: 16 pixeles por caracter en 640 (40 caracteres); lineas dobladas
    double sx = 16.0 / px_char * (SCREEN_W / 640.0);
    int sy = 2;
    int out_w = (int)(width * sx), out_h = lines * sy;
    int ox = (SCREEN_W - out_w) / 2, oy = (SCREEN_H - out_h) / 2;
    uint32_t start = ((crtc[12] & 0x3F) << 8) | crtc[13];
    uint32_t lin_step = linear ? (uint32_t)((crtc[0x14] & 4 ? 2 : 1) * (crtc[0x14] & 2 ? 4 : 2)) : 2;
    for (int y = 0; y < lines; y++) {
        int row = y / ((crtc[9] & 31) + 1), ra = y % ((crtc[9] & 31) + 1);
        for (int c = 0; c < chars; c++) {
            uint32_t a;
            if (linear) a = ((uint32_t)crtc[0x13] << 16) + ((start + (uint32_t)(y * chars + c) * lin_step) & 0xFFFF);
            else {
                uint16_t ma = (uint16_t)(start + row * chars + c);
                a = ((uint32_t)crtc[0x13] << 16) | (((ma >> 12) & 3) << 14) | ((ra & 7) << 11) | ((ma & 0x3FF) << 1);
            }
            for (int p = 0; p < px_char; p++) {
                int bit = p * bpp;
                uint8_t byte = mem[(a + (bit >> 3)) & 0xFFFFFF];
                int idx = bpp == 8 ? byte : (byte >> (8 - bpp - (bit & 7))) & ((1 << bpp) - 1);
                uint32_t col = rgb12(palette[idx]);
                int x0 = ox + (int)((c * px_char + p) * sx), x1 = ox + (int)((c * px_char + p + 1) * sx);
                for (int yy = 0; yy < sy; yy++) {
                    int yo = oy + y * sy + yy;
                    if (yo < 0 || yo >= SCREEN_H) continue;
                    uint32_t* line = &fb[yo * SCREEN_W];
                    for (int x = std::max(0, x0); x < std::min(SCREEN_W, x1); x++) line[x] = col;
                }
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
        if (pic_eligible() && !int_hold && !tmr_window && z80ex_int_possible(cpu)) t = z80ex_int(cpu);
        else t = z80ex_step(cpu);
        if (t <= 0) t = 4;
        gpu.tick(t);                                    // GPU a 54 MHz: 3 ciclos por cada uno del Z80
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
    pic_raise(1u << 2);                                          // VBlank
    return true;
}

} // namespace lx
