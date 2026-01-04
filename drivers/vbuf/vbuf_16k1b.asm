;; ============================================================
;; vbuf_16k1b.asm - Драйвер видеобуфера 640x200 1bpp
;; ============================================================

module vbuf_16k1b

include "../../inc/bios/errors.inc"

; ---- Константы драйвера ----
defc VRAM_BASE      = 0x8000     ; Базовый адрес VRAM
defc MMU_SLOT2_CTL  = 0x7F       ; Порт управления MMU слота 2
defc VRAM_PAGE      = 0x10       ; Номер страницы VRAM
defc SCREEN_WIDTH   = 640
defc SCREEN_HEIGHT  = 200
defc BYTES_PER_LINE = 80         ; 640/8
defc VRAM_SIZE      = 16000      ; 80 * 200

; Команды для vbuf_command
defc VBUF_CMD_RESET          = 0x00
defc VBUF_CMD_ENABLE_DOUBLEBUF = 0x01
defc VBUF_CMD_DISABLE_DOUBLEBUF = 0x02
defc VBUF_CMD_SET_BANK       = 0x03

; Флаги режима
defc VBUF_FLAG_INTERLACED    = 1 << 0
defc VBUF_FLAG_DOUBLEBUF     = 1 << 1
defc VBUF_FLAG_LINEAR        = 1 << 2

; ---- Секция: Таблицы конкретных драйверов ----
section DRV_VTABLE

; Таблица драйвера 16K 1bpp
PUBLIC __drv_16k1b_vtable
__drv_16k1b_vtable:
    jp drv_16k1b_init
    jp drv_16k1b_get_mode_info
    jp drv_16k1b_command
    jp drv_16k1b_clear
    jp drv_16k1b_clear_rect
    jp drv_16k1b_scroll_up
    jp drv_16k1b_scroll_down
    jp drv_16k1b_set_clip_rect
    jp drv_16k1b_get_clip_rect
    jp drv_16k1b_copy_rect
    
    ; Примитивы рисования
    jp drv_16k1b_get_addr
    jp drv_16k1b_put_pixel_fast
    jp drv_16k1b_put_pixel
    jp drv_16k1b_get_pixel
    jp drv_16k1b_draw_hline
    jp drv_16k1b_draw_vline
    jp drv_16k1b_draw_line
    jp drv_16k1b_draw_rect
    jp drv_16k1b_fill_rect
    
    ; Текст и битмапы
    jp drv_16k1b_draw_char_at_addr
    jp drv_16k1b_draw_char
    jp drv_16k1b_draw_bitmap
    
    ; Двойная буферизация
    jp drv_16k1b_swap_buffers
    jp drv_16k1b_get_backbuf
    jp drv_16k1b_clear_backbuf

; ---- Секция: Данные драйвера ----
section DRV_DATA

; Информация о режиме
drv_16k1b_mode_info:
    dw SCREEN_WIDTH      ; width
    dw SCREEN_HEIGHT     ; height
    db 1                 ; bpp (1 бит на пиксель)
    db BYTES_PER_LINE    ; bytes_per_line
    dw VRAM_SIZE         ; vram_size
    db 0                 ; flags (по умолчанию)

; Область отсечения (по умолчанию - весь экран)
drv_16k1b_clip_rect:
    dw 0                 ; x
    dw 0                 ; y
    dw SCREEN_WIDTH      ; w
    dw SCREEN_HEIGHT     ; h

; ---- Секция: Код драйвера ----
section DRV_CODE

; --- Вспомогательные функции ---

; Установить банк VRAM
; Сохраняет: AF, BC
drv_16k1b_set_vram_bank:
    ld a, VRAM_PAGE
    out (MMU_SLOT2_CTL), a
    ret

; Умножение A * B -> DE
drv_16k1b_mult_ab:
    ; A * B -> DE
    ld d, 0
    ld e, b
    ld b, a
    xor a
    ld c, a
@mult_loop:
    add hl, hl
    rla
    jr nc, @mult_no_add
    add hl, de
    adc a, c
@mult_no_add:
    djnz @mult_loop
    ld d, a
    ld e, h
    ret

; --- Обязательные методы драйвера ---

; 1. Инициализация драйвера
drv_16k1b_init:
    call drv_16k1b_set_vram_bank
    xor a
    ret

; 2. Получить информацию о режиме
drv_16k1b_get_mode_info:
    ld hl, drv_16k1b_mode_info
    ret

; 3. Команда драйвера
drv_16k1b_command:
    cp VBUF_CMD_RESET
    jr z, @reset
    cp VBUF_CMD_SET_BANK
    jr z, @set_bank
    
    ; Неподдерживаемая команда
    ld a, ERR_NOT_SUPPORTED
    scf
    ret

@reset:
    call drv_16k1b_init
    xor a
    ret

@set_bank:
    ; param = номер банка
    ld a, l
    out (MMU_SLOT2_CTL), a
    xor a
    ret

