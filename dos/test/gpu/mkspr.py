#!/usr/bin/env python3
# mkspr.py: el sprite de GPUSPR (16x16, 8 bpp) y la lista de los bytes que la
# GPU tiene que cambiar en el banco 2 de la pagina 80h (808000h-80BFFFh).
# En 8 bpp el pixel i de la pagina esta en el byte i ^ 1 (posicion 0 = byte
# alto de la palabra de 16 bits, y la memoria es little-endian).
spr = {}
for sy in range(16):
    for sx in range(16):
        spr[(sx, sy)] = 0 if (sx + sy) % 3 == 0 else 0x80 | ((sy * 16 + sx) & 0x7F)
src = [0] * 256                       # bytes 400h-4FFh de la pagina
for (sx, sy), v in spr.items():
    src[((0x400 + sy * 16 + sx) ^ 1) - 0x400] = v
patch = []
for (sx, sy), v in spr.items():
    if v == 0: continue
    b = (0x8564 + sy * 320 + sx) ^ 1   # 0x8564 = 800h + 100*320 + 100
    off = b - 0x8000
    if v != (b & 0xFF): patch.append((off, v))
patch.sort()
with open('sprite_data.inc', 'w') as f:
    f.write('; generado con mkspr.py\n; el sprite, tal como queda en 800400h-8004FFh\nspr_src:\n')
    for i in range(0, 256, 16):
        f.write('        db ' + ', '.join('0%02Xh' % x for x in src[i:i+16]) + '\n')
    f.write('; los bytes que cambia la GPU en el banco 2 (desplazamiento, valor), en orden\nspr_patch:\n')
    for off, v in patch:
        f.write('        dw 0%04Xh\n        db 0%02Xh\n' % (off, v))
    f.write('spr_patch_end:\n')
print(len(patch), 'bytes cambiados; ultimo', hex(patch[-1][0]))
