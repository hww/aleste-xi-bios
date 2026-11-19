; lo que debe quedar en 800000h-80008Fh (ver chwrite.asm). Orden de los
; pixeles: bytes por direccion y, dentro del byte, desde los bits altos.
expect:
        db 011h, 022h, 033h, 044h, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 012h, 034h, 05Eh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 0AAh, 0EEh, 0BBh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 0EEh, 0CCh, 0EEh, 0DDh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 0EEh, 077h, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 001h, 002h, 003h, 004h, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 001h, 000h, 002h, 000h, 003h, 000h, 004h, 000h, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 000h, 001h, 000h, 002h, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
        db 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh, 0EEh
