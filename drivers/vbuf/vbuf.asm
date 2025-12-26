SECTION VBUF_SYSTEM
ORG 0xFD60  ; Фиксируем адрес таблицы в ОЗУ

PUBLIC vbuf_vtable
vbuf_vtable:
; SLOT 1 (0xFD60) - Управление
vbuf_init:       jp 0    ; +0
vbuf_get_info:   jp 0    ; +3
vbuf_set_vram:   jp 0    ; +6
vbuf_clear:      jp 0    ; +9
vbuf_scroll_up:  jp 0    ; +12
                 ds 17   ; Reserved

ORG 0xFD80
; SLOT 2 (0xFD80) - Рисование
vbuf_get_addr:   jp 0    ; +0
vbuf_draw_char:  jp 0    ; +3
vbuf_put_pixel:  jp 0    ; +6
vbuf_draw_block: jp 0    ; +9
                 ds 20   ; Reserved

SECTION VBUF_CODE
; Выбор драйвера: HL = адрес таблицы-шаблона
PUBLIC vbuf_select_driver
vbuf_select_driver:
    ld de, 0xFD60
    ld bc, 64           ; Копируем 2 слота
    ldir
    call vbuf_init      ; Сразу инициализируем
    ret