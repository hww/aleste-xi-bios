#!/usr/bin/env python3
# mkpkt.py file.asm out.inc label [asm]: ensambla con el tools/asm de lx-gpu-rtl
# (make asm en deps/lx-gpu-rtl) y escribe el paquete
# [SIZE, START, palabras (16 bajos, 16 altos)] como dw de pasmo
import subprocess, sys, re
src, out, label = sys.argv[1], sys.argv[2], sys.argv[3]
asm = sys.argv[4] if len(sys.argv) > 4 else '../../../deps/lx-gpu-rtl/build/asm'
words = subprocess.run([asm, src], capture_output=True, text=True, check=True).stdout.split()
org = 0x20
for l in open(src):
    m = re.match(r'\s*\.org\s+(\S+)', l)
    if m: org = int(m.group(1), 0); break
pk = [2 * len(words), org]
for w in words:
    v = int(w, 16); pk += [v & 0xFFFF, v >> 16]
with open(out, 'w') as f:
    f.write(f'; generado con el ensamblador de lx-gpu-rtl desde {src}\n{label}:\n')
    for i in range(0, len(pk), 8):
        f.write('        dw ' + ', '.join('0%04Xh' % x for x in pk[i:i+8]) + '\n')
    f.write(f'{label}_end:\n')
print(src, len(words), 'instrucciones')
