#!/usr/bin/env python3
# Prueba del servidor GDB RSP de alestelx: habla el protocolo como GDB/DeZog.
#   alestelx --headless --gdb 2159 boot.rom &   ;   python3 gdb_rsp_test.py 2159 [dir_main]
import socket, sys, time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 2159
main = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x43FD
s = socket.create_connection(('127.0.0.1', port), timeout=10)
buf = b''

def pkt(data):
    cs = sum(data.encode()) & 0xFF
    s.sendall(('$%s#%02x' % (data, cs)).encode())

def reply(timeout=10):
    global buf
    end = time.time() + timeout
    while time.time() < end:
        i = buf.find(b'$'); j = buf.find(b'#', i)
        if i >= 0 and j >= 0 and len(buf) >= j + 3:
            data = buf[i + 1:j].decode(); buf = buf[j + 3:]
            s.sendall(b'+')
            return data
        try: buf += s.recv(4096)
        except socket.timeout: pass
    raise SystemExit('no reply')

def cmd(data, timeout=10):
    pkt(data); return reply(timeout)

def regs(r):
    names = ['af', 'bc', 'de', 'hl', "af'", "bc'", "de'", "hl'", 'ix', 'iy', 'sp', 'pc']
    return {n: int(r[i*4+2:i*4+4] + r[i*4:i*4+2], 16) for i, n in enumerate(names)}

print('qSupported ->', cmd('qSupported:swbreak+;xmlRegisters=z80'))
x = cmd('qXfer:features:read:target.xml:0,fff')
print('target.xml   ->', x[:60] + '...', 'regs:', x.count('<reg '))
print('?            ->', cmd('?'))
r = regs(cmd('g')); print('g (reset)    -> PC=%04X SP=%04X' % (r['pc'], r['sp']))
print('m 0,8        ->', cmd('m0,8'), '(DI, JP 0100)')
print('Z0 main      ->', cmd('Z0,%x,1' % main))
pkt('c')
stop = reply(30); r = regs(cmd('g'))
print('c            -> %s, PC=%04X' % (stop, r['pc']))
print('m fisica C00000 (0x10000 + C00000) ->', cmd('m%x,4' % (0x10000 + 0xC00000)))
print('s            ->', cmd('s'), 'PC=%04X' % regs(cmd('g'))['pc'])
print('z0 main      ->', cmd('z0,%x,1' % main))
print('monitor r    ->', cmd('qRcmd,' + 'r'.encode().hex()))
pkt('c'); time.sleep(1.0); s.sendall(b'\x03')
print('Ctrl-C       ->', reply(10), 'PC=%04X' % regs(cmd('g'))['pc'])
pkt('k')
print('OK')
