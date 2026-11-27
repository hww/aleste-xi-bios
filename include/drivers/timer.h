/**
 * @file timer.h
 * @brief Temporizador del sistema (driver polimorfico, modo LX)
 * Shaeon (Carlos Palmero), para el modo LX del core aleste520ex.
 *
 * Temporizador rev 6 (puertos F4..F6, linea 6 del PIC): RT de 16 bits a
 * 1 MHz y un comparador. Los milisegundos se cuentan en software; el driver
 * rearma el comparador cada 30 ms como mucho para no perder vueltas de RT,
 * asi que int_install va antes que timer_install. Sin interrupciones,
 * timer_get_ms sigue bien si se llama al menos cada 65 ms.
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include "drivers/int.h"

void     timer_activate_lx(void);
uint8_t  timer_detect(void);
uint8_t  timer_install(void);
void*    timer_get_info(void);
void     timer_uninstall(void);
uint16_t timer_get_ms(void);                                  /* da la vuelta cada 65,5 s */
void     timer_delay_ms(uint16_t ms) __z88dk_fastcall;
void     timer_set_periodic(uint16_t period_us, int_handler_t handler); /* handler 0 = parar */
uint16_t timer_get_count(void);                               /* saltos del periodico */

#endif
