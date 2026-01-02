#ifndef _PORTS_H_
#define _PORTS_H_

#include <stdint.h>
#include "z80ex.h"
#include "mmu_native.h"

uint8_t int_read(Z80EX_CONTEXT *cpu, void *user_data);
uint8_t port_read(Z80EX_CONTEXT *cpu, uint16_t port, void *user_data) ;
void port_write(Z80EX_CONTEXT *cpu, uint16_t port, uint8_t value, void *user_data);
void ports_reset();


#endif //_PORTS_H_