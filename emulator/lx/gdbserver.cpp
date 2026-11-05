// gdbserver.cpp - Servidor GDB RSP de alestelx (ver gdbserver.h)
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
#include "gdbserver.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
static void sock_close(long long s) { closesocket((SOCKET)s); }
static void sock_nonblock(long long s) { u_long one = 1; ioctlsocket((SOCKET)s, FIONBIO, &one); }
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
static void sock_close(long long s) { close((int)s); }
static void sock_nonblock(long long s) { fcntl((int)s, F_SETFL, fcntl((int)s, F_GETFL) | O_NONBLOCK); }
#endif

namespace lx {

static const char* TARGET_XML =
    "<?xml version=\"1.0\"?><!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
    "<target version=\"1.0\"><architecture>z80</architecture><feature name=\"mame.z80\">"
    "<reg name=\"af\" bitsize=\"16\" type=\"int\"/><reg name=\"bc\" bitsize=\"16\" type=\"int\"/>"
    "<reg name=\"de\" bitsize=\"16\" type=\"int\"/><reg name=\"hl\" bitsize=\"16\" type=\"int\"/>"
    "<reg name=\"af'\" bitsize=\"16\" type=\"int\"/><reg name=\"bc'\" bitsize=\"16\" type=\"int\"/>"
    "<reg name=\"de'\" bitsize=\"16\" type=\"int\"/><reg name=\"hl'\" bitsize=\"16\" type=\"int\"/>"
    "<reg name=\"ix\" bitsize=\"16\" type=\"int\"/><reg name=\"iy\" bitsize=\"16\" type=\"int\"/>"
    "<reg name=\"sp\" bitsize=\"16\" type=\"data_ptr\"/><reg name=\"pc\" bitsize=\"16\" type=\"code_ptr\"/>"
    "</feature></target>";

static const Z80_REG_T REGS[12] = {regAF, regBC, regDE, regHL, regAF_, regBC_, regDE_, regHL_,
                                   regIX, regIY, regSP, regPC};

GdbServer::~GdbServer() {
    if (client >= 0) sock_close(client);
    if (server >= 0) sock_close(server);
}

bool GdbServer::listen(int port) {
#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);
#endif
    long long s = (long long)socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return false;
    int one = 1;
    setsockopt((int)s, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one));
    sockaddr_in a = {};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((uint16_t)port);
    if (bind((int)s, (sockaddr*)&a, sizeof(a)) != 0 || ::listen((int)s, 1) != 0) { sock_close(s); return false; }
    sock_nonblock(s);
    server = s;
    printf("GDB server: listening on localhost:%d (target remote :%d)\n", port, port);
    return true;
}

static int hexv(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 0; }
static std::string hex8(uint8_t v) { char b[3]; snprintf(b, sizeof(b), "%02x", v); return b; }

void GdbServer::send(const std::string& d) {
    if (client < 0) return;
    uint8_t cs = 0;
    for (char c : d) cs += (uint8_t)c;
    std::string p = "$" + d + "#" + hex8(cs);
    size_t off = 0;
    while (off < p.size()) {
        int n = (int)::send((int)client, p.data() + off, (int)(p.size() - off), 0);
        if (n > 0) off += n;
#ifdef _WIN32
        else if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
#else
        else if (n < 0 && errno == EAGAIN) continue;
#endif
        else break;
    }
}

void GdbServer::stop_reply(int sig) { send("S" + hex8((uint8_t)sig)); }

uint16_t GdbServer::reg(int n) { return (uint16_t)z80ex_get_reg(m.cpu, REGS[n]); }
void GdbServer::set_reg(int n, uint16_t v) { z80ex_set_reg(m.cpu, REGS[n], v); }

std::string GdbServer::read_regs() {
    std::string r;
    for (int i = 0; i < 12; i++) { uint16_t v = reg(i); r += hex8(v & 0xFF) + hex8(v >> 8); }
    return r;
}

uint8_t GdbServer::read_mem(uint32_t a) {
    if (a < 0x10000) return m.peek((uint16_t)a);
    a -= 0x10000;
    return a < MEM_SIZE ? m.mem[a] : 0xFF;
}

