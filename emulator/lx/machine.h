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
#include <z80ex.h>
#include "fdc765.h"

namespace lx {

constexpr uint32_t MEM_SIZE   = 16u << 20;
constexpr uint32_t ROM_BASE   = 0xC00000;   // LX.ROM (64 KB)
constexpr uint32_t ROM2_BASE  = 0x400000;   // lo que va detras (AL512.BIN)
constexpr int      AUDIO_RATE = 48000;
constexpr int      SCREEN_W   = 640;
constexpr int      SCREEN_H   = 400;        // 200 lineas dobladas

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

    // Pantalla: SCREEN_W x SCREEN_H en ARGB8888
    const uint32_t* framebuffer() const { return fb.data(); }
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
    double   cpu_hz() const { return 54e6 / (mmu_clock + 1); }
    bool     legacy() const { return !(mmu_control & 0x02); }
    std::string stop_reason;

    uint32_t translate(uint16_t addr) const;            // direccion de la CPU -> fisica

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
public:
    bool     layout_es = false;                         // opcion del OSD
private:

    // ---- Video
    std::vector<uint32_t> fb;
    uint32_t rgb12(uint16_t c) const;
    double   frame_acc = 0;
    bool     vblank = false;
};

} // namespace lx
