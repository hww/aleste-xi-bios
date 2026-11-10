// legacy.cpp - Modo legacy del Aleste LX: Aleste 520EX / Amstrad CPC
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Sigue el RTL del core AlesteLX_MiSTer:
//   MMU legacy            rtl/core/mmu_legacy.sv
//   video (CRTC + GA)     rtl/core/cpc/cpc_video.sv (UM6845R tipo 0, GA40010)
//   E/S, PPI, AY, IRQ     rtl/core/aleste_lx_core.sv
//   cinta                 PB7 = senal, PC4 = motor (tape.cpp)
// La CPU va a 4 MHz; el CRTC avanza un caracter por microsegundo (0,6 us en
// la alta resolucion del 520EX, FABF D1).
#include "machine.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace lx {

void Machine::leg_reset() {
    leg_rmr = 0x80; leg_mmr = 0xC0; leg_upper = 0;
    for (int i = 0; i < 4; i++) leg_map[i] = i;
    extport = 0;
    crt_idx = 0; memset(crt, 0, sizeof(crt));
    crt_hc = crt_vlc = crt_vc = crt_adj = crt_hsc = crt_vsc = 0;
    crt_in_adj = crt_hs = crt_vs = false;
    crt_ma = crt_ma_row = 0;
    char_acc = 0;
    ga_pen = 0; memset(ga_ink, 0, sizeof(ga_ink)); ga_border = 0x14; ga_mode = 1;
    ga_hcnt = 0; ga_vcnt = 7; ga_hs = ga_vs = prev_crt_hs = prev_crt_vs = false;
    irq_cnt = 0; vs_wait = 0; ga_irq = false;
    ppi_a = 0; ppi_c = 0; ppi_ctrl = 0x9B; ay_addr = 0; memset(ay_reg, 0, sizeof(ay_reg));
    ay_reg[7] = 0xFF;
    for (int c = 0; c < 3; c++) { ay_tcnt[c] = 0; ay_tout[c] = false; }
    ay_ncnt = ay_ecnt = ay_estep = 0; ay_lfsr = 1; ay_acc = 0; ay_ehold = true; ay_eatt = false;
    leg_raw.assign(1024 * 320, 0xFF000000);
    leg_fb.assign(LEG_W * LEG_H, 0xFF000000);
    beam_x = beam_y = 0; beam_t = 0;
    ms = MagicSound();
    mouse = Mice();
}

// ---------------------------------------------------------------- ratones
void Machine::mouse_packet(int dx, int dy, uint8_t b) {
    mouse.buttons = b & 7;
    // serie: se acumula solo con el receptor del 8251 activado
    if (mouse.cmd & 4) { mouse.acc_x += dx; mouse.acc_y += dy; }
    // Kempston y AMX acumulan; SYMBiFACE II y Multiplay dan el ultimo paquete
    mouse.kx = (mouse.kx + dx) & 0xFFF;
    mouse.ky = (mouse.ky + dy) & 0xFFF;
    mouse.amx_dx += dx; mouse.amx_dy += dy;
    mouse.last_dx = dx; mouse.last_dy = dy;
    mouse.sf2_avail = (dy ? 4 : 0) | (dx ? 2 : 0) | 1;
    mouse.mp_avail = 3;
}

// 8251 (K580VV51): EExx datos, EFxx control / estado
uint8_t Machine::serial_read(bool a8) {
    if (a8) return 0x80 | (mouse.overrun ? 0x10 : 0) | 0x04 | (mouse.rx_rdy ? 0x02 : 0) | 0x01;  // DSR, TxEMPTY, RxRDY, TxRDY
    mouse.rx_rdy = false;
    return mouse.rx_data;
}

void Machine::serial_write(bool a8, uint8_t v) {
    if (!a8) return;
    if (v == 0x40) { mouse.expect_mode = true; mouse.cmd = 0; mouse.rx_rdy = false; return; }   // reset interno
    if (mouse.expect_mode) { mouse.expect_mode = false; return; }                               // palabra de modo
    mouse.cmd = v;
    if (v & 0x10) mouse.overrun = false;                                                         // ER
    if (v & 0x40) { mouse.expect_mode = true; mouse.cmd = 0; mouse.rx_rdy = false; }             // IR
}

