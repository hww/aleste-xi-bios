/* edit.h - linea de ordenes de LX-DOS (edicion, historial y Tab) */
#ifndef __EDIT_H__
#define __EDIT_H__
#include <stdint.h>

/* escribe el indicador y lee una linea; devuelve su longitud */
uint8_t le_gets(const char* prompt, char* buf, uint8_t max);

#endif
