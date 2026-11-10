// gdbserver.h - Servidor del protocolo remoto de GDB (RSP) para alestelx
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Para depurar con gdb-multiarch o con DeZog (VS Code), como con el gdbstub
// de MAME. Registros en el orden de MAME (AF BC DE HL AF' BC' DE' HL' IX IY
// SP PC, 16 bits, byte bajo primero), descritos tambien en target.xml.
// Paquetes: ?, g, G, p, P, m, M, X (no), c, s, vCont, Z0/Z1 y z0/z1 (puntos
// de ruptura en la direccion de la CPU), D, k, qSupported,
// qXfer:features:read, qAttached, qC, qfThreadInfo/qsThreadInfo, qOffsets,
// qRcmd (ordenes del depurador de alestelx: su salida va a la terminal del
// emulador), Ctrl-C para parar.
// Memoria: 0000-FFFF, la que ve la CPU; desde 10000h, la fisica (direccion -
// 10000h), p. ej. 0xC60000 es la fisica C50000 (la VRAM de 8 bpp).
#pragma once
#include <string>
#include <cstdint>
#include "machine.h"
#include "debugger.h"

namespace lx {

class GdbServer {
public:
    GdbServer(Machine& m, Debugger& d) : m(m), dbg(d) {}
    ~GdbServer();
    bool listen(int port);
    void poll();                                // sin bloquear: conexiones y paquetes
    bool connected() const { return client >= 0; }
    bool kill_requested() const { return killed; }
private:
    Machine& m;
    Debugger& dbg;
    long long server = -1, client = -1;
    std::string in;
    bool running = false, killed = false, no_ack = false;
    void handle(const std::string& pkt);
    void send(const std::string& data);
    void stop_reply(int sig);
    std::string read_regs();
    uint16_t reg(int n);
    void set_reg(int n, uint16_t v);
    uint8_t read_mem(uint32_t a);
    void write_mem(uint32_t a, uint8_t v);
};

} // namespace lx