// Raton Mouse Systems a 1200 baudios: un byte cada 8,33 ms (10 bits); paquete
// 10000LMR (activos a 0), dx, dy, 0, 0
void Machine::serial_time(int t) {
    auto sat = [](int v) { return v > 127 ? 127 : v < -127 ? -127 : v; };
    const double byte_cycles = 4e6 / 120;
    if (mouse.tick > 0) { mouse.tick -= t; return; }
    if (mouse.pkt_idx < 5) {
        if (mouse.rx_rdy) mouse.overrun = true;
        mouse.rx_data = mouse.pkt[mouse.pkt_idx++];
        mouse.rx_rdy = true;
        mouse.tick = byte_cycles;
        return;
    }
    if (!(mouse.cmd & 4)) { mouse.acc_x = mouse.acc_y = 0; return; }
    uint8_t nb = ((mouse.buttons & 1) << 2) | ((mouse.buttons & 4) >> 1) | ((mouse.buttons & 2) >> 1);   // L, M, R
    if (mouse.acc_x || mouse.acc_y || nb != mouse.btn_sent) {
        int dx = sat(mouse.acc_x), dy = sat(mouse.acc_y);
        mouse.pkt[0] = 0x80 | (~nb & 7);
        mouse.pkt[1] = (uint8_t)dx; mouse.pkt[2] = (uint8_t)dy; mouse.pkt[3] = 0; mouse.pkt[4] = 0;
        mouse.acc_x -= dx; mouse.acc_y -= dy;
        mouse.btn_sent = nb;
        mouse.pkt_idx = 0;
    }
}

// AMX por la fila 9 (joystick): un paso de 4 unidades por lectura
uint8_t Machine::amx_read() {
    uint8_t d = ((mouse.buttons & 4) ? 0x40 : 0) | ((mouse.buttons & 1) ? 0x20 : 0) | ((mouse.buttons & 2) ? 0x10 : 0);
    if (mouse.amx_dx <= -4)     { d |= 0x02; mouse.amx_dx += 4; }    // como amx_mouse.v
    else if (mouse.amx_dx >= 4) { d |= 0x01; mouse.amx_dx -= 4; }
    else mouse.amx_dx = 0;
    if (mouse.amx_dy <= -4)     { d |= 0x04; mouse.amx_dy += 4; }
    else if (mouse.amx_dy >= 4) { d |= 0x08; mouse.amx_dy -= 4; }
    else mouse.amx_dy = 0;
    return d;
}

// ---------------------------------------------------------------- MagicSound
// Puertos (A15..A10 = 111110, A7..A4 = D): F8Dx 8237, F9D8-F9DB 8254 n.1
// (contadores 0-2 = canales A-C), F9D4-F9D7 8254 n.2 (contador 0 = canal D),
// FAD0-FAD3 volumen, FBDx mapper de la DMA (A3..A2 canal, A1..A0 cuarto de
// 64 KB; la pagina va invertida)
static bool ms_port(uint16_t p) { return (p >> 10) == 0x3E && ((p >> 4) & 15) == 0xD; }

uint8_t Machine::ms_read(uint16_t p) {
    int blk = (p >> 8) & 3, r = p & 15;
    if (blk != 0) return 0xFF;
    if (r < 8) {
        int c = (r >> 1) & 3;
        uint16_t v = (r & 1) ? ms.cur_cnt[c] : ms.cur_adr[c];
        uint8_t b = ms.ff ? v >> 8 : v & 0xFF;
        ms.ff = !ms.ff;
        return b;
    }
    if (r == 8) { uint8_t v = ms.tc_stat & 15; ms.tc_stat = 0; return v; }
    return 0xFF;
}

