; ============================================================
; vbuf.inc - Интерфейс видеобуфера для ассемблера
; ============================================================

; ---- Адреса системных методов (полиморфный слот) ----
VBUF_INIT          equ 0xFD60  ; Инициализация
VBUF_GET_MODE_INFO equ 0xFD63  ; Получить информацию
VBUF_COMMAND       equ 0xFD66  ; Команда драйвера

; ---- Управление буфером ----
VBUF_CLEAR         equ 0xFD69  ; Очистить весь буфер
VBUF_CLEAR_RECT    equ 0xFD6C  ; Очистить прямоугольник
VBUF_SCROLL_UP     equ 0xFD6F  ; Прокрутка вверх
VBUF_SCROLL_DOWN   equ 0xFD72  ; Прокрутка вниз
VBUF_SET_CLIP_RECT equ 0xFD75  ; Установить отсечение
VBUF_GET_CLIP_RECT equ 0xFD78  ; Получить отсечение
VBUF_COPY_RECT     equ 0xFD7B  ; Копировать область

; ---- Примитивы рисования (0xFD80-0xFD9F) ----
VBUF_GET_ADDR      equ 0xFD80  ; Получить адрес VRAM
VBUF_PUT_PIXEL_FAST equ 0xFD83 ; Нарисовать пиксель (быстро)
VBUF_PUT_PIXEL     equ 0xFD86  ; Нарисовать пиксель (с отсечением)
VBUF_GET_PIXEL     equ 0xFD89  ; Получить пиксель
VBUF_DRAW_HLINE    equ 0xFD8C  ; Горизонтальная линия
VBUF_DRAW_VLINE    equ 0xFD8F  ; Вертикальная линия
VBUF_DRAW_LINE     equ 0xFD92  ; Линия Брезенхэма
VBUF_DRAW_RECT     equ 0xFD95  ; Контур прямоугольника
VBUF_FILL_RECT     equ 0xFD98  ; Заливка прямоугольника

; ---- Работа с текстом и битмапами ----
VBUF_DRAW_CHAR_8X8 equ 0xFD9B  ; Символ 8x8
VBUF_DRAW_BITMAP   equ 0xFD9E  ; Битмап

; ---- Двойная буферизация ----
VBUF_SWAP_BUFFERS  equ 0xFDA1  ; Показать задний буфер
VBUF_GET_BACKBUF   equ 0xFDA4  ; Адрес заднего буфера
VBUF_CLEAR_BACKBUF equ 0xFDA7  ; Очистить задний буфер

; ---- Коды ошибок ----
VBUF_ERR_SUCCESS   equ 0
VBUF_ERR_NOT_FOUND equ 1
VBUF_ERR_INVALID_MODE equ 2
VBUF_ERR_NO_MEMORY equ 3
VBUF_ERR_MMU_FAIL  equ 4

; ---- Флаги режима ----
VBUF_FLAG_INTERLACED   equ 1 << 0
VBUF_FLAG_DOUBLEBUF    equ 1 << 1
VBUF_FLAG_LINEAR       equ 1 << 2

; ---- Команды ----
VBUF_CMD_RESET         equ 0x00
VBUF_CMD_ENABLE_DOUBLEBUF equ 0x01
VBUF_CMD_DISABLE_DOUBLEBUF equ 0x02
VBUF_CMD_SET_BANK      equ 0x03

; ---- Макросы для вызовов ----
macro VBUF_CALL address
    call address
endm

macro VBUF_INIT_DRIVER
    VBUF_CALL VBUF_INIT
endm

macro VBUF_CLEAR_SCREEN color
    ld a, color
    VBUF_CALL VBUF_CLEAR
endm

macro VBUF_DRAW_PIXEL x, y, color
    ld bc, x
    ld de, y
    ld a, color
    VBUF_CALL VBUF_PUT_PIXEL
endm

macro VBUF_FAST_PIXEL x, y, color
    ld bc, x
    ld de, y
    ld a, color
    VBUF_CALL VBUF_PUT_PIXEL_FAST
endm