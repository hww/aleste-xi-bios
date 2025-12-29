/**
 * @file audio.h
 * @brief AY-3-8910 Audio Driver API (Semantic)
 * @version 1.0
 * 
 * Семантический API для работы с AY-3-8910/YM2149.
 * Предоставляет понятный интерфейс вместо работы с регистрами.
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ОСНОВНЫЕ ТИПЫ
// ============================================================================

/** Каналы AY */
typedef enum {
    AUDIO_CHANNEL_A = 0,
    AUDIO_CHANNEL_B = 1,
    AUDIO_CHANNEL_C = 2,
    AUDIO_CHANNEL_ALL = 255
} audio_channel_t;

/** Ноты (по MIDI но без октав 8+) */
typedef enum {
    NOTE_C0 = 0,   NOTE_Cs0, NOTE_D0, NOTE_Ds0, NOTE_E0, NOTE_F0, 
    NOTE_Fs0, NOTE_G0, NOTE_Gs0, NOTE_A0, NOTE_As0, NOTE_B0,
    
    NOTE_C1, NOTE_Cs1, NOTE_D1, NOTE_Ds1, NOTE_E1, NOTE_F1,
    NOTE_Fs1, NOTE_G1, NOTE_Gs1, NOTE_A1, NOTE_As1, NOTE_B1,
    
    NOTE_C2, NOTE_Cs2, NOTE_D2, NOTE_Ds2, NOTE_E2, NOTE_F2,
    NOTE_Fs2, NOTE_G2, NOTE_Gs2, NOTE_A2, NOTE_As2, NOTE_B2,
    
    NOTE_C3, NOTE_Cs3, NOTE_D3, NOTE_Ds3, NOTE_E3, NOTE_F3,
    NOTE_Fs3, NOTE_G3, NOTE_Gs3, NOTE_A3, NOTE_As3, NOTE_B3,
    
    NOTE_C4, NOTE_Cs4, NOTE_D4, NOTE_Ds4, NOTE_E4, NOTE_F4,
    NOTE_Fs4, NOTE_G4, NOTE_Gs4, NOTE_A4, NOTE_As4, NOTE_B4,
    
    NOTE_C5, NOTE_Cs5, NOTE_D5, NOTE_Ds5, NOTE_E5, NOTE_F5,
    NOTE_Fs5, NOTE_G5, NOTE_Gs5, NOTE_A5, NOTE_As5, NOTE_B5,
    
    NOTE_C6, NOTE_Cs6, NOTE_D6, NOTE_Ds6, NOTE_E6, NOTE_F6,
    NOTE_Fs6, NOTE_G6, NOTE_Gs6, NOTE_A6, NOTE_As6, NOTE_B6,
    
    NOTE_C7, NOTE_Cs7, NOTE_D7, NOTE_Ds7, NOTE_E7, NOTE_F7,
    NOTE_Fs7, NOTE_G7, NOTE_Gs7, NOTE_A7, NOTE_As7, NOTE_B7,
    
    NOTE_SILENCE = 255  // Тишина
} audio_note_t;

/** Громкость (0-15) */
typedef uint8_t audio_volume_t;
#define VOLUME_MIN   0
#define VOLUME_MAX   15
#define VOLUME_MUTE  0
#define VOLUME_FULL  15

/** Типы шума */
typedef enum {
    NOISE_OFF = 0,      /**< Шум выключен */
    NOISE_LOW,          /**< Низкочастотный шум (~30 Гц) */
    NOISE_MEDIUM,       /**< Среднечастотный шум (~500 Гц) */
    NOISE_HIGH,         /**< Высокочастотный шум (~15 кГц) */
    NOISE_CUSTOM        /**< Пользовательская частота */
} audio_noise_t;

/** Формы огибающей */
typedef enum {
    ENVELOPE_OFF = 0,   /**< Огибающая выключена */
    ENVELOPE_DOWN,      /**< Спад: \______ */
    ENVELOPE_UP,        /**< Подъем: /______ */
    ENVELOPE_TRIANGLE,  /**< Треугольник: /\/\/\ */
    ENVELOPE_SAWTOOTH   /**< Пила: /|/|/| */
} audio_envelope_t;

/** Скорость огибающей (условные единицы) */
typedef enum {
    ENVELOPE_SLOW = 0,  /**< Медленно */
    ENVELOPE_MEDIUM,    /**< Средне */
    ENVELOPE_FAST,      /**< Быстро */
    ENVELOPE_VERY_FAST  /**< Очень быстро */
} envelope_speed_t;

// ============================================================================
// ОБЯЗАТЕЛЬНЫЕ МЕТОДЫ ДРАЙВЕРА
// ============================================================================

/**
 * @brief Проверить наличие аудио устройства
 * @return true если устройство обнаружено, false в противном случае
 */
