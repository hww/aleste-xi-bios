#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdint.h>
#include "forward_decl.h"
#include <z80ex.h>

enum class ECycle { IOWR, IORD, MWR, MRD, OPCODE };
enum class Mode { LEGACY, NATIVE };

extern Debugger debugger;

// ============================================================
// Debugger
// ============================================================

class Minibuffer;
class IBuffer;
class CLI;
    
extern Minibuffer minibuffer;
extern IBuffer ibuffer;
extern CLI cli;
extern Z88dkSymParser sym_parser;

// ============================================================
// Memory
// ============================================================

#define RAM_SLOT_COUNT 4
#define RAM_SLOT_PAGE_COUNT 256
#define RAM_PAGE_SIZE_BYTES 0x4000
#define RAM_SLOT_SIZE_BYTES (RAM_PAGE_SIZE_BYTES * RAM_SLOT_PAGE_COUNT)
#define TOTAL_RAM_SIZE_BYTES (RAM_SLOT_COUNT * RAM_SLOT_SIZE_BYTES)

extern uint8_t memory[];  // Измените на массив

// ============================================================
// Video
// ============================================================

extern CRTC g_crct;
extern Palette g_palette;

// ============================================================
// Memory Manager
// ============================================================

class NativeMMU;

extern NativeMMU g_mmu;


#endif