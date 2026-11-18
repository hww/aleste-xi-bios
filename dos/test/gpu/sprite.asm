; sprite.asm - test_sprite de lx-gpu-rtl en la pagina 80h (800000h) en vez de la 0:
; copia un sprite de 16x16 de 8 bpp (pixel 400h) sobre la pantalla de 320x200
; (pixel 800h) en X=100, Y=100, saltando los pixeles a 0 (transparentes).
; Otro cambio: FLUSH 1 al final (el canal que escribe) en vez de FLUSH 2.
.org 0x20
        ; --- Канал 0: источник (спрайт 16x16 по адресу 0x0400) ---
        LDI     R5, #0x0180      ; PC=0x020: FMT=1 (8bpp), PAGE=80h
        MOV     PAGE0, R5        ; PC=0x021
        LDI     R5, #1           ; PC=0x022: STEP0 = 1 пиксель
        MOV     STEP0, R5        ; PC=0x023
        MOV     IDXH0, #0        ; PC=0x024
        LDI     R4, #0x0400      ; PC=0x025: IDX0 = 0x0400
        MOV     IDX0, R4         ; PC=0x026

        ; --- Канал 1: экран 320x200 (база 0x0800) ---
        LDI     R5, #0x0180      ; PC=0x027: FMT=1 (8bpp), PAGE=80h
        MOV     PAGE1, R5        ; PC=0x028
        LDI     R5, #1           ; PC=0x029: STEP1 = 1 пиксель
        MOV     STEP1, R5        ; PC=0x02A
        MOV     IDXH1, #0        ; PC=0x02B

        ; --- Настройка координат и аппаратного цикла ---
        LDI     R3, #0x8564      ; PC=0x02C: (X=100, Y=100) -> 0x0800 + 100*320 + 100
        LDI     R6, #320         ; PC=0x02D: ширина экрана
        LDI     R1, #16          ; PC=0x02E: внешний счётчик: 16 строк
        LDI     R0, #16          ; PC=0x02F: ширина спрайта: 16 пикселей
        LDI     R4, #0x0034      ; PC=0x030: адрес col_loop (PC=0x034)
        MOV     LOOP_TGT, R4     ; PC=0x031

row_loop:
        MOV     R7, R3           ; PC=0x032: R7 = текущий адрес пикселя в строке
        MOV     LOOP_CNT, R0     ; PC=0x033: запуск аппаратного цикла на 16 пикселей

col_loop:
        MOV     R2, DATA0        ; PC=0x034: читаем пиксель спрайта
        OR      R2, R2           ; PC=0x035: проверяем на прозрачность (0)
        JMP.EQ  skip_pixel       ; PC=0x036: если 0 — пропускаем запись
        MOV     IDX1, R7         ; PC=0x037: ставим адрес приёмника
        MOV     DATA1, R2        ; PC=0x038: пишем непрозрачный пиксель
skip_pixel:
      L ADD     R7, #1           ; PC=0x039: конец тела цикла (L-префикс!) -> возврат на 0x034

        ADD     R3, R6           ; PC=0x03A: следующая строка экрана (R3 += 320)
        SUB     R1, #1           ; PC=0x03B: уменьшаем счётчик строк
        JMP.NE  row_loop         ; PC=0x03C: повторяем 16 строк

        FLUSH   1                ; PC=0x03D: el canal 1 (FLUSH 2 en el original:
                                 ; el ultimo pixel no llegaba a la memoria)
        HALT                     ; PC=0x03E
