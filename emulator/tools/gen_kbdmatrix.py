#!/usr/bin/env python3
# Saca de rtl/core/cpc/kbd_520ex.sv (AlesteLX_MiSTer) la tabla PS/2 -> matriz
# del 520EX/CPC que usa el emulador (lx/kbdmatrix.inc), para que no se separen.
#   python3 tools/gen_kbdmatrix.py ruta/a/kbd_520ex.sv > lx/kbdmatrix.inc
import re, sys
s = open(sys.argv[1], encoding='utf-8', errors='replace').read().splitlines()
ext, norm, mode = [], [], None
for l in s:
    if "if (key_extended) begin" in l: mode = 'e'; continue
    if mode == 'e' and "end else begin" in l: mode = 'n'; continue
    if mode == 'n' and "endcase" in l: break
    m = re.search(r"8'h([0-9A-F]{2}):.*?key\[(\d+)\]\[(\d)\]", l)
    if m and mode:
        c = re.search(r"//\s*(.*)$", l)
        com = (c.group(1).strip() if c else "").encode('ascii', 'replace').decode()
        (ext if mode == 'e' else norm).append((int(m.group(1), 16), int(m.group(2)), int(m.group(3)), 'fshift' in l, com))
print("// Generado de rtl/core/cpc/kbd_520ex.sv (AlesteLX_MiSTer): codigo PS/2 (juego 2)")
print("// -> fila y bit de la matriz del 520EX/CPC; fshift = F6-F10 (con SHIFT).")
print("// No editar a mano: tools/gen_kbdmatrix.py lo vuelve a sacar del RTL.")
print("struct KbdMatrixKey { uint8_t code; uint8_t ext; uint8_t row; uint8_t bit; uint8_t fshift; };")
print("static const KbdMatrixKey kbd_matrix_keys[] = {")
for e, lst in ((1, ext), (0, norm)):
    for code, r, b, f, com in lst:
        print("    {0x%02X, %d, %2d, %d, %d},  // %s" % (code, e, r, b, 1 if f else 0, com))
print("};")
