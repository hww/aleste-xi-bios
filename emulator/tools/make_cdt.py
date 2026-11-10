#!/usr/bin/env python3
# make_cdt.py - Fichero -> cinta CDT del Amstrad CPC (formato del firmware)
#   python3 make_cdt.py fichero.bin NOMBRE salida.cdt [--load 4000] [--entry 4000] [--type 2]
# Bloques de 2 KB: registro de cabecera (sincronismo 2Ch, 64 bytes) y de datos
# (16h), en segmentos de 256 bytes con CRC-CCITT (FFFFh, invertido, MSB primero),
# 4 bytes FFh de cola; velocidad de 1000 baudios (bloques TZX 11h, como los
# CDT habituales). Tipo 0 = BASIC, 2 = binario.
import argparse, struct

def crc16(data):
    c = 0xFFFF
    for b in data:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c ^ 0xFFFF

def record(sync, data):
    out = bytes([sync])
    for i in range(0, len(data), 256):
        seg = data[i:i + 256].ljust(256, b'\0')
        out += seg + struct.pack('>H', crc16(seg))
    return out + b'\xff' * 4

def tzx11(payload, pause=1000):
    pilot, sync, zero, one = 0x091A, 0x048D, 0x048D, 0x091A
    return bytes([0x11]) + struct.pack('<HHHHHHBH', pilot, sync, sync, zero, one, 0x1000, 8, pause) + \
           struct.pack('<I', len(payload))[:3] + payload

ap = argparse.ArgumentParser()
ap.add_argument('file'); ap.add_argument('name'); ap.add_argument('out')
ap.add_argument('--load', default='4000'); ap.add_argument('--entry', default=None)
ap.add_argument('--type', type=int, default=2)
a = ap.parse_args()
data = open(a.file, 'rb').read()
load = int(a.load, 16); entry = int(a.entry, 16) if a.entry else load
name = a.name.upper().encode()[:16].ljust(16, b'\0')
cdt = b'ZXTape!\x1a\x01\x14'
nblocks = (len(data) + 2047) // 2048
for n in range(nblocks):
    chunk = data[n * 2048:(n + 1) * 2048]
    hdr = name + bytes([n + 1, 0xFF if n == nblocks - 1 else 0, a.type]) + \
          struct.pack('<HH', len(chunk), load + n * 2048) + bytes([0xFF if n == 0 else 0]) + \
          struct.pack('<HH', len(data), entry)
    hdr = hdr.ljust(64, b'\0')
    cdt += tzx11(record(0x2C, hdr), 10) + tzx11(record(0x16, chunk), 2000)
open(a.out, 'wb').write(cdt)
print(f"{a.out}: {len(data)} bytes, {nblocks} block(s), load {load:04X} entry {entry:04X}")
