#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <z80ex.h>
#include <stdint.h>
#include "forward_decl.h"



void mem_write(Z80EX_CONTEXT *cpu, uint16_t addr, uint8_t value, void *user_data) ;
uint8_t mem_read(Z80EX_CONTEXT *cpu, uint16_t addr, int m1_state, void *user_data) ;

uint8_t dasm_read_byte(uint16_t addr, void *user_data);
uint32_t dasm_translate_address(uint16_t addr, ECycle cycle);

#endif