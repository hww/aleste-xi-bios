// tape.cpp - Reproductor de cintas TZX / CDT (ver tape.h)
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
#include "tape.h"
#include <cstdio>
#include <cstring>

namespace lx {

namespace {
struct Decoder {
    const std::vector<uint8_t>& d;
    std::vector<uint8_t> out_lv;
    std::vector<uint32_t> out_t;
    std::vector<uint8_t> out_stop;
    bool lv = false;
    explicit Decoder(const std::vector<uint8_t>& data) : d(data) {}

    uint32_t u16(size_t p) const { return p + 1 < d.size() ? d[p] | (d[p + 1] << 8) : 0; }
    uint32_t u24(size_t p) const { return p + 2 < d.size() ? d[p] | (d[p + 1] << 8) | (d[p + 2] << 16) : 0; }
    uint32_t u32(size_t p) const { return p + 3 < d.size() ? u24(p) | (d[p + 3] << 24) : 0; }

    void pulse(uint32_t t) { lv = !lv; out_t.push_back(t); out_lv.push_back(lv); out_stop.push_back(0); }
    void hold(uint32_t t, bool l, bool stop = false) { lv = l; out_t.push_back(t); out_lv.push_back(l); out_stop.push_back(stop); }
    // pausa en ms: tras ella la senal queda baja (1 ms al nivel anterior, como el estandar)
    void pause(uint32_t ms) {
        if (!ms) return;
        hold(3500, lv);
        if (ms > 1) hold((ms - 1) * 3500, false);
    }
    void bits(size_t p, uint32_t len, int last_bits, uint32_t zero, uint32_t one) {
        for (uint32_t i = 0; i < len && p + i < d.size(); i++) {
            int nb = (i == len - 1) ? last_bits : 8;
            for (int b = 0; b < nb; b++) {
                uint32_t t = (d[p + i] & (0x80 >> b)) ? one : zero;
                pulse(t); pulse(t);
            }
        }
    }
    void data_block(size_t p, uint32_t pilot, uint32_t s1, uint32_t s2, uint32_t zero, uint32_t one,
                    uint32_t npilot, int last_bits, uint32_t pause_ms, uint32_t len) {
        for (uint32_t i = 0; i < npilot; i++) pulse(pilot);
        if (s1) pulse(s1);
        if (s2) pulse(s2);
        bits(p, len, last_bits, zero, one);
        pause(pause_ms);
    }