; 4. detect (обязательный)
drv_16k1b_detect:
    ; Всегда возвращаем успех, так как это встроенный видеорежим
    ld a, 1
    ret

; 5. install (обязательный)
drv_16k1b_install:
    ; Устанавливаем обработчики, если нужно
    xor a
    ret

; 6. get_info (обязательный)
drv_16k1b_get_info:
    ld hl, drv_16k1b_info
    ret

; 7. uninstall (обязательный)
drv_16k1b_uninstall:
    ; Освобождаем ресурсы
    xor a
    ret

; --- Управление буфером ---

; Очистить весь буфер
; Вход: A = паттерн заполнения (0 или 1)
drv_16k1b_clear:
    call drv_16k1b_set_vram_bank
    
    ; Преобразуем 0/1 в 0x00/0xFF
    or a
    jr z, @clear_zero
    ld a, 0xFF
@clear_zero:
    
    ld hl, VRAM_BASE
    ld (hl), a
    ld d, h
    ld e, l
    inc de
    ld bc, VRAM_SIZE - 1
    ldir
    ret

; Конвертация координат в адрес и битовую маску
; Вход: BC = X, DE = Y
; Выход: HL = адрес VRAM, A = битовая маска
; Использует: BC, DE, HL, A
drv_16k1b_get_addr:
    ; HL = Y * 80
    ld l, e
    ld h, d
    add hl, hl  ; *2
    add hl, hl  ; *4
    add hl, hl  ; *8
    push hl
    add hl, hl  ; *16
    add hl, hl  ; *32
    pop de
    add hl, de  ; *40
    add hl, hl  ; *80
    
    ; Добавить X / 8
    ld a, c
    srl b \ rra  ; BC/2
    srl b \ rra  ; BC/4
    srl b \ rra  ; BC/8
    ld e, a
    ld d, 0
    add hl, de
    
    ; Битовая маска (бит 0 = самый старший бит)
    ld a, c
    and 7
    ld b, a
    ld a, 0x80
    jr z, @mask_done
@mask_shift:
    rrca
    djnz @mask_shift
@mask_done:
    
    ; Добавить базовый адрес
    ld de, VRAM_BASE
    add hl, de
    ret

; Рисование пикселя (быстро, без проверок)
; Вход: BC = X, DE = Y, A = цвет (0/1)
drv_16k1b_put_pixel_fast:
    push af
    call drv_16k1b_set_vram_bank
    call drv_16k1b_get_addr  ; HL = адрес, A = маска бита
    ld c, a                  ; C = битовая маска
    pop af                   ; A = цвет (0/1)
    
    or a
    ld a, c
    jr z, @clear_pixel
    
    ; Установить бит
    or (hl)
    ld (hl), a
    ret
    
@clear_pixel:
    ; Сбросить бит
    cpl
    and (hl)
    ld (hl), a
    ret

; Нарисовать пиксель (с проверкой отсечения)
; Для внешнего API через VTable
; Вход: BC = X, DE = Y, A = цвет (0/1)
; Выход: A = 1 если нарисован, 0 если за пределами отсечения
; Сохраняет: BC, DE, HL
drv_16k1b_put_pixel:
    ; Проверяем, попадает ли точка в область отсечения
    push hl
    push bc
    push de
    
    ; Получаем текущий clip rect
    ld hl, (drv_16k1b_clip_rect)    ; HL = clip.x
    ld a, c
    cp l
    jr c, @outside                  ; X < clip.x
    
    ld a, b
    cp h
    jr nc, @outside                 ; X >= clip.x + clip.w
    
    ; Проверка Y
    ld hl, (drv_16k1b_clip_rect + 4) ; HL = clip.y
    ld a, e
    cp l
    jr c, @outside                  ; Y < clip.y
    
    ld a, d
    cp h
    jr nc, @outside                 ; Y >= clip.y + clip.h
    
    ; Точка внутри отсечения - рисуем
    pop de
    pop bc
    pop hl
    
    ; Вызываем быструю версию
    call drv_16k1b_put_pixel_fast
    ld a, 1                         ; успех
    ret
    
@outside:
    pop de
    pop bc
    pop hl
    xor a                           ; не нарисовано
    ret
    
; Вывод символа по адресу в шрифте
; Вход: HL = адрес символа в шрифте (8 байт)
;       DE = адрес в видеопамяти
drv_16k1b_draw_char_at_addr:
    call drv_16k1b_set_vram_bank
    
    ld b, 8                  ; 8 строк
@char_loop:
    ld a, (hl)               ; байт символа
    ld (de), a               ; записать в видеопамять
    inc hl
    ld a, e
    add a, BYTES_PER_LINE    ; следующая строка
    ld e, a
    jr nc, @no_carry
    inc d
@no_carry:
    djnz @char_loop
    
    ret

