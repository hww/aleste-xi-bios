// machine.h - Aleste LX (modo nativo) para el emulador
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Emula el hardware tal como esta en el RTL del core AlesteLX_MiSTer:
// memoria fisica de 16 MB (la SDRAM), MMU nativa (puertos E0h-FFh), y los
// dispositivos de las paginas MMIO (puerto F1h = pagina; 00h-7Fh =
// FF0000 + pagina * 128 + puerto):
//   0  PIC (00-08), temporizador (40-46), reloj MC146818 (60 indice, 61 dato)
//   2  paleta (00-06) y CRTC (20 indice, 21 dato)
//   6  DMA / blitter            7  Magic Sound 2
//   8  teclado PS/2             9  disquetera uPD765 (00 estado, 01 datos, 02 motor)
// La CPU es z80ex. Reloj: 54 MHz / (divisor F3h + 1) = 18 MHz al arrancar.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <z80ex.h>
#include "fdc765.h"

namespace lx {

constexpr uint32_t MEM_SIZE   = 16u << 20;
constexpr uint32_t ROM_BASE   = 0xC00000;   // LX.ROM (64 KB)
constexpr uint32_t ROM2_BASE  = 0x400000;   // lo que va detras (AL512.BIN)
constexpr int      AUDIO_RATE = 48000;
constexpr int      SCREEN_W   = 640;
constexpr int      SCREEN_H   = 400;        // 200 lineas dobladas
constexpr int      LEG_W      = 768;        // modo legacy: 48 us a 16 MHz
constexpr int      LEG_H      = 272;        // lineas 30-302 tras el VSYNC

struct PS2Event { uint8_t code; bool pressed; bool extended; };

class Machine {
public:
    Machine();
    ~Machine();

    bool load_boot_rom(const std::string& path);       // LX.ROM [+ AL512.BIN]
    bool load_raw(const std::string& path, uint32_t phys);
    void reset();

    // Ejecuta hasta completar un cuadro de video (60 Hz). Devuelve false si
    // la CPU se ha parado (modo legacy no emulado todavia, etc.).
    bool run_frame();
    // Una instruccion (o la aceptacion de una interrupcion), con el tiempo de
    // los dispositivos; devuelve los ciclos
    int  step();
    // Depurador: se llama antes de cada instruccion; si devuelve true, la
    // ejecucion se para ahi (run_frame vuelve con break_hit = true)
    std::function<bool(uint16_t pc)> break_check;
    bool break_hit = false;

    // Pantalla en ARGB8888: SCREEN_W x SCREEN_H en modo nativo; en modo
    // legacy (520EX/CPC), la ventana visible del monitor (LEG_W x LEG_H)
    const uint32_t* framebuffer() const { return legacy() ? leg_fb.data() : fb.data(); }
    int  fb_width() const  { return legacy() ? LEG_W : SCREEN_W; }
    int  fb_height() const { return legacy() ? LEG_H : SCREEN_H; }
    void render();

    // Audio: muestras estereo de 16 bits producidas desde la ultima llamada
    std::vector<int16_t>& audio() { return audio_buf; }

    // Teclado
    void key_event(uint8_t code, bool pressed, bool extended);

    // Disquetera
    Fdc765 fdc;

    // Estado para el depurador y las pruebas
    Z80EX_CONTEXT* cpu = nullptr;
    std::vector<uint8_t> mem;
    uint64_t cycles = 0;
    double   cpu_hz() const { return legacy() ? 4e6 : 54e6 / (mmu_clock + 1); }
    bool     legacy() const { return !(mmu_control & 0x02); }
    std::string stop_reason;
    void     dump_legacy() const;                       // estado del modo legacy (pruebas)
    uint64_t leg_frames = 0, leg_irqs = 0;

    uint32_t translate(uint16_t addr) const;            // direccion de la CPU -> fisica
    uint8_t  peek(uint16_t addr) const { uint32_t p = legacy() ? leg_translate(addr, false) : translate(addr); return p < 0xFF0000 ? mem[p] : 0xFF; }
    // estado de la MMU para el depurador
    bool     is_supervisor() const { return supervisor; }
    uint8_t  mmu_ctrl() const { return mmu_control; }
    uint8_t  mmio_pg() const { return mmu_page; }
    uint8_t  super_slots() const { return slot_super; }
    uint8_t  user_slots() const { return slot_user; }
    const uint8_t* banks() const { return bank; }
    uint16_t pic_pend() const { return pic_pending; }
    uint16_t pic_msk() const { return pic_mask; }

    // Callbacks de z80ex
    uint8_t  mread(uint16_t addr, int m1);
    void     mwrite(uint16_t addr, uint8_t v);
    uint8_t  pread(uint16_t port);
    void     pwrite(uint16_t port, uint8_t v);
    uint8_t  intread();

private:
    // ---- MMU nativa
    uint8_t mmu_control = 0x13, mmu_page = 0, mmu_clock = 2;
    uint8_t slot_super = 0xFF, slot_user = 0xFF;
    uint8_t bank[16] = {};
    bool    supervisor = true, exit_pending = false;
    uint8_t cur_slot(int page) const;
    uint8_t mmu_read(uint8_t reg, uint16_t port);
    void    mmu_write(uint8_t reg, uint8_t v);

    // ---- MMIO
    uint8_t mmio_read(uint32_t a);
    void    mmio_write(uint32_t a, uint8_t v);

