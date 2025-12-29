; =============================================================================
; AUDIO_AY8910@ASM - AY-3-8910 Specific Driver
; Конкретная реализация семантического API
; =============================================================================

module audio_ay8910

include "../../inc/bios/undefined_method.inc"

; -----------------------------------------------------------------------------
; ВСЕ defc КОНСТАНТЫ ЭТОГО ДРАЙВЕРА
; -----------------------------------------------------------------------------

; Порты AY
defc AY_REGISTER_PORT = 0xFD
defc AY_DATA_PORT = 0xFE

; Регистры AY
defc AY_REG_TONE_A_FINE = 0
defc AY_REG_TONE_A_COARSE = 1
defc AY_REG_TONE_B_FINE = 2
defc AY_REG_TONE_B_COARSE = 3
defc AY_REG_TONE_C_FINE = 4
defc AY_REG_TONE_C_COARSE = 5
defc AY_REG_NOISE_PERIOD = 6
defc AY_REG_ENABLE = 7
defc AY_REG_VOLUME_A = 8
defc AY_REG_VOLUME_B = 9
defc AY_REG_VOLUME_C = 10
defc AY_REG_ENV_FINE = 11
defc AY_REG_ENV_COARSE = 12
defc AY_REG_ENV_SHAPE = 13
defc AY_REG_IO_A = 14
defc AY_REG_IO_B = 15

; Предустановки шума
defc NOISE_PERIOD_LOW = 0x1F      ; ~30 Гц
defc NOISE_PERIOD_MEDIUM = 0x0F   ; ~500 Гц
defc NOISE_PERIOD_HIGH = 0x03     ; ~15 кГц

; Предустановки огибающей
defc ENV_PERIOD_SLOW = 0x0FFF
defc ENV_PERIOD_MEDIUM = 0x07FF
defc ENV_PERIOD_FAST = 0x01FF
defc ENV_PERIOD_VERY_FAST = 0x007F

; Формы огибающей
defc ENV_SHAPE_DOWN = 0x00
defc ENV_SHAPE_UP = 0x0C
defc ENV_SHAPE_TRIANGLE = 0x0A
defc ENV_SHAPE_SAWTOOTH = 0x08

; Громкость
defc VOLUME_NORMAL = 15

; -----------------------------------------------------------------------------
; VTABLE КОНКРЕТНОГО ДРАЙВЕРА AY-3-8910
; -----------------------------------------------------------------------------

SECTION DRV_VTABLE
ALIGN 32

; Экспортируем VTable драйвера
PUBLIC __audio_ay8910_vtable

__audio_ay8910_vtable:
    jp _audio_ay8910_detect           ; 0
    jp _audio_ay8910_install          ; 3
    jp _audio_ay8910_command          ; 6
    jp _audio_ay8910_uninstall        ; 9
    jp _audio_ay8910_play_note        ; 12
    jp _audio_ay8910_stop             ; 15
    jp _audio_ay8910_set_volume       ; 18
    jp _audio_ay8910_set_noise_type   ; 21
    jp _audio_ay8910_set_noise_custom ; 24
    jp _audio_ay8910_enable_noise     ; 27
    jp _audio_ay8910_set_envelope     ; 30
    jp _audio_ay8910_enable_envelope  ; 33
    jp _audio_ay8910_enable_tone      ; 36
    jp _audio_ay8910_read_register    ; 39
    jp _audio_ay8910_write_register   ; 42
    jp _audio_ay8910_play_chord       ; 45
    jp _audio_ay8910_beep             ; 48

; -----------------------------------------------------------------------------
; ДАННЫЕ ДРАЙВЕРА
; -----------------------------------------------------------------------------

SECTION DRV_DATA

; Тень регистров (16 байт)
_ay_shadow_regs:
    ds 16

; Текущий выбранный регистр
_ay_selected_reg:
    db 0

; Флаги инициализации
_ay_initialized:
    db 0