void Machine::ms_write(uint16_t p, uint8_t v) {
    int blk = (p >> 8) & 3, r = p & 15;
    switch (blk) {
    case 0:                                                     // 8237
        if (r < 8) {
            int c = (r >> 1) & 3;
            uint16_t* base = (r & 1) ? ms.base_cnt : ms.base_adr;
            uint16_t* cur  = (r & 1) ? ms.cur_cnt : ms.cur_adr;
            if (!ms.ff) { base[c] = (base[c] & 0xFF00) | v; cur[c] = (cur[c] & 0xFF00) | v; }
            else        { base[c] = (base[c] & 0x00FF) | (v << 8); cur[c] = (cur[c] & 0x00FF) | (v << 8); }
            ms.ff = !ms.ff;
        } else switch (r) {
            case 0xA: if (v & 4) ms.dmask |= 1 << (v & 3); else ms.dmask &= ~(1 << (v & 3)); break;
            case 0xB: ms.dmode[v & 3] = v; break;
            case 0xC: ms.ff = false; break;
            case 0xD: ms.dmask = 0x0F; ms.ff = false; ms.tc_stat = 0; break;
            case 0xE: ms.dmask = 0; break;
            case 0xF: ms.dmask = v & 15; break;
        }
        break;
    case 1: {                                                   // 8254
        int tidx = (p & 8) ? (p & 3) : 3 + (p & 3);
        if ((p & 3) == 3) {
            int i = ((p & 8) ? 0 : 3) + (v >> 6);
            if ((v & 0x30) && i < 6) { ms.rwmode[i] = (v >> 4) & 3; ms.lsb_next[i] = true; }
        } else if (tidx < 6) {
            switch (ms.rwmode[tidx]) {
            case 1: ms.reload[tidx] = v; ms.tcount[tidx] = v ? v : 65536; break;
            case 2: ms.reload[tidx] = v << 8; ms.tcount[tidx] = (v << 8) ? (v << 8) : 65536; break;
            default:
                if (ms.lsb_next[tidx]) ms.reload[tidx] = (ms.reload[tidx] & 0xFF00) | v;
                else { ms.reload[tidx] = (ms.reload[tidx] & 0x00FF) | (v << 8); ms.tcount[tidx] = ms.reload[tidx] ? ms.reload[tidx] : 65536; }
                ms.lsb_next[tidx] = !ms.lsb_next[tidx];
            }
        }
        break;
    }
    case 2: ms.vol[p & 3] = v; break;                           // volumen (byte entero)
    case 3: ms.page[(p >> 2) & 3][p & 3] = ~v & 0x3F; break;    // mapper de la DMA (invertido)
    }
}

// Un byte de la RAM al DAC del canal; al acabar el bloque, NMI
void Machine::ms_dma(int c) {
    if (ms.dmask & (1 << c)) return;
    uint16_t a = ms.cur_adr[c];
    uint32_t phys = ((uint32_t)ms.page[c][a >> 14] << 14) | (a & 0x3FFF);
    ms.dac[c] = mem[phys];
    ms.cur_adr[c] = (ms.dmode[c] & 0x20) ? a - 1 : a + 1;
    if (ms.cur_cnt[c] == 0) {                                   // fin de bloque (TC)
        ms.tc_stat |= 1 << c;
        ms.nmi = true;
        if (ms.dmode[c] & 0x10) { ms.cur_adr[c] = ms.base_adr[c]; ms.cur_cnt[c] = ms.base_cnt[c]; }  // autoinicio
        else { ms.cur_cnt[c] = 0xFFFF; ms.dmask |= 1 << c; }
    } else ms.cur_cnt[c]--;
}

// Los contadores (modo de pulsos, 4 MHz = el reloj de la CPU): cada uno de los
// canales A-D pide un byte al vencer
void Machine::ms_time(int t) {
    for (int i = 0; i < 4; i++) {
        if (!ms.reload[i] && !ms.tcount[i]) continue;
        ms.tcount[i] -= t;
        while (ms.tcount[i] <= 0) {
            ms.tcount[i] += ms.reload[i] ? ms.reload[i] : 65536;
            ms_dma(i);
        }
    }
}

// ---------------------------------------------------------------- MMU legacy
// RAM en el slot 0 (000000-), ROM en el slot 1 (400000-: AL512.BIN)
uint32_t Machine::leg_translate(uint16_t a, bool write) const {
    int page = a >> 14;
    uint32_t ram;
    if (extport & 0x04) {                                       // mapper MSX (MAPMOD)
        ram = ((uint32_t)leg_map[page] << 14) | (a & 0x3FFF);
    } else {
        static const uint8_t cfg[8][4] = {
            {0, 1, 2, 3}, {0, 1, 2, 7}, {4, 5, 6, 7}, {0, 3, 2, 7},
            {0, 4, 2, 3}, {0, 5, 2, 3}, {0, 6, 2, 3}, {0, 7, 2, 3}};
        int pg = cfg[leg_mmr & 7][page];
        if (pg < 4) ram = a;
        else ram = ((uint32_t)(((leg_mmr >> 3) & 7) + 1) << 16) | ((uint32_t)(pg & 3) << 14) | (a & 0x3FFF);
    }
    if (!write) {
        if (page == 0 && !(leg_rmr & 0x04)) return 0x400000 + a;                         // ROM baja
        if (page == 3 && !(leg_rmr & 0x08)) return 0x400000 + ((uint32_t)leg_upper << 14) + (a & 0x3FFF);
    }
    return ram;
}

