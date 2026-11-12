EXTERN _main
EXTERN __int_isr      ; despachador del driver de interrupciones (drivers/int)
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
    
; IM1 прерывание (0x0038): al driver de interrupciones (sin driver, ei/reti)
    jp __int_isr
    
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
    ld sp, 0xBFF0         ; pila arriba de la ventana 2 (memoria de la BIOS; el video
                          ; se dibuja con la DMA y no ocupa ninguna ventana)
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
    ld a, 0x01
    di
    halt