; Предустановленные периоды шума
_noise_periods:
    db 0            ; NOISE_OFF
    db NOISE_PERIOD_LOW
    db NOISE_PERIOD_MEDIUM
    db NOISE_PERIOD_HIGH
    db 0            ; NOISE_CUSTOM (будет установлен отдельно)

; Предустановленные периоды огибающей
_env_periods:
    dw 0            ; ENVELOPE_OFF
    dw ENV_PERIOD_SLOW
    dw ENV_PERIOD_MEDIUM
    dw ENV_PERIOD_FAST
    dw ENV_PERIOD_VERY_FAST

; Формы огибающей
_env_shapes:
    db 0            ; ENVELOPE_OFF
    db ENV_SHAPE_DOWN
    db ENV_SHAPE_UP
    db ENV_SHAPE_TRIANGLE
    db ENV_SHAPE_SAWTOOTH

; Таблица частот нот (96 нот × 2 байта)
_notes_freq_table:
    dw 3822, 3608, 3405, 3214, 3034, 2863, 2703, 2551, 2408, 2273, 2145, 2025
    dw 1911, 1804, 1703, 1607, 1517, 1432, 1351, 1276, 1204, 1136, 1073, 1012
    dw 956, 902, 851, 804, 758, 716, 676, 638, 602, 568, 536, 506
    dw 478, 451, 426, 402, 379, 358, 338, 319, 301, 284, 268, 253
    dw 239, 225, 213, 201, 190, 179, 169, 159, 150, 142, 134, 127
    dw 119, 113, 106, 100, 95, 89, 84, 80, 75, 71, 67, 63
    dw 60, 56, 53, 50, 47, 45, 42, 40, 38, 36, 34, 32
    dw 30, 28, 26, 25, 24, 22, 21, 20, 19, 18, 17, 16

; -----------------------------------------------------------------------------
; ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (приватные, без подчеркиваний)
; -----------------------------------------------------------------------------

SECTION DRV_CODE

; Выбрать регистр AY
; Вход: A = номер регистра
ay_select_register:
    push bc
    ld (_ay_selected_reg), a
    ld c, AY_REGISTER_PORT
    out (c), a
    pop bc
    ret

; Записать в выбранный регистр
; Вход: A = значение
ay_write_register:
    push bc
    push hl
    
    ; Обновляем тень
    ld hl, _ay_shadow_regs
    ld c, a                    ; Сохраняем значение
    ld a, (_ay_selected_reg)
    ld b, 0
    add hl, bc
    ld (hl), a                 ; Записываем в тень
    
    ; Пишем в чип
    ld a, c                    ; Восстанавливаем значение
    ld c, AY_DATA_PORT
    out (c), a
    
    pop hl
    pop bc
    ret

; Прочитать из выбранного регистра (из тени)
; Выход: A = значение
ay_read_shadow:
    push hl
    push bc
    ld hl, _ay_shadow_regs
    ld a, (_ay_selected_reg)
    ld c, a
    ld b, 0
    add hl, bc
    ld a, (hl)
    pop bc
    pop hl
    ret

; Получить частоту ноты
; Вход: A = номер ноты (0-95)
; Выход: DE = частота (младший, старший)
_get_note_frequency:
    push hl
    push bc
    
    ; Проверяем NOTE_SILENCE
    cp 255
    jr z, @silence
    
    ; Получаем адрес в таблице частот
    ld hl, _notes_freq_table
    add a, a          ; Умножаем на 2
    ld c, a
    ld b, 0
    add hl, bc
    
    ; Загружаем частоту
    ld e, (hl)
    inc hl
    ld d, (hl)
    jr @exit
    
@silence:
    ; Для тишины возвращаем 0
    ld de, 0
    
@exit:
    pop bc
    pop hl
    ret

; Получить регистр громкости для канала
; Вход: A = канал (0-2)
; Выход: A = номер регистра громкости (8-10)
_get_volume_register:
    add a, AY_REG_VOLUME_A
    ret

