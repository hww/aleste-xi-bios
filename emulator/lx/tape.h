// tape.h - Reproductor de cintas TZX / CDT para el modo 520EX/CPC
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Los mismos bloques que el tzxplayer del core (traduccion del de gyurco del
// core Amstrad de MiST): 10 normal, 11 turbo, 12 tono, 13 pulsos, 14 datos,
// 15 grabacion directa, 20 pausa (0 = parar), 21/22 grupos, 24/25 bucles,
// 2A, 2B nivel, 30-35 y 5A informativos. Al cargarla se decodifica a una lista
// de tramos (duracion en estados T del Spectrum, 3,5 MHz, y nivel); suena
// mientras el motor (PC4 de la PPI) esta encendido. La senal entra por PB7.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lx {

class Tape {
public:
    bool load(const std::string& path);
    void eject() { spans.clear(); path_.clear(); pos = 0; acc = 0; level = false; stopped = false; }
    void rewind() { pos = 0; acc = 0; level = false; stopped = false; }
    bool loaded() const { return !spans.empty(); }
    const std::string& path() const { return path_; }
    // Avanza t ciclos de la CPU (a cpu_hz); devuelve el nivel de la senal
    bool run(int t, double cpu_hz, bool motor);
    bool level_now() const { return level; }
    int  percent() const { return spans.empty() ? 0 : (int)(100.0 * pos / spans.size()); }
    bool at_end() const { return pos >= spans.size(); }
private:
    struct Span { uint32_t t; uint8_t level; uint8_t stop; };   // stop: bloque 20 con pausa 0
    std::vector<Span> spans;
    std::string path_;
    size_t pos = 0;
    double acc = 0;
    bool   level = false, stopped = false, prev_motor = false;
};

} // namespace lx
