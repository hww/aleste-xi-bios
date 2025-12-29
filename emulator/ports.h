#ifndef _PORTS_H_
#define _PORTS_H_

#include <stdint.h>
#include <z80ex.h>

#define UART_PORT 0x80

uint8_t int_read(Z80EX_CONTEXT *cpu, void *user_data);
uint8_t port_read(Z80EX_CONTEXT *cpu, uint16_t port, void *user_data) ;
void port_write(Z80EX_CONTEXT *cpu, uint16_t port, uint8_t value, void *user_data);

#endif //_PORTS_H_