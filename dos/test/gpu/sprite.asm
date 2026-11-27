; sprite.asm - test_sprite de lx-gpu-rtl en la pagina 80h (800000h) en vez de la 0:
; copia un sprite de 16x16 de 8 bpp (pixel 400h) sobre la pantalla de 320x200
; (pixel 800h) en X=100, Y=100, saltando los pixeles a 0 (transparentes).
; Otro cambio: FLUSH 1 al final (el canal que escribe) en vez de FLUSH 2.
.org 0x20
        ; --- Канал 0: источник (спрайт 16x16 по адресу 0x0400) ---
        LDI     R5, #0x0180      ; FMT=1 (8bpp), PAGE=80h
        MOV     BANK, #4
        MOV     MODE_PAGE0, R5
        MOV     BANK, #0
        LDI     R5, #1           ; STEP0 = 1 пиксель
        MOV     BANK, #1
        MOV     DXI0, R5
        MOV     BANK, #0
        MOV     YI0, #0
        LDI     R4, #0x0400      ; IDX0 = 0x0400
        MOV     XI0, R4

        ; --- Канал 1: экран 320x200 (база 0x0800) ---
        LDI     R5, #0x0180      ; FMT=1 (8bpp), PAGE=80h
        MOV     BANK, #4
        MOV     MODE_PAGE1, R5
        MOV     BANK, #0
        LDI     R5, #1           ; STEP1 = 1 пиксель
        MOV     BANK, #1
        MOV     DXI1, R5
        MOV     BANK, #0
        MOV     YI1, #0

        ; --- Настройка координат и аппаратного цикла ---
        LDI     R3, #0x8564      ; (X=100, Y=100) -> 0x0800 + 100*320 + 100
        LDI     R6, #320         ; ширина экрана
        LDI     R1, #16          ; внешний счётчик: 16 строк
        LDI     R0, #16          ; ширина спрайта: 16 пикселей
        LDI     R4, #col_loop    ; адрес col_loop
        MOV     LOOP_TGT, R4

row_loop:
        MOV     R7, R3           ; R7 = текущий адрес пикселя в строке
        MOV     LOOP_CNT, R0     ; запуск аппаратного цикла на 16 пикселей

col_loop:
        MOV     R2, DATA0        ; читаем пиксель спрайта
        OR      R2, R2           ; проверяем на прозрачность (0)
        JMP.EQ  skip_pixel       ; если 0 — пропускаем запись
        MOV     XI1, R7         ; ставим адрес приёмника
        MOV     DATA1, R2        ; пишем непрозрачный пиксель
skip_pixel:
      L ADD     R7, #1           ; конец тела цикла (L-префикс!) -> возврат на 0x034

        ADD     R3, R6           ; следующая строка экрана (R3 += 320)
        SUB     R1, #1           ; уменьшаем счётчик строк
        JMP.NE  row_loop         ; повторяем 16 строк

        FLUSH   1                ; el canal 1 (FLUSH 2 en el original:
                                 ; el ultimo pixel no llegaba a la memoria)
        HALT