bool audio_detect(void) __z88dk_fastcall __naked;

/**
 * @brief Инициализировать аудио драйвер
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_install(void) __z88dk_fastcall __naked;

/**
 * @brief Выполнить команду драйвера
 * @param cmd Код команды
 * @param param Параметр команды
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_command(uint8_t cmd, uint16_t param) __naked;

/**
 * @brief Деинициализировать аудио драйвер
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_uninstall(void) __z88dk_fastcall __naked;

// ============================================================================
// СЕМАНТИЧЕСКИЙ API (ОСНОВНОЙ)
// ============================================================================

/**
 * @brief Воспроизвести ноту на канале
 * @param note Номер ноты (0-95 или NOTE_SILENCE)
 * @param channel Канал (A, B, C или ALL)
 * @param volume Громкость (0-15)
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_play_note(audio_note_t note, audio_channel_t channel, 
                       audio_volume_t volume) __naked;

/**
 * @brief Остановить воспроизведение
 * @param channel Канал (A, B, C или ALL)
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_stop(audio_channel_t channel) __z88dk_fastcall __naked;

/**
 * @brief Установить громкость канала
 * @param channel Канал (A, B, C или ALL)
 * @param volume Громкость (0-15)
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_set_volume(audio_channel_t channel, audio_volume_t volume) __naked;

// ============================================================================
// УПРАВЛЕНИЕ ШУМОМ
// ============================================================================

/**
 * @brief Установить тип шума
 * @param type Тип шума (OFF, LOW, MEDIUM, HIGH, CUSTOM)
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_set_noise_type(audio_noise_t type) __z88dk_fastcall __naked;

/**
 * @brief Установить пользовательскую частоту шума
 * @param period Период шума (0-31)
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_set_noise_custom(uint8_t period) __z88dk_fastcall __naked;

/**
 * @brief Включить/выключить шум на канале
 * @param channel Канал (A, B, C)
 * @param enable true - включить, false - выключить
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_enable_noise(audio_channel_t channel, bool enable) __naked;

// ============================================================================
// УПРАВЛЕНИЕ ОГИБАЮЩЕЙ
// ============================================================================

/**
 * @brief Установить огибающую
 * @param shape Форма огибающей
 * @param speed Скорость огибающей
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_set_envelope(audio_envelope_t shape, envelope_speed_t speed) __naked;

/**
 * @brief Включить/выключить огибающую на канале
 * @param channel Канал (A, B, C)
 * @param enable true - включить, false - выключить
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_enable_envelope(audio_channel_t channel, bool enable) __naked;

// ============================================================================
// УПРАВЛЕНИЕ ТОНОМ
// ============================================================================

/**
 * @brief Включить/выключить тон на канале
 * @param channel Канал (A, B, C)
 * @param enable true - включить, false - выключить
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_enable_tone(audio_channel_t channel, bool enable) __naked;

// ============================================================================
// РАСШИРЕННЫЙ ДОСТУП (ТОЛЬКО ДЛЯ ЭКСПЕРТОВ)
// ============================================================================

/**
 * @brief Прочитать регистр AY (расширенный доступ)
 * @param reg Номер регистра (0-15)
 * @return Значение регистра
 */
uint8_t audio_read_register(uint8_t reg) __z88dk_fastcall __naked;

/**
 * @brief Записать регистр AY (расширенный доступ)
 * @param reg Номер регистра (0-15)
 * @param value Значение
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_write_register(uint8_t reg, uint8_t value) __naked;

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Воспроизвести аккорд (3 ноты одновременно)
 * @param note1 Первая нота
 * @param note2 Вторая нота
 * @param note3 Третья нота
 * @param volume Громкость
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_play_chord(audio_note_t note1, audio_note_t note2, 
                        audio_note_t note3, audio_volume_t volume) __naked;

/**
 * @brief Сыграть мелодию из одной ноты (для тестов)
 * @param note Нота
 * @param duration Длительность в системных тиках
 * @return Код ошибки (0 = успех)
 */
uint8_t audio_beep(audio_note_t note, uint16_t duration) __naked;

// ============================================================================
// УДОБНЫЕ КОНСТАНТЫ
// ============================================================================

/** Коды ошибок */
#define AUDIO_ERR_SUCCESS          0
#define AUDIO_ERR_NOT_INITIALIZED  1
#define AUDIO_ERR_HARDWARE_ERROR   2
#define AUDIO_ERR_INVALID_CHANNEL  3
#define AUDIO_ERR_INVALID_PARAM    4

/** Предустановленные громкости */
#define VOLUME_QUIET   4
#define VOLUME_NORMAL  8
#define VOLUME_LOUD    12
#define VOLUME_LOUDEST 15

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_H__ */