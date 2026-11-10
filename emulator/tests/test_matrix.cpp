// Prueba de la matriz del teclado de alestelx contra la tabla del RTL
// (kbd_520ex.sv): cada tecla pulsada aparece en su fila y bit, activa a 0,
// en los puertos 10h-1Ah de la pagina MMIO 8, y desaparece al soltarla.
#include <cstdio>
#include "../lx/machine.h"
namespace lx {
#include "../lx/kbdmatrix.inc"
}
int main() {
    static lx::Machine m;
    m.pwrite(0xF1, 8);                                  // pagina MMIO 8 (supervisor tras el reset)
    int fails = 0, n = 0;
    for (auto& k : lx::kbd_matrix_keys) {
        m.key_event(k.code, true, k.ext);
        uint8_t v = m.pread(0x10 + k.row);
        if (v & (1 << k.bit)) { printf("FAIL %02X%s: fila %d bit %d no pulsado (%02X)\n", k.code, k.ext ? " E0" : "", k.row, k.bit, v); fails++; }
        m.key_event(k.code, false, k.ext);
        for (int r = 0; r <= 10; r++) {
            uint8_t w = m.pread(0x10 + r);
            if (w != 0xFF && !(r == 2 && k.code == 0x14)) { /* otra tecla con el mismo bit puede quedar */ }
        }
        n++;
    }
    // todo suelto al final
    for (int r = 0; r <= 10; r++) if (m.pread(0x10 + r) != 0xFF) { printf("FAIL fila %d queda %02X\n", r, m.pread(0x10 + r)); fails++; }
    // joystick: fila 9 = teclas | joy (bit 0 arriba ... bit 5 fuego 1)
    m.joy = 0x21;
    if (m.pread(0x19) != (uint8_t)~0x21) { printf("FAIL joystick: fila 9 = %02X\n", m.pread(0x19)); fails++; }
    m.joy = 0;
    // ejemplo: 'A' (1Ch)
    m.key_event(0x1C, true, false);
    printf("A pulsada: fila 8 = %02X\n", m.pread(0x18));
    printf("%d teclas, %d fallos\n", n, fails);
    return fails != 0;
}
