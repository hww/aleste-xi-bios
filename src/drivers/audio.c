/**
 * @file audio.c
 * @brief AY-3-8910 Audio Driver C Interface Implementation
 */

#include "drivers/audio.h"

// ============================================================================
// ВНЕШНИЕ СИМВОЛЫ ИЗ АССЕМБЛЕРА
// ============================================================================

/* Обязательные методы драйвера */
extern void _audio_detect(void);
extern void _audio_install(void);
extern void _audio_command(void);
extern void _audio_uninstall(void);

/* Семантический API */
extern void _audio_play_note(void);
extern void _audio_stop(void);
extern void _audio_set_volume(void);

/* Управление шумом */
extern void _audio_set_noise_type(void);
extern void _audio_set_noise_custom(void);
extern void _audio_enable_noise(void);

/* Управление огибающей */
extern void _audio_set_envelope(void);
extern void _audio_enable_envelope(void);

/* Управление тоном */
extern void _audio_enable_tone(void);

/* Расширенный доступ */
extern void _audio_read_register(void);
extern void _audio_write_register(void);

/* Вспомогательные функции */
extern void _audio_play_chord(void);
extern void _audio_beep(void);

// ============================================================================
// РЕАЛИЗАЦИЯ ОБЯЗАТЕЛЬНЫХ МЕТОДОВ
// ============================================================================

bool audio_detect(void) __z88dk_fastcall __naked {
    __asm
        jp __audio_detect
    __endasm;
}

uint8_t audio_install(void) __z88dk_fastcall __naked {
    __asm
        jp __audio_install
    __endasm;
}

uint8_t audio_command(uint8_t cmd, uint16_t param) __naked {
    cmd; param;
    __asm
        ld hl, #2
        add hl, sp
        
        ld a, (hl)       ; A = cmd
        inc hl
        ld e, (hl)       ; E = param low
        inc hl
        ld d, (hl)       ; D = param high
        
        ld l, e          ; HL = param
        ld h, d
        
        call __audio_command
        ld l, a
        ret
    __endasm;
}

uint8_t audio_uninstall(void) __z88dk_fastcall __naked {
    __asm
        jp __audio_uninstall
    __endasm;
}

// ============================================================================
// РЕАЛИЗАЦИЯ СЕМАНТИЧЕСКОГО API
// ============================================================================

uint8_t audio_play_note(audio_note_t note, audio_channel_t channel, 
                       audio_volume_t volume) __naked {
    note; channel; volume;
    __asm
        ld hl, #2
        add hl, sp
        
        ; __audio_play_note ожидает: A = нота, B = канал, C = громкость
        ld a, (hl)       ; A = note
        inc hl
        ld b, (hl)       ; B = channel
        inc hl
        ld c, (hl)       ; C = volume
        
        call __audio_play_note
        ld l, a
        ret
    __endasm;
}

uint8_t audio_stop(audio_channel_t channel) __z88dk_fastcall __naked {
    __asm
        ; fastcall: channel уже в L
        ld a, l
        
        call __audio_stop
        ld l, a
        ret
    __endasm;
}

uint8_t audio_set_volume(audio_channel_t channel, audio_volume_t volume) __naked {
    (void)channel; (void)volume;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_set_volume ожидает: A = канал, B = громкость
        ld a, (hl)       ; A = channel
        inc hl
        ld b, (hl)       ; B = volume
        
        call __audio_set_volume
        ld l, a
        ret
    __endasm;
}

// ============================================================================
// УПРАВЛЕНИЕ ШУМОМ
// ============================================================================

uint8_t audio_set_noise_type(audio_noise_t type) __z88dk_fastcall __naked {
    __asm
        ; fastcall: type уже в L
        ld a, l
        
        call __audio_set_noise_type
        ld l, a
        ret
    __endasm;
}

uint8_t audio_set_noise_custom(uint8_t period) __z88dk_fastcall __naked {
    (void)period;
    __asm
        ; fastcall: period уже в L
        ld a, l
        
        call __audio_set_noise_custom
        ld l, a
        ret
    __endasm;
}

uint8_t audio_enable_noise(audio_channel_t channel, bool enable) __naked {
    (void)channel; (void)enable;
    __asm
        ld hl, #2
        add hl, sp
        
        ; __audio_enable_noise ожидает: A = канал, B = enable (0/1)
        ld a, (hl)       ; A = channel
        inc hl
        ld b, (hl)       ; B = enable
        
        call __audio_enable_noise
        ld l, a
        ret
    __endasm;
}

