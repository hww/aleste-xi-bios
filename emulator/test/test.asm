  ORG 0x0000
start:
        ld a, 'H'
        out (0x01), a
        ld a, 'i'
        out (0x01), a
        ld a, '!'
        out (0x01), a
loop:
        jp loop        ; Бесконечный цикл
