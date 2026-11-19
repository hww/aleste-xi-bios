#!/usr/bin/env python3
# mkpal.py: escribe pal.inc (paleta de MAG.COM, 256 colores 0RGB de 12 bits)
# y grad.inc (los 32 colores del degradado del titulo, que MAG rota).
# Shaeon (Carlos Palmero).
import colorsys
def rgb(r, g, b): return (min(15, round(r)) << 8) | (min(15, round(g)) << 4) | min(15, round(b))
pal = [0] * 256
fixed = {0: (0,0,0), 1: (15,15,15), 2: (0,0,0), 3: (15,8,0), 4: (15,4,3),
         5: (15,9,10), 6: (15,12,0), 7: (12,5,15), 8: (11,11,12), 9: (6,13,15)}
for i, c in fixed.items(): pal[i] = rgb(*c)
for i, c in enumerate([(5,5,8), (8,8,11), (11,12,15), (15,15,15)]): pal[32 + i] = rgb(*c)
for i in range(64):                        # medidores: verde -> amarillo -> rojo
    t = i / 63
    if t < 0.6: r, g = 15 * t / 0.6, 15
    else:       r, g = 15, 15 * (1 - (t - 0.6) / 0.4)
    pal[64 + i] = rgb(r, g, 1)
grad = []                                  # degradado: un arco iris suave
for i in range(32):
    h = i / 32
    r, g, b = colorsys.hsv_to_rgb(h, 0.75, 1.0)
    grad.append(rgb(15 * r, 15 * g, 15 * b))
for i in range(16): pal[16 + i] = grad[i]
def dw(name, v):
    out = f'{name}:\n'
    for i in range(0, len(v), 8):
        out += '        dw ' + ', '.join('0%03Xh' % x for x in v[i:i+8]) + '\n'
    return out
open('pal.inc', 'w').write('; generado con mkpal.py\n' + dw('pal', pal) + dw('grad', grad))
