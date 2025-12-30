#ifndef _PORTS_H_
#define _PORTS_H_

#include <stdint.h>
#include "z80ex.h"

#define UART_PORT 0x80

typedef enum {
    EScreenMode_Cpc_4b, 
    EScreenMode_Cpc_2b,
    EScreenMode_Cpc_1b,
    EScreenMode_Linear16k1b,
    EScreenMode_Linear16k2b,
    EScreenMode_Linear16k4b,
    EScreenMode_Linear32k2b,
    EScreenMode_Linear32k4b,
    EScreenMode_Linear32k8b,
    EScreenMode_Linear64k2b,
    EScreenMode_Linear64k4b,
    EScreenMode_Linear64k8b
} EScreenMode;


typedef struct {
    EScreenMode screenMode;
    uint32_t screenPosition;
} mc6845_t;

extern mc6845_t g_crct;


uint8_t int_read(Z80EX_CONTEXT *cpu, void *user_data);
uint8_t port_read(Z80EX_CONTEXT *cpu, uint16_t port, void *user_data) ;
void port_write(Z80EX_CONTEXT *cpu, uint16_t port, uint8_t value, void *user_data);

#endif //_PORTS_H_