// ---------------------------------------------------------------- E/S legacy
uint8_t Machine::leg_pread(uint16_t port) {
    uint8_t v = 0xFF;
    // Gate Array (A15 = 0): el mapper se puede leer con MAPMOD
    if (!(port & 0x8000) && (extport & 0x04)) v &= 0xC0 | leg_map[(port >> 8) & 3];
    // CRTC (A14 = 0): BFxx lee R12-R17 (tipo 0)
    if (!(port & 0x4000) && (port & 0x100)) {
        uint8_t r = crt_idx & 31;
        v &= (r >= 12 && r <= 17) ? crt[r] : 0x00;
    }
    // PPI (A11 = 0)
    if (!(port & 0x0800)) {
        switch ((port >> 8) & 3) {
        case 0:                                                 // A: AY (BC1 = 1, BDIR = 0) o la salida
            if ((ppi_ctrl & 0x10) && (extport & 0x20) && (ppi_c & 7) == 5) {
                v &= rtc_read(rtc_leg_addr);                    // el reloj pone el dato (FABF D5, PC = 5)
            } else if (ppi_ctrl & 0x10) {
                uint8_t ay = 0xFF;
                if ((ppi_c & 0xC0) == 0x40) {
                    if (ay_addr == 14) {                        // puerto A del AY: la fila del teclado
                        int row = ppi_c & 15;
                        uint8_t k = row <= 10 ? kmatrix[row] : 0;
                        if (row == 9) k |= joy;
                        if (row == 9 && amx_mouse) {                // AMX: un paso al empezar a leer la fila
                            if (!mouse.amx_sel) mouse.amx_data = amx_read();
                            k |= mouse.amx_data;
                        }
                        mouse.amx_sel = (row == 9);
                        if (row == 2 && kfshift) k |= 0x20;
                        // como el core: SHIFT 2,5 s al arrancar el 520EX (modo
                        // AMSTRAD) si se pide, hasta que se pulse otra tecla
                        if (row == 2 && boot_shift && !leg_key && leg_since < 10000000) k |= 0x20;
                        ay = ~k;
                    } else if (ay_addr < 16) ay = ay_reg[ay_addr];
                }
                v &= ay;
            } else v &= ppi_a;
            break;
        case 1:                                                 // B: cinta, /EXP, 50 Hz, marca, DISKINT, VSYNC
            v &= (tape.level_now() ? 0x80 : 0) | (1 << 4) | (3 << 2) | (fdc.irq() ? 2 : 0) | (crt_vs ? 1 : 0);
            break;
        case 2: v &= ppi_c; break;
        default: v &= 0xFF;
        }
    }
    if (ms_port(port)) v &= ms_read(port);
    // 8251 con el raton serie (EExx datos, EFxx estado)
    if ((port >> 9) == 0x77) v &= serial_read(port & 0x100);
    // ratones de CPC
    if (cpc_mouse) {
        if ((port >> 9) == 0x7D && (port & 0x80) && !(port & 0x10)) {          // Kempston
            if (port & 0x100) v &= (port & 1) ? (mouse.ky & 0xFF) : (mouse.kx & 0xFF);
            else v &= ~((((mouse.buttons >> 2) & 1) << 2) | ((mouse.buttons & 1) << 1) | ((mouse.buttons >> 1) & 1)) | 0xF8;
        }
        if (port == 0xFD10) {                                                   // SYMBiFACE II
            auto c6 = [](int d) { return (uint8_t)((d > 31 ? 31 : d < -32 ? -32 : d) & 0x3F); };
            if (mouse.sf2_avail & 4)      { mouse.sf2_avail &= ~4; v &= 0x80 | c6(mouse.last_dy); }
            else if (mouse.sf2_avail & 2) { mouse.sf2_avail &= ~2; v &= 0x40 | c6(mouse.last_dx); }
            else if (mouse.sf2_avail & 1) { mouse.sf2_avail &= ~1; v &= 0xC0 | (mouse.buttons & 7); }
            else v &= 0;
        }
        if ((port >> 4) == 0xF99 && !(port & 8)) {                              // Multiplay
            auto c4 = [](int d) { return (uint8_t)(d > 7 ? 7 : d < -8 ? -8 : d); };
            switch (port & 7) {
            case 0: v &= (mouse.buttons & 7) << 4; break;
            case 2: v &= (mouse.mp_avail & 1) ? c4(mouse.last_dx) : 0; mouse.mp_avail &= ~1; break;
            case 3: v &= (mouse.mp_avail & 2) ? c4(-mouse.last_dy) : 0; mouse.mp_avail &= ~2; break;
            default: break;
            }
        }
    }
    // disquetera: FB7E estado, FB7F datos
    if (!(port & 0x0400) && (port & 0x0100) && !(port & 0x0080))
        v &= (port & 1) ? fdc.read_data() : fdc.read_status();
    return v;
}

