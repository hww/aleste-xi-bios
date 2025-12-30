ORG 0x0000

start:
    ld a, 0x01        ; Начинаем с бита 0 (00000001b)

main_loop:
    ; Заполняем весь блок 16KB текущим паттерном
    ld hl, 0x8000     ; Начало видеопамяти
    ld bc, 0x4000     ; 16KB = 16384 байт

fill_loop:
    ld (hl), a        ; Записываем паттерн
    inc hl            ; Следующий адрес
    
    ; Уменьшаем счетчик и проверяем окончание
    dec bc
    ld d, a           ; Сохраняем паттерн
    
    ; Проверяем BC == 0?
    ld a, b
    or c
    jr nz, not_done   ; Если не ноль, продолжаем
    
    ; Блок заполнен, меняем паттерн для следующего кадра
    ld a, d           ; Восстанавливаем паттерн
    rlca              ; Следующий паттерн
    
    ; Небольшая задержка
    call delay
    
    ; Начинаем заново с новым паттерном
    jr main_loop

not_done:
    ld a, d           ; Восстанавливаем паттерн
    jr fill_loop      ; Продолжаем заполнение

delay:
    push bc
    ld b, 0x10        ; Настройте для нужной скорости
outer_delay:
    ld c, 0xFF
inner_delay:
    dec c
    jr nz, inner_delay
    djnz outer_delay
    pop bc
    ret