void GdbServer::write_mem(uint32_t a, uint8_t v) {
    if (a < 0x10000) { uint32_t p = m.cpu_phys((uint16_t)a); if (p < 0xFF0000) m.mem[p] = v; return; }
    a -= 0x10000;
    if (a < MEM_SIZE) m.mem[a] = v;
}

void GdbServer::poll() {
    if (server < 0) return;
    if (client < 0) {
        long long c = (long long)accept((int)server, nullptr, nullptr);
        if (c < 0) return;
        client = c;
        sock_nonblock(client);
        int one = 1;
        setsockopt((int)client, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof(one));
        in.clear(); running = false; no_ack = false;
        dbg.set_paused(true);
        printf("GDB: connected\n");
    }
    char buf[4096];
    for (;;) {
        int n = (int)recv((int)client, buf, sizeof(buf), 0);
        if (n > 0) { in.append(buf, n); continue; }
        if (n == 0) {                                           // se ha ido
            printf("GDB: disconnected\n");
            sock_close(client); client = -1; running = false;
            dbg.set_paused(false);
            return;
        }
        break;                                                  // nada mas por ahora
    }
    // Ctrl-C: parar
    size_t p;
    while ((p = in.find('\x03')) != std::string::npos && (in.find('$') == std::string::npos || p < in.find('$'))) {
        in.erase(p, 1);
        if (running) { dbg.set_paused(true); running = false; stop_reply(2); }
    }
    // paquetes completos $...#cs
    for (;;) {
        size_t s = in.find('$');
        if (s == std::string::npos) { in.clear(); break; }
        size_t e = in.find('#', s);
        if (e == std::string::npos || e + 2 >= in.size()) { in.erase(0, s); break; }
        std::string pkt = in.substr(s + 1, e - s - 1);
        in.erase(0, e + 3);
        if (!no_ack) ::send((int)client, "+", 1, 0);
        handle(pkt);
    }
    // parada por un punto de ruptura mientras corria
    if (running && dbg.paused()) { running = false; stop_reply(5); }
}