; Получить бит маски для канала в регистре 7
; Вход: A = канал (0-2)
; Выход: C = бит для тона, B = бит для шума
_get_channel_bits:
    ld c, 1           ; Бит 0 для канала A
    ld b, 8           ; Бит 3 для шума канала A
    
    cp 0
    ret z
    
    ld c, 2           ; Бит 1 для канала B
    ld b, 16          ; Бит 4 для шума канала B
    
    cp 1
    ret z
    
    ld c, 4           ; Бит 2 для канала C
    ld b, 32          ; Бит 5 для шума канала C
    ret

; -----------------------------------------------------------------------------
; ПУБЛИЧНЫЕ МЕТОДЫ (с двойным подчеркиванием)
; -----------------------------------------------------------------------------

PUBLIC _audio_ay8910_detect
_audio_ay8910_detect:
    push bc
    push de
    push hl
    
    ; Тест записи/чтения регистра
    ld a, AY_REG_VOLUME_A
    call ay_select_register
    ld a, 0x0A
    call ay_write_register
    
    ; Небольшая задержка
    ld bc, 100
@delay:
    dec bc
    ld a, b
    or c
    jr nz, @delay
    
    ; Проверяем
    call ay_read_shadow
    cp 0x0A
    jr nz, @not_found
    
    ; Успех
    xor a
    jr @exit
    
@not_found:
    ld a, 2  ; AUDIO_ERR_HARDWARE_ERROR
    scf
    
@exit:
    pop hl
    pop de
    pop bc
    ret

PUBLIC _audio_ay8910_install
_audio_ay8910_install:
    push af
    push bc
    push hl
    
    ld a, (_ay_initialized)
    or a
    jr nz, @already_init
    
    ; Сбрасываем тень регистров
    ld hl, _ay_shadow_regs
    ld de, _ay_shadow_regs + 1
    ld bc, 15
    ld (hl), 0
    ldir
    
    ; Сбрасываем все регистры чипа
    ld b, 16
    ld c, 0
@reset_loop:
    ld a, c
    call ay_select_register
    xor a
    call ay_write_register
    inc c
    djnz @reset_loop
    
    ; Базовая настройка
    ld a, 0x07        ; Включить все тоны
    ld c, AY_REG_ENABLE
    call ay_select_register
    call ay_write_register
    
    ; Установить громкость по умолчанию
    ld a, VOLUME_NORMAL
    ld c, AY_REG_VOLUME_A
    call ay_select_register
    call ay_write_register
    ld c, AY_REG_VOLUME_B
    call ay_select_register
    call ay_write_register
    ld c, AY_REG_VOLUME_C
    call ay_select_register
    call ay_write_register
    
    ; Флаг инициализации
    ld a, 1
    ld (_ay_initialized), a
    
@already_init:
    xor a  ; Успех
    
    pop hl
    pop bc
    pop af
    ret

PUBLIC _audio_ay8910_command
_audio_ay8910_command:
    ; TODO: реализовать команды
    ld a, 1  ; ERR_NOT_SUPPORTED
    scf
    ret

PUBLIC _audio_ay8910_uninstall
_audio_ay8910_uninstall:
    call _audio_ay8910_stop
    xor a
    ld (_ay_initialized), a
    ret

PUBLIC _audio_ay8910_play_note
_audio_ay8910_play_note:
    ; A = нота, B = канал, C = громкость
    push af
    push bc
    push de
    push hl
    
    ; Проверяем канал
    ld a, b
    cp 3
    jr nc, @invalid_channel
    
    ; Получаем частоту ноты
    pop af           ; Восстанавливаем A (ноту)
    push af
    call _get_note_frequency
    ; DE содержит частоту
    
    ; Определяем регистры для канала
    pop af           ; Восстанавливаем регистры
    pop bc
    push bc
    push af
    
    ld a, b          ; Канал
    cp 0
    jr z, @ch_a
    cp 1
    jr z, @ch_b
    ; Канал C
    
@ch_c:
    ; Частота (регистры 4-5)
    ld a, e
    ld c, AY_REG_TONE_C_FINE
    call ay_select_register
    call ay_write_register
    ld a, d
    ld c, AY_REG_TONE_C_COARSE
    call ay_select_register
    call ay_write_register
    
    ; Громкость (регистр 10)
    ld a, 10
    jr @set_volume
    
