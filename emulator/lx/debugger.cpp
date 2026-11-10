// debugger.cpp - Depurador de alestelx, ver debugger.h
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
#include "debugger.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <regex>
#include <z80ex_dasm.h>

namespace lx {

Debugger::Debugger(Machine& mm) : m(mm) {
    m.break_check = [this](uint16_t pc) { return check(pc); };
}

Debugger::~Debugger() {
    if (reader.joinable()) reader.detach();
}

void Debugger::start_console() {
    reader = std::thread([this]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            std::lock_guard<std::mutex> g(qm);
            queue.push_back(line);
        }
    });
    printf("Debugger: type 'help' here (Pause/Scroll Lock in the window stops and resumes)\n");
    prompt();
}

void Debugger::prompt() { printf("dbg%s> ", pause ? " [paused]" : ""); fflush(stdout); }

void Debugger::poll() {
    std::deque<std::string> q;
    { std::lock_guard<std::mutex> g(qm); q.swap(queue); }
    for (auto& l : q) { exec(l); prompt(); }
}

void Debugger::set_paused(bool p) {
    if (p && !pause) { pause = true; printf("\n"); show_regs(); show_disasm(z80ex_get_reg(m.cpu, regPC), 1); prompt(); }
    else if (!p && pause) { continue_run(); }
}

void Debugger::continue_run() {
    pause = false;
    resume_pc = z80ex_get_reg(m.cpu, regPC);
    resuming = true;                    // no parar otra vez en este mismo punto
}

bool Debugger::check(uint16_t pc) {
    if (pause) return true;
    int hi = hist_n & (HIST_N - 1);
    hist_pc[hi] = pc; hist_sp[hi] = (uint16_t)z80ex_get_reg(m.cpu, regSP); hist_ph[hi] = m.cpu_phys(pc); hist_n++;
    if (resuming) { resuming = false; if (pc == resume_pc) return false; }
    bool hit = false;
    if (temp_bp >= 0 && pc == temp_bp) { temp_bp = -1; hit = true; }
    else if (bps.count(pc) || (!pbps.empty() && pbps.count(m.cpu_phys(pc))) ||
             (!pgbps.empty() && pgbps.count(m.cpu_phys(pc) >> 14))) {
        printf("\nBreakpoint at %04X%s\n", pc, label(pc).c_str());
        hit = true;
    }
    if (hit) { pause = true; show_regs(); show_disasm(pc, 1); prompt(); }
    return hit;
}

// ---- simbolos de z88dk: "nombre = $ABCD ; addr, public, ..."
bool Debugger::load_symbols(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    if (!f) { printf("Cannot open %s\n", path.c_str()); return false; }
    char buf[512];
    int n = 0;
    // nombre = $ABCD ; addr, public|local, , modulo, seccion, fichero:linea
    std::regex re(R"(^\s*(\S+)\s*=\s*\$([0-9A-Fa-f]+)\s*;\s*(addr|const)?\s*,\s*[^,]*,[^,]*,[^,]*,\s*([^,]*),)");
    while (fgets(buf, sizeof(buf), f)) {
        std::cmatch mt;
        if (!std::regex_search(buf, mt, re)) continue;
        if (mt[3].str() != "addr") continue;
        std::string name = mt[1].str();
        uint16_t a = (uint16_t)strtoul(mt[2].str().c_str(), nullptr, 16);
        addr_by_sym[name] = a;
        bool internal = name.rfind("__", 0) == 0 && (name.find("_head") != std::string::npos ||
                        name.find("_tail") != std::string::npos || name.find("_size") != std::string::npos);
        if (internal) continue;
        if (mt[4].str().empty()) continue;      // constante (sin seccion): solo por nombre
        auto it = sym_by_addr.find(a);
        // los nombres de C (_main) y los publicos de ensamblador antes que los internos
        if (it == sym_by_addr.end() || (it->second.rfind("__", 0) == 0 && name.rfind("__", 0) != 0))
            sym_by_addr[a] = name;
        n++;
    }
    fclose(f);
    printf("%d symbols from %s\n", n, path.c_str());
    return true;
}

