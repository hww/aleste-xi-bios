; chwrite.asm - escritura y lectura por canales en cada formato, en la pagina
; 80h (800000h). El Z80 llena antes 800000h-8000FFh con EEh y pone 01 02 03 04
; en 800050h. Canales v2 en modo lineal: XI (indice) y DXI (paso) van en
; pixeles; la direccion se calcula con el formato que tenga el canal.
.org 0x20
; (a) 8 bpp, 4 pixeles 11 22 33 44 desde el byte 000h (test_channel_write)
        LDI     R5, #0x0180      ; FMT=1 (8 bpp), PAGE=80h
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        MOV     YI1, #0
        MOV     XI1, #0
        LDI     R2, #0x0011
        MOV     DATA1, R2
        LDI     R2, #0x0022
        MOV     DATA1, R2
        LDI     R2, #0x0033
        MOV     DATA1, R2
        LDI     R2, #0x0044
        MOV     DATA1, R2
        FLUSH   1
; (b) 4 bpp, 5 pixeles 1 2 3 4 5 desde el byte 010h (pixel 20h)
        LDI     R5, #0x0280      ; FMT=2 (4 bpp)
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        LDI     R4, #0x0020
        MOV     XI1, R4
        LDI     R2, #1
        MOV     DATA1, R2
        LDI     R2, #2
        MOV     DATA1, R2
        LDI     R2, #3
        MOV     DATA1, R2
        LDI     R2, #4
        MOV     DATA1, R2
        LDI     R2, #5
        MOV     DATA1, R2
        FLUSH   1
; (c) un byte por palabra (lo que era LO8): 8 bpp con STEP 2 desde el byte
;     bajo de la palabra del byte 020h (pixel 20h): AAh y BBh
        LDI     R5, #0x0180      ; FMT=1 (8 bpp)
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #2
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        LDI     R4, #0x0020
        MOV     XI1, R4
        LDI     R2, #0x00AA
        MOV     DATA1, R2
        LDI     R2, #0x00BB
        MOV     DATA1, R2
        FLUSH   1
; (d) lo mismo con el byte alto (lo que era HI8): desde el pixel 31h
        LDI     R4, #0x0031
        MOV     XI1, R4
        LDI     R2, #0x00CC
        MOV     DATA1, R2
        LDI     R2, #0x00DD
        MOV     DATA1, R2
        FLUSH   1
; (e) 8 bpp, un solo pixel 77h en el pixel 41h (el byte 041h)
        LDI     R5, #0x0180
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        LDI     R4, #0x0041
        MOV     XI1, R4
        LDI     R2, #0x0077
        MOV     DATA1, R2
        FLUSH   1
; (f) leer 4 pixeles de 8 bpp desde el byte 050h (canal 2) y escribirlos
;     en 16 bpp desde el byte 060h (canal 3, palabra 30h)
        LDI     R5, #0x0180
        MOV     BANK, #4
        MOV     MODE_PAGE2, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI2, R5
        MOV     BANK, #0
        MOV     YI2, #0
        LDI     R4, #0x0050
        MOV     XI2, R4
        LDI     R5, #0x0080      ; FMT=0 (16 bpp)
        MOV     BANK, #4
        MOV     MODE_PAGE3, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI3, R5
        MOV     BANK, #0
        MOV     YI3, #0
        LDI     R4, #0x0030
        MOV     XI3, R4
        MOV     R2, DATA2
        MOV     DATA3, R2
        MOV     R2, DATA2
        MOV     DATA3, R2
        MOV     R2, DATA2
        MOV     DATA3, R2
        MOV     R2, DATA2
        MOV     DATA3, R2
        FLUSH   3
; (g) leer 4 pixeles de 4 bpp desde el byte 050h (pixel A0h) y escribirlos
;     en 8 bpp desde el byte 070h (canal 1, pixel 70h)
        LDI     R5, #0x0280
        MOV     BANK, #4
        MOV     MODE_PAGE2, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI2, R5
        MOV     BANK, #0
        LDI     R4, #0x00A0
        MOV     XI2, R4
        LDI     R5, #0x0180
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #1
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        LDI     R4, #0x0070
        MOV     XI1, R4
        MOV     R2, DATA2
        MOV     DATA1, R2
        MOV     R2, DATA2
        MOV     DATA1, R2
        MOV     R2, DATA2
        MOV     DATA1, R2
        MOV     R2, DATA2
        MOV     DATA1, R2
        FLUSH   1
        HALT
