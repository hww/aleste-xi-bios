; chwrite.asm - escritura y lectura por canales en cada formato, en la pagina
; 80h (800000h). El Z80 llena antes 800000h-8000FFh con EEh y pone 01 02 03 04
; en 800050h. IDX y STEP van en pixeles del formato que tenga el canal
; cuando se escriben: despues de cambiar el formato hay que volver a escribir
; STEP (y IDX), porque el valor guardado no se reescala.
.org 0x20
; (a) 8 bpp, 4 pixeles 11 22 33 44 desde el byte 000h (test_channel_write)
        LDI     R5, #0x0180      ; FMT=1 (8 bpp), PAGE=80h
        MOV     PAGE1, R5
        LDI     R5, #1
        MOV     STEP1, R5
        MOV     IDXH1, #0
        MOV     IDX1, #0
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
        MOV     PAGE1, R5
        LDI     R5, #1
        MOV     STEP1, R5
        LDI     R4, #0x0020
        MOV     IDX1, R4
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
        MOV     PAGE1, R5
        LDI     R5, #2
        MOV     STEP1, R5
        LDI     R4, #0x0020
        MOV     IDX1, R4
        LDI     R2, #0x00AA
        MOV     DATA1, R2
        LDI     R2, #0x00BB
        MOV     DATA1, R2
        FLUSH   1
; (d) lo mismo con el byte alto (lo que era HI8): desde el pixel 31h
        LDI     R4, #0x0031
        MOV     IDX1, R4
        LDI     R2, #0x00CC
        MOV     DATA1, R2
        LDI     R2, #0x00DD
        MOV     DATA1, R2
        FLUSH   1
; (e) 8 bpp, un solo pixel 77h en el pixel 41h (el byte 041h)
        LDI     R5, #0x0180
        MOV     PAGE1, R5
        LDI     R5, #1
        MOV     STEP1, R5
        LDI     R4, #0x0041
        MOV     IDX1, R4
        LDI     R2, #0x0077
        MOV     DATA1, R2
        FLUSH   1
; (f) leer 4 pixeles de 8 bpp desde el byte 050h (canal 2) y escribirlos
;     en 16 bpp desde el byte 060h (canal 3, palabra 30h)
        LDI     R5, #0x0180
        MOV     PAGE2, R5
        LDI     R5, #1
        MOV     STEP2, R5
        MOV     IDXH2, #0
        LDI     R4, #0x0050
        MOV     IDX2, R4
        LDI     R5, #0x0080      ; FMT=0 (16 bpp)
        MOV     PAGE3, R5
        LDI     R5, #1
        MOV     STEP3, R5
        MOV     IDXH3, #0
        LDI     R4, #0x0030
        MOV     IDX3, R4
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
        MOV     PAGE2, R5
        LDI     R5, #1
        MOV     STEP2, R5
        LDI     R4, #0x00A0
        MOV     IDX2, R4
        LDI     R5, #0x0180
        MOV     PAGE1, R5
        LDI     R5, #1
        MOV     STEP1, R5
        LDI     R4, #0x0070
        MOV     IDX1, R4
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
