#ifndef CRTC_H
#define CRTC_H

#include <stdint.h>

// 1. Адреса слотов (из memmap.cfg)
#define BIOS_CRTC_VTABLE 0xFD20

// 2. Публичные константы команд
#define CRTC_CMD_WRITE      0x10
#define CRTC_CMD_READ       0x11
#define CRTC_CMD_VSYNC      0x12

// 3. Перечисление режимов (Enum) — ОСТАЕТСЯ ТУТ
// Это не создает кода, это создает только таблицу имен для компилятора
typedef enum {
    CPC_MODE2 = 0, CPC_MODE1, CPC_MODE0,
    LX_16K_1B,     LX_16K_2B,  LX_16K_4B,
    LX_32K_2B,     LX_32K_4B,  LX_32K_8B,
    LX_64K_4B,     LX_64K_8B
} crtc_mode_t;

// 4. Прототипы функций
void crtc_install() __naked;
uint8_t crtc_detect(void);
void    crtc_set_mode(crtc_mode_t mode) __z88dk_fastcall;
void    crtc_set_view(uint8_t slot, uint8_t page);
uint8_t crtc_command(uint8_t cmd, uint8_t reg, uint8_t val);

// Удобные макросы-алиасы
#define crtc_wait_vsync() crtc_command(CRTC_CMD_VSYNC, 0, 0)

#endif