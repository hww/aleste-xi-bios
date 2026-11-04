// debugger.h - Depurador de alestelx (consola en la terminal)
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww). Usa el desensamblador
// de z80ex y lee los mapas de z88dk (.map) para los nombres, como el
// depurador de z80_emu de hww.
//
// Ordenes (en la terminal; la tecla Pausa o Bloq Despl de la ventana para y
// sigue):
//   c | g              continuar            p        parar
//   s [n]              n instrucciones      n        saltar CALL/RST/LDIR...
//   t [n]              traza de n instrucciones
//   r                  registros, MMU y PIC
//   d [dir] [n]        desensamblar         m [dir] [n]   memoria (CPU)
//   pm fisica [n]      memoria fisica (24 bits)
//   b dir|simbolo      punto de ruptura (logico)    b p:fisica  (fisico)
//   bl                 lista                bc n | bc all  borrar
//   sym fichero.map    cargar simbolos      q        salir
// Las direcciones van en hexadecimal (C000, 0xC000, $C000) o por simbolo.
#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include "machine.h"

namespace lx {

class Debugger {
public:
    explicit Debugger(Machine& m);
    ~Debugger();

    void start_console();                       // hilo que lee la terminal
    void poll();                                // ejecuta las ordenes pendientes
    bool paused() const { return pause; }
    void set_paused(bool p);
    bool quit_requested() const { return quit; }
    bool load_symbols(const std::string& path);
    bool add_breakpoint(const std::string& spec);
    bool check(uint16_t pc);                    // enganche de Machine::break_check

private:
    Machine& m;
    bool pause = false, quit = false;
    std::set<uint16_t> bps;
    std::set<uint32_t> pbps;
    int  temp_bp = -1;                          // para "n"
    uint16_t resume_pc = 0;                      // no parar en el mismo sitio al seguir
    bool resuming = false;
    std::map<uint16_t, std::string> sym_by_addr;
    std::map<std::string, uint16_t> addr_by_sym;

    std::deque<std::string> queue;
    std::mutex qm;
    std::thread reader;

    void exec(const std::string& line);
    void prompt();
    bool parse_addr(const std::string& s, uint32_t& out);
    std::string label(uint16_t a) const;
    int  disasm(uint16_t a, std::string& text);
    void show_regs();
    void show_disasm(uint16_t a, int n);
    void dump(uint32_t a, int n, bool phys);
    void step_n(int n, bool trace);
    void step_over();
    void continue_run();
};

} // namespace lx
