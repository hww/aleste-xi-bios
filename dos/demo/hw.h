/* hw.h - acceso al hardware del Aleste LX para GPUDEMO.COM
 * Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
 *
 * GPUDEMO es un programa del LX-DOS (CP/M), en modo supervisor: usa los
 * puertos directamente. Paginas MMIO: 0 sistema (PIC), 2 paleta y CRTC,
 * 3 LX-GPU. Las interrupciones guardan y restauran la pagina.
 */
#ifndef HW_H
#define HW_H

#include <stdint.h>

uint8_t  port_in(uint8_t port) __z88dk_fastcall;

/* temporizador libre de 1 MHz (F4h-F7h), 32 bits con una sola copia */
uint32_t timer_us(void);

/* direccion fisica (24 bits) de una direccion del programa */
uint32_t phys(const void* p);

/* VBlank (linea 2 del PIC, enmascarada: se mira el pendiente) */
void     vblank_clear(void);
uint8_t  vblank_pending(void);
void     vblank_wait(void);               /* borra y espera el siguiente */

/* CRTC (registros R0-R21) y paleta */
void     crtc_set(uint8_t reg, uint8_t v);
uint8_t  crtc_get(uint8_t reg);
void     pal_set(uint8_t idx, uint16_t rgb);   /* 0RGB de 12 bits */

/* LX-GPU: lanza el paquete en la direccion fisica 'pk' y espera a que pare;
   devuelve el estado (bit 7 parada, bits 6-0 codigo de error) */
void     gpu_start(uint32_t pk);
uint8_t  gpu_status(void);
uint8_t  gpu_wait(void);

#endif
