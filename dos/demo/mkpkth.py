#!/usr/bin/env python3
# mkpkth.py: ensambla un microprograma de la LX-GPU y lo escribe como un
# paquete del cargador [SIZE, START, palabras] en un array de C (uint16_t).
# Uso: mkpkth.py fuente.asm salida.h nombre [ensamblador]
# Shaeon (Carlos Palmero).
# Antes de ensamblar, cambia cada linea `.include "fichero"` por el fichero
# (relativo a la fuente).
import os, re, subprocess, sys, tempfile
src, out, name = sys.argv[1:4]
asm = sys.argv[4] if len(sys.argv) > 4 else 'asm'
def expand(path):
    res = []
    for line in open(path):
        m = re.match(r'\s*(\w+:)?\s*\.include\s+"([^"]+)"', line)
        if not m:
            res.append(line)
            continue
        if m.group(1):
            res.append(m.group(1) + '\n')
        res += expand(os.path.join(os.path.dirname(path), m.group(2)))
    return res
with tempfile.NamedTemporaryFile('w', suffix='.asm', delete=False) as t:
    t.writelines(expand(src))
r = subprocess.run([asm, t.name], capture_output=True, text=True)
os.unlink(t.name)
if r.returncode or not r.stdout.split():
    sys.exit(f'{src}: {r.stderr.strip() or "nada ensamblado"}')
words = [int(x, 16) for x in r.stdout.split()]
pk = [2 * len(words), 0x20]
for w in words: pk += [w & 0xFFFF, w >> 16]
with open(out, 'w') as f:
    f.write(f'/* generado con mkpkth.py desde {src.split("/")[-1]}: {len(words)} instrucciones */\n')
    f.write(f'static const uint16_t {name}[{len(pk)}] = {{\n')
    for i in range(0, len(pk), 8):
        f.write('    ' + ', '.join('0x%04X' % x for x in pk[i:i+8]) + ',\n')
    f.write('};\n')
