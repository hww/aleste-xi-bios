#include "globals.h"
#include "utils/minibuffer.h"
#include "utils/ibuffer.h"
#include "utils/z88dk_sym_parser.h"
#include "palette.h"
#include "crtc.h"
#include "cli.h"
#include "mmu_native.h"
#include "debugger.h"
#include <z80ex.h>

// ============================================================
// Глобальные переменные
// ============================================================

Debugger debugger;
Z88dkSymParser sym_parser;
Z80EX_CONTEXT *cpu = NULL;

extern Z80EX_CONTEXT *cpu;
// ============================================================
// Screen buffers
// ============================================================

Minibuffer minibuffer;
IBuffer ibuffer = IBuffer(512,14);

// ============================================================
// Command Shell
// ============================================================

CLI cli;

// ============================================================
// Memory Dump
// ============================================================

uint8_t memory[TOTAL_RAM_SIZE_BYTES];

NativeMMU g_mmu = NativeMMU(memory);

// ============================================================
// Memory Dump
// ============================================================

Palette g_palette;

// ============================================================
// Memory Dump
// ============================================================

// Инициализация при объявлении
CRTC g_crct = CRTC();