    // ---- PIC y temporizador (pagina 0)
    uint16_t pic_mask = 0, pic_pending = 0;
    uint8_t  pic_vector = 0;
    uint16_t irq_lines = 0, irq_prev = 0;
    void     set_line(int n, bool level);
    void     pic_update();
    uint8_t  tmr_ctrl = 0;
    uint16_t tmr_reload = 0, tmr_count = 0, tmr_ticks = 0;
    uint8_t  tmr_cnt_hi_l = 0, tmr_ticks_hi_l = 0;
    double   us_acc = 0, ms_acc = 0;
    void     timer_us(int n);

    // ---- Reloj MC146818 (pagina 0, 60h/61h)
    uint8_t  rtc_idx = 0, rtc_ram[64] = {};
    int64_t  rtc_offset = 0;                            // segundos respecto al reloj del PC
    uint8_t  rtc_read(uint8_t r);
    void     rtc_write(uint8_t r, uint8_t v);

    // ---- Paleta y CRTC (pagina 2)
    uint8_t  pal_index = 0, pal_ctrl = 0, pal_mod = 0, pal_lo = 0;
    uint16_t palette[256] = {}, border = 0x888;
    uint8_t  crtc_idx = 0, crtc[32] = {};

    // ---- DMA (pagina 6)
    uint8_t  dma_reg[0x11] = {}, dma_ctrl = 0;
    bool     dma_done = false;
    void     dma_start(uint8_t ctrl);

public:
    // ---- Magic Sound 2 (pagina 7)
    struct Ms2Ch {
        uint8_t  ctrl = 0;
        uint32_t start = 0, end = 0, loop = 0, step = 0x10000;
        uint8_t  vol_l = 255, vol_r = 255;
        uint64_t pos = 0;                               // 24.16
        int16_t  smp = 0;
        uint16_t tmp = 0;
        bool     eof = false;
    } ch[4];
private:
    uint8_t  ms2_ctrl = 0, ms2_vol_l = 255, ms2_vol_r = 255, ms2_irq_ctrl = 0, ms2_irq_stat = 0;
    uint16_t ms2_tmr_period = 0, ms2_tmr_cnt = 0;
    double   sample_acc = 0;
    uint8_t  ms2_read(uint8_t r);
    void     ms2_write(uint8_t r, uint8_t v);
    void     ms2_tick();
    std::vector<int16_t> audio_buf;

    // ---- Teclado (pagina 8)
    std::deque<PS2Event> kq;
    bool     kq_ovf = false;
    uint8_t  kin_ctrl = 0;
    uint8_t  kmatrix[11] = {};                          // filas del 520EX/CPC, 1 = pulsada
    bool     kfshift = false;                           // F6-F10: SHIFT (fila 2) en la lectura del CPC
public:
    uint8_t  joy = 0;                                   // fila 9 (joystick), 1 = pulsado
    const uint8_t* matrix() const { return kmatrix; }
private:
public:
    bool     layout_es = false;                         // opcion del OSD
    bool     boot_shift = false;                        // SHIFT al entrar en el 520EX: modo AMSTRAD
    uint64_t leg_since = 0;                             // ciclos desde la entrada en modo legacy
    bool     leg_key = false;                           // se ha pulsado una tecla desde entonces
    bool     was_legacy = false;
private:

    // ---- Modo legacy (520EX/CPC): legacy.cpp, segun mmu_legacy.sv,
    //      cpc_video.sv y la E/S legacy de aleste_lx_core.sv
    uint8_t  leg_rmr = 0x80, leg_mmr = 0xC0, leg_upper = 0, leg_map[4] = {0, 1, 2, 3};
    uint8_t  extport = 0;                               // FABF: D1 alta res., D2 MAPMOD, D3 pantalla
    uint32_t leg_translate(uint16_t a, bool write) const;
    uint8_t  leg_pread(uint16_t port);
    void     leg_pwrite(uint16_t port, uint8_t v);
    void     leg_time(int tstates);                     // avanza el CRTC y el video
    void     leg_reset();
    // CRTC (UM6845, tipo 0)
    uint8_t  crt_idx = 0, crt[18] = {};
    int      crt_hc = 0, crt_vlc = 0, crt_vc = 0, crt_adj = 0, crt_hsc = 0, crt_vsc = 0;
    bool     crt_in_adj = false, crt_hs = false, crt_vs = false;
    uint16_t crt_ma = 0, crt_ma_row = 0;
    double   char_acc = 0;
    void     crtc_char();
    // Gate Array
    uint8_t  ga_pen = 0, ga_ink[16] = {}, ga_border = 0x14, ga_mode = 1;
    int      ga_hcnt = 0, ga_vcnt = 7;
    bool     ga_hs = false, ga_vs = false, prev_crt_hs = false, prev_crt_vs = false, vs_seen = false;
    int      irq_cnt = 0, vs_wait = 0;
    bool     ga_irq = false;
    // PPI 8255 y AY (registros; el sonido, en la fase 2)
    uint8_t  ppi_a = 0, ppi_c = 0, ppi_ctrl = 0x9B, ay_addr = 0, ay_reg[16] = {};
    void     ppi_c_changed();
    // Monitor: imagen en bruto y ventana visible
    std::vector<uint32_t> leg_raw, leg_fb;
    int      beam_x = 0, beam_y = 0;
    double   beam_t = 0;                                // ns desde el HSYNC de salida
    void     leg_frame_done();
    uint32_t leg_color(uint8_t c7) const;

    // ---- Video
    std::vector<uint32_t> fb;
    uint32_t rgb12(uint16_t c) const;
    double   frame_acc = 0;
    bool     vblank = false;
};

} // namespace lx