    // Decodifica los bloques desde p hasta end; devuelve false si hay uno desconocido
    bool run(size_t p, size_t end) {
        while (p < end) {
            uint8_t id = d[p++];
            switch (id) {
            case 0x10: {
                uint32_t ps = u16(p), len = u16(p + 2);
                size_t q = p + 4;
                uint32_t np = (q < d.size() && d[q] < 0x80) ? 8063 : 3223;
                data_block(q, 2168, 667, 735, 855, 1710, np, 8, ps, len);
                p = q + len; break;
            }
            case 0x11: {
                uint32_t len = u24(p + 15);
                data_block(p + 18, u16(p), u16(p + 2), u16(p + 4), u16(p + 6), u16(p + 8), u16(p + 10),
                           d[p + 12] ? d[p + 12] : 8, u16(p + 13), len);
                p += 18 + len; break;
            }
            case 0x12: { uint32_t t = u16(p), n = u16(p + 2); for (uint32_t i = 0; i < n; i++) pulse(t); p += 4; break; }
            case 0x13: { int n = d[p]; for (int i = 0; i < n; i++) pulse(u16(p + 1 + i * 2)); p += 1 + n * 2; break; }
            case 0x14: {
                uint32_t len = u24(p + 7);
                bits(p + 10, len, d[p + 4] ? d[p + 4] : 8, u16(p), u16(p + 2));
                pause(u16(p + 5));
                p += 10 + len; break;
            }
            case 0x15: {
                uint32_t tps = u16(p), ps = u16(p + 2), len = u24(p + 5);
                int last = d[p + 4] ? d[p + 4] : 8;
                size_t q = p + 8;
                for (uint32_t i = 0; i < len && q + i < d.size(); i++) {
                    int nb = (i == len - 1) ? last : 8;
                    for (int b = 0; b < nb; b++) hold(tps, (d[q + i] & (0x80 >> b)) != 0);
                }
                pause(ps);
                p = q + len; break;
            }
            case 0x20: { uint32_t ms = u16(p); if (ms) pause(ms); else hold(0, false, true); p += 2; break; }
            case 0x21: p += 1 + d[p]; break;
            case 0x22: break;
            case 0x24: {
                uint32_t n = u16(p);
                size_t start = p + 2, q = start;
                // el bloque 25 que lo cierra (los bucles no se anidan)
                size_t le = start;
                while (q < end) {
                    if (d[q] == 0x25) { le = q; break; }
                    size_t nq = skip(q);
                    if (!nq) return false;
                    q = nq;
                }
                for (uint32_t i = 0; i < n; i++) if (!run(start, le)) return false;
                p = le + 1; break;
            }
            case 0x25: break;
            case 0x2A: p += 4; break;
            case 0x2B: lv = d[p + 4] != 0; p += 5; break;
            case 0x30: p += 1 + d[p]; break;
            case 0x31: p += 2 + d[p + 1]; break;
            case 0x32: p += 2 + u16(p); break;
            case 0x33: p += 1 + d[p] * 3; break;
            case 0x35: p += 20 + u32(p + 16); break;
            case 0x5A: p += 9; break;
            default:
                fprintf(stderr, "TZX: unknown block %02X\n", id);
                return false;
            }
        }
        return true;
    }

    // Posicion despues del bloque de p (0 si no se conoce)
    size_t skip(size_t p) const {
        uint8_t id = d[p++];
        switch (id) {
        case 0x10: return p + 4 + u16(p + 2);
        case 0x11: return p + 18 + u24(p + 15);
        case 0x12: return p + 4;
        case 0x13: return p + 1 + d[p] * 2;
        case 0x14: return p + 10 + u24(p + 7);
        case 0x15: return p + 8 + u24(p + 5);
        case 0x20: return p + 2;
        case 0x21: return p + 1 + d[p];
        case 0x22: return p;
        case 0x2A: return p + 4;
        case 0x2B: return p + 5;
        case 0x30: return p + 1 + d[p];
        case 0x31: return p + 2 + d[p + 1];
        case 0x32: return p + 2 + u16(p);
        case 0x33: return p + 1 + d[p] * 3;
        case 0x35: return p + 20 + u32(p + 16);
        case 0x5A: return p + 9;
        default: return 0;
        }
    }
};
}

bool Tape::load(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    std::vector<uint8_t> d;
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) d.insert(d.end(), buf, buf + n);
    fclose(f);
    if (d.size() < 10 || memcmp(d.data(), "ZXTape!\x1A", 8) != 0) {
        fprintf(stderr, "%s: not a TZX/CDT file\n", path.c_str());
        return false;
    }
    Decoder dec(d);
    dec.run(10, d.size());
    spans.clear();
    for (size_t i = 0; i < dec.out_t.size(); i++) spans.push_back({dec.out_t[i], dec.out_lv[i], dec.out_stop[i]});
    path_ = path;
    rewind();
    return !spans.empty();
}

bool Tape::run(int t, double cpu_hz, bool motor) {
    if (motor && !prev_motor) stopped = false;          // el motor vuelve a arrancar tras una parada
    prev_motor = motor;
    if (!motor || stopped || pos >= spans.size()) return level;
    acc += t * 3.5e6 / cpu_hz;                          // a estados T del Spectrum
    while (pos < spans.size()) {
        const Span& s = spans[pos];
        if (s.stop) { pos++; stopped = true; break; }
        level = s.level;
        if (acc < s.t) break;
        acc -= s.t;
        pos++;
    }
    return level;
}

} // namespace lx