// ============================================================================
// УПРАВЛЕНИЕ ОГИБАЮЩЕЙ
// ============================================================================

uint8_t audio_set_envelope(audio_envelope_t shape, envelope_speed_t speed) __naked {
    shape; speed;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_set_envelope ожидает: A = форма, B = скорость
        ld a, (hl)       ; A = shape
        inc hl
        ld b, (hl)       ; B = speed
        
        call __audio_set_envelope
        ld l, a
        ret
    __endasm;
}

uint8_t audio_enable_envelope(audio_channel_t channel, bool enable) __naked {
    channel; enable;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_enable_envelope ожидает: A = канал, B = enable
        ld a, (hl)       ; A = channel
        inc hl
        ld b, (hl)       ; B = enable
        
        call __audio_enable_envelope
        ld l, a
        ret
    __endasm;
}

// ============================================================================
// УПРАВЛЕНИЕ ТОНОМ
// ============================================================================

uint8_t audio_enable_tone(audio_channel_t channel, bool enable) __naked {
    channel; enable;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_enable_tone ожидает: A = канал, B = enable
        ld a, (hl)       ; A = channel
        inc hl
        ld b, (hl)       ; B = enable
        
        call __audio_enable_tone
        ld l, a
        ret
    __endasm;
}

// ============================================================================
// РАСШИРЕННЫЙ ДОСТУП
// ============================================================================

uint8_t audio_read_register(uint8_t reg) __z88dk_fastcall __naked {
    (void)reg;
    __asm
        ; fastcall: reg уже в L
        ld a, l
        
        call __audio_read_register
        ld l, a
        ret
    __endasm;
}

uint8_t audio_write_register(uint8_t reg, uint8_t value) __naked {
    reg; value;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_write_register ожидает: A = значение, C = регистр
        ld c, (hl)       ; C = reg
        inc hl
        ld a, (hl)       ; A = value
        
        call __audio_write_register
        ld l, a
        ret
    __endasm;
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

uint8_t audio_play_chord(audio_note_t note1, audio_note_t note2, 
                        audio_note_t note3, audio_volume_t volume) __naked {
    note1; note2; note3; volume;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_play_chord ожидает: A = note1, B = note2, C = note3, D = volume
        ld a, (hl)       ; A = note1
        inc hl
        ld b, (hl)       ; B = note2
        inc hl
        ld c, (hl)       ; C = note3
        inc hl
        ld d, (hl)       ; D = volume
        
        call __audio_play_chord
        ld l, a
        ret
    __endasm;
}

uint8_t audio_beep(audio_note_t note, uint16_t duration) __naked {
    note; duration;
    __asm
        ld hl, #2
        add hl, sp
        
        //; __audio_beep ожидает: A = нота, HL = длительность
        ld a, (hl)       ; A = note
        inc hl
        ld e, (hl)       ; E = duration low
        inc hl
        ld d, (hl)       ; D = duration high
        ex de, hl        ; HL = duration
        
        call __audio_beep
        ld l, a
        ret
    __endasm;
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ C-ФУНКЦИИ
// ============================================================================

/**
 * @brief Сыграть ноту по имени (например, "C4", "A#5", "Gb3")
 */
uint8_t audio_play_note_by_name(const char* name, audio_channel_t channel, 
                               audio_volume_t volume) {
    (void)name;
    // TODO: преобразование строки в номер ноты
    // Пока просто заглушка
    return audio_play_note(NOTE_C4, channel, volume);
}

/**
 * @brief Тест всех каналов
 */
uint8_t audio_test_all(void) {
    uint8_t result;
    
    // Тест канала A
    result = audio_play_note(NOTE_C4, AUDIO_CHANNEL_A, VOLUME_NORMAL);
    if (result != AUDIO_ERR_SUCCESS) return result;
    // delay...
    audio_stop(AUDIO_CHANNEL_A);
    
    // Тест канала B
    result = audio_play_note(NOTE_E4, AUDIO_CHANNEL_B, VOLUME_NORMAL);
    if (result != AUDIO_ERR_SUCCESS) return result;
    // delay...
    audio_stop(AUDIO_CHANNEL_B);
    
    // Тест канала C
    result = audio_play_note(NOTE_G4, AUDIO_CHANNEL_C, VOLUME_NORMAL);
    if (result != AUDIO_ERR_SUCCESS) return result;
    // delay...
    audio_stop(AUDIO_CHANNEL_C);
    
    return AUDIO_ERR_SUCCESS;
}