void Machine::ppi_c_changed() {
    // reloj MC146818 (FABF D5): PC2..0 = 2 direccion, 4 escritura (dato en el
    // puerto A de la PPI), 5 lectura
    if (extport & 0x20) {
        switch (ppi_c & 7) {
        case 2: rtc_leg_addr = ppi_a & 63; break;
        case 4: rtc_write(rtc_leg_addr, ppi_a); break;
        default: break;
        }
    }
    // AY: BDIR = PC7, BC1 = PC6, dato por el puerto A de la PPI
    switch (ppi_c & 0xC0) {
    case 0xC0: ay_addr = ppi_a & 0x0F; break;                   // latch de direccion
    case 0x80:                                                  // escritura
        if (ay_addr < 16) { ay_reg[ay_addr] = ppi_a; if (ay_addr == 13) ay_env_restart(); }
        break;
    default: break;
    }
}

void Machine::leg_pwrite(uint16_t port, uint8_t v) {
    // Gate Array (A15 = 0)
    if (!(port & 0x8000)) {
        switch (v >> 6) {
        case 0: ga_pen = v & 0x1F; break;
        case 1: {
            uint8_t c = (extport & 0x04) ? (0x40 | (v & 0x3F)) : (v & 0x1F);
            if (ga_pen & 0x10) ga_border = c; else ga_ink[ga_pen & 15] = c;
            break;
        }
        case 2: leg_rmr = v; break;                             // modo y ROM (D4: LED RUS en el 520EX)
        case 3:
            if (extport & 0x04) leg_map[(port >> 8) & 3] = v & 0x3F;
            else leg_mmr = v;
            break;
        }
    }
    // CRTC (A14 = 0): BCxx indice, BDxx dato
    if (!(port & 0x4000)) {
        if (!(port & 0x100)) crt_idx = v & 31;
        else if (crt_idx < 18) crt[crt_idx] = v;
    }
    // ROM alta: DFxx, por la PROM D62 del 520EX
    if ((port >> 8) == 0xDF) leg_upper = (v == 7) ? 2 : (v == 3) ? 3 : 1;
    // PPI (A11 = 0)
    if (!(port & 0x0800)) {
        switch ((port >> 8) & 3) {
        case 0: ppi_a = v; if (!(extport & 0x20)) ppi_c_changed(); break;
        case 2: ppi_c = v; ppi_c_changed(); break;
        case 3:
            if (v & 0x80) { ppi_ctrl = v; ppi_a = 0; ppi_c = 0; ppi_c_changed(); }
            else {                                              // poner / quitar un bit de C
                int b = (v >> 1) & 7;
                if (v & 1) ppi_c |= 1 << b; else ppi_c &= ~(1 << b);
                ppi_c_changed();
            }
            break;
        default: break;
        }
    }
    if (ms_port(port)) ms_write(port, v);
    if ((port >> 9) == 0x77) serial_write(port & 0x100, v);
    // FABx (A7 = 1, A5 = 1): EXTEND PORT del 520EX
    if ((port >> 8) == 0xFA && (port & 0x80) && (port & 0x20)) extport = v;
    // disquetera: FB7F datos, FA7E motor
    if (!(port & 0x0400) && !(port & 0x0080)) {
        if (port & 0x0100) { if (port & 1) fdc.write_data(v); }
        else fdc.motor(v & 1);
    }
}

