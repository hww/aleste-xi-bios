EXTERN _main
SECTION code_crt_start
; aleste.asm
; z80asm синтаксис

; =============== ВЕКТОРА ===============
; RST 00h
    di
    jp start
    
; Заполняем до 0x0038 (правильный синтаксис)
    defs 0x0038 - $      ; без значения - заполнит 0x00
    ; ИЛИ
    ; defs 0x0038 - $, 0xFF  ; заполнит 0xFF
    
; IM1 прерывание (0x0038)
    ei
    reti
    
; Заполняем до 0x0066
    defs 0x0066 - $
    
; NMI (0x0066)
    retn
    
; Заполняем до 0x8000
    defs 0x100 - $

; =============== ОСНОВНОЙ КОД ===============
start:
    ; Инициализация Aleste
    di
    ld sp, 0x7F00
    ld a, 0x00
    out (0xFC), a
    ld a, 0x01
    out (0xFD), a
    ld a, 0x02
    out (0xFE), a
    ld a, 0x03
    out (0xFF), a

    
    ; Вызов C main()
    call _main
    
    ; Завершение
    di
    halt
