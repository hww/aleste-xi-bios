// legacy.cpp - Modo legacy del Aleste LX: Aleste 520EX / Amstrad CPC
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Sigue el RTL del core AlesteLX_MiSTer:
//   MMU legacy            rtl/core/mmu_legacy.sv
//   video (CRTC + GA)     rtl/core/cpc/cpc_video.sv (UM6845R tipo 0, GA40010)
//   E/S, PPI, AY, IRQ     rtl/core/aleste_lx_core.sv
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
            v &= (0 << 7) | (1 << 4) | (3 << 2) | (fdc.irq() ? 2 : 0) | (crt_vs ? 1 : 0);
            break;
        case 2: v &= ppi_c; break;
        default: v &= 0xFF;
        }
    }
    // 8251 en reposo (EExx datos, EFxx estado 85h: lo lee la BIOS de MSX-DOS)
    if ((port >> 9) == 0x77) v &= (port & 0x100) ? 0x85 : 0x00;
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
            } else crt_vc++;
        } else crt_vlc++;
        if (!crt_in_adj && crt_vlc == 0 && crt_vc == (crt[7] & 127) && !crt_vs) {
            crt_vs = true;
            crt_vsc = (crt[3] >> 4) ? (crt[3] >> 4) : 16;
        }
        crt_ma = crt_ma_row;
    } else crt_hc++;
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
}

} // namespace lx