// ---------------------------------------------------------------- video
uint32_t Machine::leg_color(uint8_t c) const {
    static const uint8_t d62[32] = {
        0x15, 0x15, 0x1C, 0x1F, 0x10, 0x13, 0x14, 0x17, 0x13, 0x1F, 0x0F, 0x3F, 0x03, 0x33, 0x07, 0x37,
        0x10, 0x1C, 0x0C, 0x3C, 0x00, 0x30, 0x04, 0x34, 0x11, 0x1D, 0x0D, 0x3D, 0x01, 0x31, 0x05, 0x35};
    uint8_t c6 = (c & 0x40) ? (((c & 3) << 4) | (c & 0x0C) | ((c >> 4) & 3)) : d62[c & 31];
    // c6: r = bits 1..0, g = 3..2, b = 5..4 (RGB222); en MAPMOD el dato es RRGGBB
    auto lvl = [](int v) { return (uint32_t)(v * 85); };
    if (c & 0x40) {
        uint8_t rr = (c >> 4) & 3, gg = (c >> 2) & 3, bb = c & 3;
        return 0xFF000000 | (lvl(rr) << 16) | (lvl(gg) << 8) | lvl(bb);
    }
    return 0xFF000000 | (lvl(c6 & 3) << 16) | (lvl((c6 >> 2) & 3) << 8) | lvl((c6 >> 4) & 3);
}

// La ventana visible, fija respecto a los sincronismos de salida del Gate
// Array (como un monitor): de 12,25 us (15,6 en alta resolucion) a 48 us mas
// tras el HSYNC, lineas 30 a 302 tras el VSYNC
void Machine::leg_frame_done() {
    leg_frames++;
    int x0 = (extport & 0x02) ? 250 : 196;
    for (int y = 0; y < LEG_H; y++)
        memcpy(&leg_fb[y * LEG_W], &leg_raw[(y + 30) * 1024 + x0], LEG_W * 4);
    std::fill(leg_raw.begin(), leg_raw.end(), 0xFF000000);
}

