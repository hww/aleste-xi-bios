#ifndef ALESTE_H
#define ALESTE_H

/*
 * ALESTE BIOS - Главный заголовочный файл
 * Версия 1.0
 */

// ==============================================================================
// ВЕРСИЯ СИСТЕМЫ
// ==============================================================================
#define ALESTE_BIOS_VERSION_MAJOR 1
#define ALESTE_BIOS_VERSION_MINOR 0
#define ALESTE_BIOS_VERSION_PATCH 0

// ==============================================================================
// БАЗОВЫЕ ТИПЫ (для совместимости с C)
// ==============================================================================
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef bool
#define bool uint8_t
#define true 1
#define false 0
#endif

// ==============================================================================
// ВКЛЮЧЕНИЕ ПОДСИСТЕМ
// ==============================================================================

// Железо-специфичные определения
#include "hardware/ports.h"
#include "hardware/memory.h"

// Подсистемы BIOS
#include "bios/errors.h"
#include "bios/drivers.h"

// Драйверы (можно включать выборочно)
#ifdef USE_MMU
#include "bios/mmu.h"
#endif

#ifdef USE_VIDEO
#include "bios/video.h"
#endif

// ==============================================================================
// МАКРОСЫ ДЛЯ АССЕМБЛЕРА (общие)
// ==============================================================================

// Быстрое сохранение регистров
#define SAVE_REGS __asm push af \ push bc \ push de \ push hl \ push ix \ push iy __endasm
#define RESTORE_REGS __asm pop iy \ pop ix \ pop hl \ pop de \ pop bc \ pop af __endasm

// Отключение/включение прерываний
#define DI __asm di __endasm
#define EI __asm ei __endasm

// Пустая операция (для выравнивания)
#define NOP __asm nop __endasm

// ==============================================================================
// СИСТЕМНЫЕ ВЫЗОВЫ BIOS
// ==============================================================================

// Базовые сервисы BIOS
extern void bios_init(void);      // Инициализация BIOS
extern void bios_version(void);   // Получить версию BIOS
extern void bios_memory_test(void); // Тест памяти

#endif // ALESTE_H