void GdbServer::handle(const std::string& p) {
    if (p.empty()) return;
    char c = p[0];
    switch (c) {
    case '?': stop_reply(5); return;
    case 'g': send(read_regs()); return;
    case 'G':
        for (int i = 0; i < 12 && (size_t)(1 + i * 4 + 4) <= p.size(); i++) {
            const char* h = p.c_str() + 1 + i * 4;
            set_reg(i, (uint16_t)((hexv(h[0]) << 4 | hexv(h[1])) | (hexv(h[2]) << 12 | hexv(h[3]) << 8)));
        }
        send("OK"); return;
    case 'p': {
        int n = (int)strtol(p.c_str() + 1, nullptr, 16);
        if (n < 12) { uint16_t v = reg(n); send(hex8(v & 0xFF) + hex8(v >> 8)); }
        else send("0000");
        return;
    }
    case 'P': {
        int n = (int)strtol(p.c_str() + 1, nullptr, 16);
        size_t eq = p.find('=');
        if (n < 12 && eq != std::string::npos && p.size() >= eq + 5) {
            const char* h = p.c_str() + eq + 1;
            set_reg(n, (uint16_t)((hexv(h[0]) << 4 | hexv(h[1])) | (hexv(h[2]) << 12 | hexv(h[3]) << 8)));
        }
        send("OK"); return;
    }
    case 'm': {
        uint32_t a = (uint32_t)strtoul(p.c_str() + 1, nullptr, 16);
        size_t comma = p.find(',');
        uint32_t n = comma == std::string::npos ? 0 : (uint32_t)strtoul(p.c_str() + comma + 1, nullptr, 16);
        std::string r;
        for (uint32_t i = 0; i < n && i < 2048; i++) r += hex8(read_mem(a + i));
        send(r); return;
    }
    case 'M': {
        uint32_t a = (uint32_t)strtoul(p.c_str() + 1, nullptr, 16);
        size_t comma = p.find(','), colon = p.find(':');
        if (comma == std::string::npos || colon == std::string::npos) { send("E01"); return; }
        uint32_t n = (uint32_t)strtoul(p.c_str() + comma + 1, nullptr, 16);
        const char* h = p.c_str() + colon + 1;
        for (uint32_t i = 0; i < n && h[i * 2] && h[i * 2 + 1]; i++)
            write_mem(a + i, (uint8_t)(hexv(h[i * 2]) << 4 | hexv(h[i * 2 + 1])));
        send("OK"); return;
    }
    case 'c':
        if (p.size() > 1) set_reg(11, (uint16_t)strtoul(p.c_str() + 1, nullptr, 16));
        dbg.set_paused(false); running = true; return;
    case 's':
        if (p.size() > 1) set_reg(11, (uint16_t)strtoul(p.c_str() + 1, nullptr, 16));
        dbg.step_quiet(); stop_reply(5); return;
    case 'Z': case 'z': {
        int type = p[1] - '0';
        if (type != 0 && type != 1) { send(""); return; }          // solo de ejecucion
        uint32_t a = (uint32_t)strtoul(p.c_str() + 3, nullptr, 16);
        if (c == 'Z') dbg.add_bp((uint16_t)a); else dbg.remove_bp((uint16_t)a);
        send("OK"); return;
    }
    case 'D': send("OK"); dbg.set_paused(false); running = false;
              sock_close(client); client = -1; printf("GDB: detached\n"); return;
    case 'k': killed = true; return;
    case 'H': send("OK"); return;
    case 'T': send("OK"); return;
    case 'v':
        if (p == "vCont?") { send("vCont;c;C;s;S;t"); return; }
        if (p.rfind("vCont;c", 0) == 0 || p.rfind("vCont;C", 0) == 0) { dbg.set_paused(false); running = true; return; }
        if (p.rfind("vCont;s", 0) == 0 || p.rfind("vCont;S", 0) == 0) { dbg.step_quiet(); stop_reply(5); return; }
        if (p.rfind("vCont;t", 0) == 0) { dbg.set_paused(true); running = false; stop_reply(2); return; }
        if (p.rfind("vKill", 0) == 0) { send("OK"); killed = true; return; }
        send(""); return;
    case 'q':
        if (p.rfind("qSupported", 0) == 0) { send("PacketSize=4000;qXfer:features:read+;swbreak+;QStartNoAckMode+"); return; }
        if (p.rfind("qXfer:features:read:target.xml:", 0) == 0) {
            size_t o = p.find(':', 31);
            unsigned long off = strtoul(p.c_str() + 31, nullptr, 16);
            unsigned long len = o == std::string::npos ? 4000 : strtoul(p.c_str() + p.find(',', 31) + 1, nullptr, 16);
            std::string x = TARGET_XML;
            if (off >= x.size()) { send("l"); return; }
            std::string part = x.substr(off, len);
            send((off + part.size() >= x.size() ? "l" : "m") + part);
            return;
        }
        if (p == "qAttached") { send("1"); return; }
        if (p == "qC") { send("QC1"); return; }
        if (p == "qfThreadInfo") { send("m1"); return; }
        if (p == "qsThreadInfo") { send("l"); return; }
        if (p == "qOffsets") { send("Text=0;Data=0;Bss=0"); return; }
        if (p == "qSymbol::") { send("OK"); return; }
        if (p.rfind("qRcmd,", 0) == 0) {                         // monitor <orden del depurador>
            std::string cmd;
            for (size_t i = 6; i + 1 < p.size(); i += 2) cmd += (char)(hexv(p[i]) << 4 | hexv(p[i + 1]));
            printf("GDB monitor: %s\n", cmd.c_str());
            dbg.run_command(cmd);
            fflush(stdout);
            send("OK");
            return;
        }
        send(""); return;
    case 'Q':
        if (p == "QStartNoAckMode") { send("OK"); no_ack = true; return; }
        send(""); return;
    default: send(""); return;
    }
}

} // namespace lx