// Un caracter del CRTC: pixeles, sincronismos del GA y la interrupcion
void Machine::crtc_char() {
    bool hires = extport & 0x02;
    // ---- salida de este caracter
    bool de = crt_hc < crt[1] && crt_vc < crt[6] && !crt_in_adj;
    uint16_t ma = crt_ma;
    int ra = crt_vlc;
    uint32_t addr = hires
        ? (((ma >> 13) & 1) << 15) | ((ra & 1) << 14) | (((ra >> 1) & 3) << 12) | ((ma & 0x7FF) << 1)
        : (((ma >> 12) & 3) << 14) | ((ra & 7) << 11) | ((ma & 0x3FF) << 1);
    uint16_t word = mem[addr & 0xFFFF] | (mem[(addr + 1) & 0xFFFF] << 8);
    int npix = hires ? 8 : 16;
    double pix_ns = hires ? 75.0 : 62.5;
    bool blank = ga_hs || ga_vs || !(extport & 0x08);
    for (int p = 0; p < npix; p++) {
        uint8_t col;
        if (blank) col = 0xFF;
        else if (!de) col = ga_border;
        else {
            uint8_t by; int q;
            if (!hires) { by = (p & 8) ? word >> 8 : word & 0xFF; q = p & 7; }
            else        { by = (p & 4) ? word >> 8 : word & 0xFF; q = (p & 3) << 1; }
            int inkn;
            int m = (hires ? 4 : 0) | ga_mode;
            switch (m) {
            case 0: case 4: case 7:                             // 16 colores
                inkn = (q & 4) ? (((by >> 0) & 1) << 3 | ((by >> 4) & 1) << 2 | ((by >> 2) & 1) << 1 | ((by >> 6) & 1))
                               : (((by >> 1) & 1) << 3 | ((by >> 5) & 1) << 2 | ((by >> 3) & 1) << 1 | ((by >> 7) & 1));
                break;
            case 1: case 5: case 6:                             // 4 colores
                switch ((q >> 1) & 3) {
                case 0: inkn = ((by >> 3) & 1) << 1 | ((by >> 7) & 1); break;
                case 1: inkn = ((by >> 2) & 1) << 1 | ((by >> 6) & 1); break;
                case 2: inkn = ((by >> 1) & 1) << 1 | ((by >> 5) & 1); break;
                default: inkn = ((by >> 0) & 1) << 1 | ((by >> 4) & 1); break;
                }
                break;
            case 2: inkn = (by >> (7 - q)) & 1; break;          // 2 colores
            default:                                            // CPC modo 3
                inkn = (q & 4) ? (((by >> 2) & 1) << 1 | ((by >> 6) & 1)) : (((by >> 3) & 1) << 1 | ((by >> 7) & 1));
            }
            col = ga_ink[inkn & 15];
        }
        uint32_t argb = (col == 0xFF) ? 0xFF000000 : leg_color(col);
        int x = (int)(beam_t / 62.5);
        int x2 = (int)((beam_t + pix_ns) / 62.5);
        for (int xx = x; xx < std::max(x + 1, x2); xx++)
            if (xx >= 0 && xx < 1024 && beam_y >= 0 && beam_y < 320) leg_raw[beam_y * 1024 + xx] = argb;
        beam_t += pix_ns;
    }

    // ---- sincronismos del CRTC para este caracter
    if (crt_hc == crt[2]) crt_hsc = (crt[3] & 15);              // tipo 0: anchura 0 = sin HSYNC
    bool hs = crt_hsc > 0;
    if (crt_hsc > 0) crt_hsc--;

    // ---- Gate Array: HSYNC de salida del 2.o al 5.o caracter del del CRTC,
    //      VSYNC de salida en las lineas 2..5 desde el del CRTC
    if (!hs) ga_hcnt = 0; else if (ga_hcnt != 7) ga_hcnt++;
    bool new_ga_hs = hs && ga_hcnt >= 2 && ga_hcnt <= 5;
    if (crt_vs && !prev_crt_vs) ga_vcnt = 0;
    else if (prev_crt_hs && !hs && ga_vcnt != 7) ga_vcnt++;
    bool new_ga_vs = ga_vcnt >= 2 && ga_vcnt <= 5;
    if (new_ga_vs && !ga_vs) vs_seen = true;                   // el VSYNC sube entre lineas
    if (new_ga_hs && !ga_hs) {                                  // monitor: linea nueva
        ga_mode = leg_rmr & 3;                                  // el modo cambia aqui
        beam_t = 0; beam_y++;
        if (vs_seen) { vs_seen = false; leg_frame_done(); beam_y = 0; }
    }
    ga_hs = new_ga_hs; ga_vs = new_ga_vs;

    // ---- interrupcion del Gate Array (300 Hz): cuenta los fines de HSYNC
    if (crt_vs && !prev_crt_vs) vs_wait = 2;
    if (prev_crt_hs && !hs) {
        if (vs_wait == 1) { if (irq_cnt >= 32) ga_irq = true; irq_cnt = 0; }
        else if (irq_cnt == 51) { ga_irq = true; irq_cnt = 0; }
        else irq_cnt++;
        if (vs_wait) vs_wait--;
    }
    prev_crt_hs = hs;
    prev_crt_vs = crt_vs;
    crt_hs = hs;

    // ---- avance de los contadores del CRTC
    crt_ma++;
    if (crt_hc == crt[0]) {                                     // fin de linea
        crt_hc = 0;
        if (crt_vsc > 0 && --crt_vsc == 0) crt_vs = false;
        if (crt_in_adj) {
            if (++crt_adj >= (crt[5] & 31)) {                   // fin del ajuste: cuadro nuevo
                crt_in_adj = false; crt_vc = 0; crt_vlc = 0;
                crt_ma_row = ((crt[12] & 0x3F) << 8) | crt[13];
            }
        } else if (crt_vlc == (crt[9] & 31)) {                  // fin de fila
            crt_vlc = 0;
            crt_ma_row += crt[1];
            if (crt_vc == (crt[4] & 127)) {
                if (crt[5] & 31) { crt_in_adj = true; crt_adj = 0; }
                else { crt_vc = 0; crt_ma_row = ((crt[12] & 0x3F) << 8) | crt[13]; }
            } else crt_vc = (crt_vc + 1) & 0x7F;
        } else crt_vlc = (crt_vlc + 1) & 0x1F;
        if (!crt_in_adj && crt_vlc == 0 && crt_vc == (crt[7] & 127) && !crt_vs) {
            crt_vs = true;
            crt_vsc = (crt[3] >> 4) ? (crt[3] >> 4) : 16;
        }
        crt_ma = crt_ma_row;
    } else crt_hc = (crt_hc + 1) & 0xFF;                      // 8 bits: si R0 baja, da la vuelta
}

void Machine::leg_time(int t) {
    double char_ns = (extport & 0x02) ? 600.0 : 1000.0;
    char_acc += t * 250.0;                                      // 4 MHz
    while (char_acc >= char_ns) { char_acc -= char_ns; crtc_char(); }
}