std::string Debugger::label(uint16_t a) const {
    auto it = sym_by_addr.find(a);
    return it == sym_by_addr.end() ? "" : " <" + it->second + ">";
}

bool Debugger::parse_addr(const std::string& s0, uint32_t& out) {
    std::string s = s0;
    if (s.empty()) return false;
    auto it = addr_by_sym.find(s);
    if (it == addr_by_sym.end()) it = addr_by_sym.find("_" + s);
    if (it != addr_by_sym.end()) { out = it->second; return true; }
    if (s[0] == '$') s = s.substr(1);
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s = s.substr(2);
    char* end = nullptr;
    unsigned long v = strtoul(s.c_str(), &end, 16);
    if (!end || *end) return false;
    out = (uint32_t)v;
    return true;
}

static Z80EX_BYTE dasm_read(Z80EX_WORD a, void* u) { return ((Machine*)u)->peek(a); }

// historial: se desensambla desde la direccion fisica en la que se ejecuto
struct PhysRead { Machine* m; uint16_t pc; uint32_t ph; };
static Z80EX_BYTE dasm_read_phys(Z80EX_WORD a, void* u) {
    PhysRead* r = (PhysRead*)u;
    uint32_t p = (r->ph + (uint16_t)(a - r->pc)) & 0xFFFFFF;
    return r->m->mem[p];
}

int Debugger::disasm(uint16_t a, std::string& text) {
    char out[64];
    int t1, t2;
    int len = z80ex_dasm(out, sizeof(out), 0, &t1, &t2, dasm_read, a, &m);
    text = out;
    // destino de saltos y llamadas con su nombre
    std::smatch mt;
    static const std::regex hexw(R"(#([0-9A-F]{4}))");
    if (std::regex_search(text, mt, hexw)) {
        uint16_t t = (uint16_t)strtoul(mt[1].str().c_str(), nullptr, 16);
        auto it = sym_by_addr.find(t);
        if (it != sym_by_addr.end()) text += "   ; " + it->second;
    }
    return len;
}

void Debugger::show_disasm(uint16_t a, int n) {
    for (int i = 0; i < n; i++) {
        std::string t;
        int len = disasm(a, t);
        auto it = sym_by_addr.find(a);
        if (it != sym_by_addr.end()) printf("%s:\n", it->second.c_str());
        char bytes[16] = "";
        for (int k = 0; k < len && k < 4; k++) sprintf(bytes + k * 3, "%02X ", m.peek(a + k));
        printf("  %04X  %06X  %-12s %s\n", a, m.cpu_phys(a), bytes, t.c_str());
        a += len;
    }
}

void Debugger::show_regs() {
    auto r = [&](Z80_REG_T x) { return (unsigned)z80ex_get_reg(m.cpu, x); };
    uint16_t af = r(regAF);
    char fl[9] = "SZ5H3PNC";
    for (int i = 0; i < 8; i++) if (!(af & (0x80 >> i))) fl[i] = '-';
    printf("AF=%04X BC=%04X DE=%04X HL=%04X IX=%04X IY=%04X SP=%04X PC=%04X  [%s]\n",
           af, r(regBC), r(regDE), r(regHL), r(regIX), r(regIY), r(regSP), r(regPC), fl);
    printf("AF'=%04X BC'=%04X DE'=%04X HL'=%04X I=%02X R=%02X IM%u IFF1=%u IFF2=%u\n",
           r(regAF_), r(regBC_), r(regDE_), r(regHL_), r(regI), (r(regR7) & 0x80) | (r(regR) & 0x7F),
           r(regIM), r(regIFF1), r(regIFF2));
    const uint8_t* b = m.banks();
    printf("MMU: %s ctrl=%02X mmio page=%02X slots S=%02X U=%02X  banks", m.is_supervisor() ? "supervisor" : "user",
           m.mmu_ctrl(), m.mmio_pg(), m.super_slots(), m.user_slots());
    for (int s = 0; s < 4; s++) printf(" [%d:%02X %02X %02X %02X]", s, b[s * 4], b[s * 4 + 1], b[s * 4 + 2], b[s * 4 + 3]);
    printf("\nPIC: pending=%04X mask=%04X   cycles=%llu (%.3f s)\n", m.pic_pend(), m.pic_msk(),
           (unsigned long long)m.cycles, m.cycles / m.cpu_hz());
}

