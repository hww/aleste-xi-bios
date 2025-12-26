SECTION VBUF_DRV_16K1B

; Константы Aleste MMU (примерные для слота 0x8000)
MMU_SLOT2_CTL:  equ 0x7F    ; Порт управления Slot 2 (0x8000-0xBFFF)
VRAM_PAGE:      equ 0x10    ; Предположим, VRAM начинается с 16-й страницы

_pitch: equ 80
_vram:  equ 0x8000

; Таблица-шаблон
PUBLIC drv_16k1b_table
drv_16k1b_table:
    jp drv_init          ; 0xFD60
    jp drv_get_info      ; +3
    jp drv_ensure_mmu    ; +6 (set_vram теперь работает как ensure)
    jp drv_clear         ; +9
    jp drv_scroll_up     ; +12
    ALIGN 32
    jp drv_get_addr      ; 0xFD80
    jp drv_draw_char     ; +3
    jp drv_put_pixel     ; +6
    jp drv_draw_block    ; +9

; --- Вспомогательная функция (Ensure MMU) ---
drv_ensure_mmu:
    ld a, VRAM_PAGE
    out (MMU_SLOT2_CTL), a
    ret

drv_init:
    call drv_ensure_mmu
    ; Тут настройка CRTC в режим 640x200
    ret

drv_clear:
    ; A = pattern (fastcall передает в L, но мы можем перехватить)
    call drv_ensure_mmu
    ld hl, _vram
    ld (hl), a
    ld d, h \ ld e, l \ inc de
    ld bc, 16000-1
    ldir
    ret

drv_get_addr:
    ; BC=X, DE=Y. MMU тут не нужен, только расчет
    ; HL = Y*80 + X/8
    ld l, e \ ld h, d
    add hl, hl \ add hl, hl \ add hl, hl \ add hl, hl ; *16
    push hl
    add hl, hl \ add hl, hl ; *64
    pop de
    add hl, de ; HL = Y*80
    ld a, c
    srl b \ rra \ srl b \ rra \ srl b \ rra ; X/8
    ld e, a \ ld d, 0
    add hl, de
    ld de, _vram
    add hl, de
    ret

drv_draw_char:
    ; HL=VRAM, DE=Font. Рисование ТРЕБУЕТ MMU
    call drv_ensure_mmu
    ld b, 8
.loop:
    ld a, (de)
    ld (hl), a
    inc de
    push de
    ld de, _pitch
    add hl, de
    pop de
    djnz .loop
    ret

drv_scroll_up:
    call drv_ensure_mmu
    ld de, _vram
    ld hl, _vram + (_pitch * 8)
    ld bc, 16000 - (_pitch * 8)
    ldir
    ; Очистка нижней строки
    ld h, d \ ld l, e
    ld (hl), 0
    inc de
    ld bc, (_pitch * 8) - 1
    ldir
    ret

drv_get_info:
    ld hl, _info
    ret

_info: dw 640, 200 \ db 80, 1