void Machine::dump_legacy() const {
    printf("legacy: PC=%04X extport=%02X rmr=%02X mmr=%02X upper=%d mode=%d frames=%llu irqs=%llu\n",
           z80ex_get_reg(cpu, regPC), extport, leg_rmr, leg_mmr, leg_upper, ga_mode,
           (unsigned long long)leg_frames, (unsigned long long)leg_irqs);
    printf("CRTC:");
    for (int i = 0; i < 16; i++) printf(" %02X", crt[i]);
    printf("  hc=%d vc=%d vlc=%d\n", crt_hc, crt_vc, crt_vlc);
    printf("inks:");
    for (int i = 0; i < 16; i++) printf(" %02X", ga_ink[i]);
    printf(" border=%02X\n", ga_border);
}

// ---------------------------------------------------------------- AY
// YM2149 a 1 MHz (como el core: 54 MHz / 54). Tono: un cambio cada 8 x TP
// relojes; ruido: el LFSR de 17 bits avanza cada 16 x NP; envolvente de 32
// pasos, un paso cada 8 x EP relojes. Volumen con la tabla del YM2149.v del
// core (32 niveles; el volumen fijo n es el nivel 2n + 1).
static const uint8_t ym_vol[32] = {
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x06, 0x07, 0x09, 0x0a, 0x0c, 0x0e, 0x11, 0x13,
    0x17, 0x1b, 0x20, 0x25, 0x2c, 0x35, 0x3e, 0x47, 0x54, 0x66, 0x77, 0x88, 0xa1, 0xc0, 0xe0, 0xff};

void Machine::ay_env_restart() {
    ay_ecnt = 0; ay_estep = 0; ay_ehold = false;
    ay_eatt = ay_reg[13] & 4;
}

void Machine::ay_sample(int& l, int& r) {
    ay_acc += 1e6 / AUDIO_RATE;
    while (ay_acc >= 1.0) {
        ay_acc -= 1.0;
        for (int c = 0; c < 3; c++) {
            int tp = (ay_reg[c * 2] | ((ay_reg[c * 2 + 1] & 15) << 8));
            if (!tp) tp = 1;
            if (++ay_tcnt[c] >= tp * 8) { ay_tcnt[c] = 0; ay_tout[c] = !ay_tout[c]; }
        }
        int np = ay_reg[6] & 31;
        if (!np) np = 1;
        if (++ay_ncnt >= np * 16) {
            ay_ncnt = 0;
            uint32_t bit = ((ay_lfsr >> 0) ^ (ay_lfsr >> 3)) & 1;
            ay_lfsr = (ay_lfsr >> 1) | (bit << 16);
        }
        int ep = ay_reg[11] | (ay_reg[12] << 8);
        if (!ep) ep = 1;
        if (!ay_ehold && ++ay_ecnt >= ep * 8) {
            ay_ecnt = 0;
            if (++ay_estep >= 32) {
                uint8_t sh = ay_reg[13];
                if (!(sh & 8)) { ay_ehold = true; ay_eatt = false; ay_estep = 31; }   // una vez y a 0
                else if (sh & 1) { ay_ehold = true; ay_estep = 31; if (sh & 2) ay_eatt = !ay_eatt; }
                else { ay_estep = 0; if (sh & 2) ay_eatt = !ay_eatt; }
            }
        }
    }
    int env = ay_ehold && !(ay_reg[13] & 8) ? 0 : (ay_eatt ? ay_estep : 31 - ay_estep);
    bool noise = ay_lfsr & 1;
    int out[3];
    for (int c = 0; c < 3; c++) {
        bool t = ay_tout[c] || (ay_reg[7] & (1 << c));
        bool n = noise || (ay_reg[7] & (8 << c));
        uint8_t a = ay_reg[8 + c];
        int lvl = (a & 0x10) ? env : ((a & 15) * 2 + ((a & 15) ? 1 : 0));
        out[c] = (t && n) ? ym_vol[lvl & 31] : 0;
    }
    // estereo ABC como el core: A x 2 + B a la izquierda, C x 2 + B a la derecha
    l = (out[0] * 2 + out[1]) << 4;
    r = (out[2] * 2 + out[1]) << 4;
    // MagicSound: (muestra - 80h) x volumen; A y D a la izquierda, B y C a la
    // derecha (como el Amiga); a la mezcla, a la mitad (como el core)
    auto chn = [&](int c) { return ((int)ms.dac[c] - 128) * (int)ms.vol[c]; };
    l += ((chn(0) + chn(3)) >> 2) >> 1;
    r += ((chn(1) + chn(2)) >> 2) >> 1;
}

} // namespace lx