void Debugger::dump(uint32_t a, int n, bool phys) {
    for (int i = 0; i < n; i += 16) {
        printf("%s%06X ", phys ? "p:" : "  ", phys ? a + i : (unsigned)((a + i) & 0xFFFF));
        std::string asc;
        for (int k = 0; k < 16; k++) {
            uint8_t v = phys ? ((a + i + k) < MEM_SIZE ? m.mem[a + i + k] : 0xFF) : m.peek((uint16_t)(a + i + k));
            printf(" %02X", v);
            asc += (v >= 32 && v < 127) ? (char)v : '.';
        }
        printf("  %s\n", asc.c_str());
    }
}

void Debugger::step_n(int n, bool trace) {
    for (int i = 0; i < n; i++) {
        uint16_t pc = z80ex_get_reg(m.cpu, regPC);
        if (trace) show_disasm(pc, 1);
        do { m.step(); } while (z80ex_last_op_type(m.cpu));     // prefijos
    }
    if (!trace || n == 0) { show_regs(); show_disasm(z80ex_get_reg(m.cpu, regPC), 1); }
}

void Debugger::step_quiet() {
    pause = true;
    do { m.step(); } while (z80ex_last_op_type(m.cpu));
}

// CALL, RST, DJNZ y las de bloque: hasta la instruccion siguiente
void Debugger::step_over() {
    uint16_t pc = z80ex_get_reg(m.cpu, regPC);
    std::string t;
    int len = disasm(pc, t);
    bool over = t.rfind("call", 0) == 0 || t.rfind("rst", 0) == 0 || t.rfind("djnz", 0) == 0 ||
                t.rfind("ldir", 0) == 0 || t.rfind("lddr", 0) == 0 || t.rfind("cpir", 0) == 0 ||
                t.rfind("cpdr", 0) == 0 || t.rfind("inir", 0) == 0 || t.rfind("otir", 0) == 0 ||
                t.rfind("indr", 0) == 0 || t.rfind("otdr", 0) == 0 || t.rfind("halt", 0) == 0;
    if (!over) { step_n(1, false); return; }
    temp_bp = (uint16_t)(pc + len);
    continue_run();
}