; Вывод символа по координатам
; Вход: HL = адрес шрифта
;       A = код символа
;       BC = X (в пикселях)
;       DE = Y (в пикселях)
drv_16k1b_draw_char:
    push hl                  ; сохраняем адрес шрифта
    push af                  ; сохраняем код символа
    
    ; Вычисляем адрес в видеопамяти
    call drv_16k1b_get_addr  ; HL = адрес VRAM для (X,Y)
    ex de, hl                ; DE = адрес VRAM
    
    pop af                   ; A = код символа
    pop hl                   ; HL = адрес шрифта
    
    ; Вычисляем адрес символа в шрифте: HL = шрифт + код * 8
    ld l, a
    ld h, 0
    add hl, hl               ; *2
    add hl, hl               ; *4
    add hl, hl               ; *8
    add hl, hl               ; HL теперь указывает на базу шрифта
    ; на самом деле нужно: HL = шрифт + A * 8
    ; но у нас HL уже содержит шрифт, так что:
    ld c, a
    ld b, 0
    add hl, bc               ; *1
    add hl, bc               ; *2
    add hl, bc               ; *3
    add hl, bc               ; *4
    add hl, bc               ; *5
    add hl, bc               ; *6
    add hl, bc               ; *7
    add hl, bc               ; *8
    
    ; Теперь вызываем рисование по адресу
    jp drv_16k1b_draw_char_at_addr

; Прокрутка экрана вверх на N пикселей
; Вход: A = количество пикселей
drv_16k1b_scroll_up:
    call drv_16k1b_set_vram_bank
    
    ; Конвертируем пиксели в строки (пиксели / 8)
    ld b, a
    srl b \ srl b \ srl b    ; B = строки
    
    ; Вычисляем смещение в байтах: строки * BYTES_PER_LINE
    ld a, BYTES_PER_LINE
    call drv_16k1b_mult_ab   ; DE = смещение
    
    ld hl, VRAM_BASE
    add hl, de               ; HL = начало области копирования
    ld de, VRAM_BASE         ; DE = начало назначения
    
    ; Вычисляем сколько копировать: весь экран минус смещение
    ld bc, VRAM_SIZE
    sbc hl, de               ; HL = VRAM_SIZE - смещение
    ld b, h
    ld c, l                  ; BC = размер для копирования
    
    ldir                     ; копируем
    
    ; Очищаем освободившуюся область
    ex de, hl                ; HL = начало области очистки
    ld (hl), 0
    push hl
    pop de
    inc de
    dec bc                   ; BC = размер области очистки - 1
    ldir
    
    ret

; Прокрутка экрана вниз на N пикселей
; Вход: A = количество пикселей
drv_16k1b_scroll_down:
    call drv_16k1b_set_vram_bank
    
    ; Если 0 пикселей - ничего не делаем
    or a
    ret z
    
    ; Конвертируем пиксели в строки (должно быть кратно 8)
    ; A уже должно быть кратно 8 вызывающим кодом
    ld b, a
    srl b \ srl b \ srl b    ; B = количество строк
    jr z, @done              ; Если 0 строк
    
    ; Вычисляем смещение в байтах: строки * BYTES_PER_LINE
    ld a, BYTES_PER_LINE
    call drv_16k1b_mult_ab   ; DE = смещение
    
    ; Копируем область вниз
    ; Размер для копирования = VRAM_SIZE - смещение
    ld hl, VRAM_SIZE
    ld a, l
    sub e
    ld c, a
    ld a, h
    sbc a, d
    ld b, a                  ; BC = VRAM_SIZE - DE
    
    ; Источник: VRAM_BASE
    ; Назначение: VRAM_BASE + смещение
    ld hl, VRAM_BASE
    push hl
    pop de
    add hl, de               ; HL = VRAM_BASE + DE (назначение)
    ex de, hl                ; DE = назначение
    ld hl, VRAM_BASE         ; HL = источник
    
    ; Копируем
    ldir
    
    ; Очищаем верхнюю часть
    ld hl, VRAM_BASE
    ld (hl), 0
    push hl
    pop de
    inc de
    dec de                   ; DE = смещение - 1
    dec de
    ld c, e
    ld b, d
    ldir
    
@done:
    ret

; --- Заглушки для остальных методов (реализовать по необходимости) ---

drv_16k1b_clear_rect:
drv_16k1b_set_clip_rect:
drv_16k1b_get_clip_rect:
drv_16k1b_copy_rect:
drv_16k1b_get_pixel:
drv_16k1b_draw_hline:
drv_16k1b_draw_vline:
drv_16k1b_draw_line:
drv_16k1b_draw_rect:
drv_16k1b_fill_rect:
drv_16k1b_draw_bitmap:
drv_16k1b_swap_buffers:
drv_16k1b_get_backbuf:
drv_16k1b_clear_backbuf:
    xor a
    ret

; Информация о драйвере (обязательная)
section DRV_DATA
drv_16k1b_info:
    dw @name
    db 1, 0        ; версия 1.0
    dw 0           ; флаги (API_STATIC_DEVICE)
    db 0           ; вектор прерывания
    db 0           ; резерв
@name:
    db "VBUF 640x200 1bpp v1.0", 0