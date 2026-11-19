; title.asm - microprograma de MAG.COM para la LX-GPU: el titulo al doble.
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; MAG escribe "ALESTE LX" con la fuente de 8x8 en la ultima fila de la
; pantalla (y = 192, x = 88, 72 pixeles) y la GPU lo copia al doble de ancho
; y de alto en y = 4 (16 filas, x = 88, 144 pixeles). Cada fila de destino
; tiene su color (16-31), que MAG va rotando en la paleta: un degradado que se
; mueve. La pantalla es de 8 bpp en C50000h (pagina C5h, 320 x 200).
.org 0x20
        LDI     R0, #0x01C5     ; 8 bpp, pagina C5h
        MOV     PAGE0, R0       ; CH0 lee el texto
        MOV     PAGE1, R0       ; CH1 escribe el titulo
        MOV     IDXH0, #0
        MOV     IDXH1, #0
        MOV     STEP0, #1
        MOV     STEP1, #1
        LDI     R10, #61528     ; origen: 192 * 320 + 88
        LDI     R11, #1368      ; destino: 4 * 320 + 88
        LDI     R12, #16        ; color de la fila
        LDI     R13, #16        ; filas de destino
        LDI     R14, #320
row:    MOV     IDX0, R10
        MOV     IDX1, R11
        LDI     R0, #72
        MOV     LOOP_CNT, R0
        LDI     LOOP_TGT, #px
px:     MOV     R1, DATA0 !f    ; pixel del texto (Z si es fondo)
        MOV     R2, ZERO
        MOV.NE  R2, R12
        MOV     DATA1, R2
      L MOV     DATA1, R2
        ADD     R11, R14        ; siguiente fila de destino
        ADD     R12, #1
        TST     R13, #1         ; cada dos filas, la siguiente del texto
        JMP.NE  odd
        JMP     next
odd:    ADD     R10, R14
next:   SUB     R13, #1
        JMP.NE  row
        FLUSH   0
        FLUSH   1
        HALT    0