void Debugger::exec(const std::string& line) {
    std::istringstream is(line);
    std::string c, a1, a2;
    is >> c >> a1 >> a2;
    if (c.empty()) return;
    uint32_t addr = 0;
    int n;
    if (c == "help" || c == "h" || c == "?") {
        printf("c|g continue   p pause   s [n] step   n step over   t [n] trace   r registers\n"
               "d [addr] [n] disassemble   m [addr] [n] memory   pm phys [n] physical memory\n"
               "b addr|symbol|p:phys breakpoint   bl list   bc n|all clear   hist [n] history\n"
               "disk a|b file   eject a|b   sym file.map   q quit\n");
    } else if (c == "c" || c == "g") {
        if (!a1.empty() && parse_addr(a1, addr)) temp_bp = (uint16_t)addr;   // "g dir": hasta ahi
        continue_run();
    } else if (c == "p") {
        set_paused(true);
    } else if (c == "s") {
        pause = true;
        step_n(a1.empty() ? 1 : std::max(1, atoi(a1.c_str())), false);
    } else if (c == "t") {
        pause = true;
        step_n(a1.empty() ? 16 : std::max(1, atoi(a1.c_str())), true);
        show_regs();
    } else if (c == "n") {
        pause = true;
        step_over();
    } else if (c == "r") {
        show_regs();
        show_disasm(z80ex_get_reg(m.cpu, regPC), 1);
    } else if (c == "d") {
        addr = z80ex_get_reg(m.cpu, regPC);
        if (!a1.empty() && !parse_addr(a1, addr)) { printf("Bad address\n"); return; }
        n = a2.empty() ? 16 : atoi(a2.c_str());
        show_disasm((uint16_t)addr, n);
    } else if (c == "m") {
        addr = z80ex_get_reg(m.cpu, regHL);
        if (!a1.empty() && !parse_addr(a1, addr)) { printf("Bad address\n"); return; }
        dump(addr, a2.empty() ? 64 : atoi(a2.c_str()), false);
    } else if (c == "pm") {
        if (!parse_addr(a1, addr)) { printf("pm physical_address [n]\n"); return; }
        dump(addr, a2.empty() ? 64 : atoi(a2.c_str()), true);
    } else if (c == "b") {
        if (!add_breakpoint(a1)) printf("b addr|symbol|p:physical\n");
    } else if (c == "bl") {
        int i = 0;
        for (auto b : bps) printf("%d: %04X%s\n", i++, b, label(b).c_str());
        for (auto b : pbps) printf("%d: p:%06X\n", i++, b);
        for (auto b : pgbps) printf("   page %02X\n", b);
        if (!i) printf("No breakpoints\n");
    } else if (c == "bc") {
        if (a1 == "all") { bps.clear(); pbps.clear(); pgbps.clear(); printf("Breakpoints cleared\n"); return; }
        int k = atoi(a1.c_str()), i = 0;
        for (auto it = bps.begin(); it != bps.end(); ++it, ++i) if (i == k) { bps.erase(it); return; }
        for (auto it = pbps.begin(); it != pbps.end(); ++it, ++i) if (i == k) { pbps.erase(it); return; }
        printf("No breakpoint %d\n", k);
    } else if (c == "disk" || c == "eject") {
        int d = (a1 == "b" || a1 == "B") ? 1 : 0;
        if (c == "disk" && a2.empty()) { printf("disk a|b file\n"); return; }
        if (disk_hook) disk_hook(d, c == "eject" ? std::string() : a2);
        else if (c == "eject") m.fdc.eject(d);
        else printf(m.fdc.mount(d, a2) ? "Drive %c: %s\n" : "Cannot mount %c: %s\n", 'A' + d, a2.c_str());
    } else if (c == "hist") {
        n = a1.empty() ? 32 : std::min(HIST_N, atoi(a1.c_str()));
        n = std::min(n, hist_n);
        for (int i = hist_n - n; i < hist_n; i++) {
            char out[64];
            int t1, t2;
            PhysRead pr{&m, hist_pc[i & (HIST_N - 1)], hist_ph[i & (HIST_N - 1)]};
            z80ex_dasm(out, sizeof(out), 0, &t1, &t2, dasm_read_phys, pr.pc, &pr);
            printf("  %04X  %06X  %-22s SP=%04X\n", pr.pc, pr.ph, out, hist_sp[i & (HIST_N - 1)]);
        }
    } else if (c == "sym") {
        load_symbols(a1);
    } else if (c == "q" || c == "quit") {
        quit = true;
    } else {
        printf("Unknown command (help)\n");
    }
}

bool Debugger::add_breakpoint(const std::string& spec) {
    uint32_t a;
    if (spec.rfind("pg:", 0) == 0) {
        if (!parse_addr(spec.substr(3), a)) return false;
        pgbps.insert(a & 0x3FF);
        printf("Breakpoint on physical page %02X (%06X-%06X)\n", a & 0x3FF, (a & 0x3FF) << 14, ((a & 0x3FF) << 14) + 0x3FFF);
        return true;
    }
    if (spec.rfind("p:", 0) == 0) {
        if (!parse_addr(spec.substr(2), a)) return false;
        pbps.insert(a & 0xFFFFFF);
        printf("Breakpoint at physical %06X\n", a & 0xFFFFFF);
        return true;
    }
    if (!parse_addr(spec, a)) return false;
    bps.insert((uint16_t)a);
    printf("Breakpoint at %04X%s\n", (uint16_t)a, label((uint16_t)a).c_str());
    return true;
}

} // namespace lx