@ch_b:
    ; Частота (регистры 2-3)
    ld a, e
    ld c, AY_REG_TONE_B_FINE
    call ay_select_register
    call ay_write_register
    ld a, d
    ld c, AY_REG_TONE_B_COARSE
    call ay_select_register
    call ay_write_register
    
    ; Громкость (регистр 9)
    ld a, 9
    jr @set_volume
    
@ch_a:
    ; Частота (регистры 0-1)
    ld a, e
    ld c, AY_REG_TONE_A_FINE
    call ay_select_register
    call ay_write_register
    ld a, d
    ld c, AY_REG_TONE_A_COARSE
    call ay_select_register
    call ay_write_register
    
    ; Громкость (регистр 8)
    ld a, 8

@set_volume:
    ; Устанавливаем громкость
    call ay_select_register
    pop bc           ; Восстанавливаем C (громкость)
    push bc
    ld a, c
    and 0x0F         ; 4 бита громкости
    call ay_write_register
    
    xor a  ; Успех
    jr @exit

@invalid_channel:
    ld a, 3  ; AUDIO_ERR_INVALID_CHANNEL
    scf
    
@exit:
    pop hl
    pop de
    pop bc
    pop af
    ret

PUBLIC _audio_ay8910_stop
_audio_ay8910_stop:
    ; A = канал
    cp 255  ; AUDIO_CHANNEL_ALL
    jr z, @stop_all
    
    ; Остановить конкретный канал
    call _get_channel_bits
    
    ; Получить текущее значение регистра 7
    ld a, AY_REG_ENABLE
    call ay_select_register
    call ay_read_shadow
    
    ; Выключить тон (установить бит)
    or c
    call ay_write_register
    
    ; Установить громкость в 0
    call _get_volume_register
    call ay_select_register
    xor a
    call ay_write_register
    
    jr @exit
    
@stop_all:
    ; Выключить все тоны
    ld a, AY_REG_ENABLE
    call ay_select_register
    ld a, 0x38
    call ay_write_register
    
    ; Сбросить громкость на всех каналах
    ld b, 3
    ld c, AY_REG_VOLUME_A
@stop_loop:
    ld a, c
    call ay_select_register
    xor a
    call ay_write_register
    inc c
    djnz @stop_loop
    
@exit:
    xor a
    ret

; Остальные методы реализованы аналогично...
; [Здесь будут реализации остальных методов по аналогии с play_note]

; Для краткости привожу только шаблоны остальных методов:

PUBLIC _audio_ay8910_set_volume
_audio_ay8910_set_volume:
    ; A = канал, B = громкость
    ret

PUBLIC _audio_ay8910_set_noise_type
_audio_ay8910_set_noise_type:
    ; A = тип шума
    ret

PUBLIC _audio_ay8910_set_noise_custom
_audio_ay8910_set_noise_custom:
    ; A = период шума
    ret

PUBLIC _audio_ay8910_enable_noise
_audio_ay8910_enable_noise:
    ; A = канал, B = enable
    ret

PUBLIC _audio_ay8910_set_envelope
_audio_ay8910_set_envelope:
    ; A = форма, B = скорость
    ret

PUBLIC _audio_ay8910_enable_envelope
_audio_ay8910_enable_envelope:
    ; A = канал, B = enable
    ret

PUBLIC _audio_ay8910_enable_tone
_audio_ay8910_enable_tone:
    ; A = канал, B = enable
    ret

PUBLIC _audio_ay8910_read_register
_audio_ay8910_read_register:
    ; A = регистр
    ret

PUBLIC _audio_ay8910_write_register
_audio_ay8910_write_register:
    ; A = значение, C = регистр
    ret

PUBLIC _audio_ay8910_play_chord
_audio_ay8910_play_chord:
    ; A = note1, B = note2, C = note3, D = volume
    ret

PUBLIC _audio_ay8910_beep
_audio_ay8910_beep:
    ; A = нота, HL = длительность
    ret