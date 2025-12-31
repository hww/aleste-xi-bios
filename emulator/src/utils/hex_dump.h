#ifndef HEX_DUMP_H
#define HEX_DUMP_H

#include <cstdint>
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Простой hex dump в формате:
 * 00 0000 | 00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00 | asciformhere |
 * 
 * @param buffer Указатель на данные
 * @param start 24-битный начальный адрес
 * @param end 24-битный конечный адрес
 */
void hex_dump(const uint8_t* buffer, uint32_t start, uint32_t end);

/**
 * Hex dump с указанием длины
 */
void hex_dump_len(const uint8_t* buffer, uint32_t start, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // HEX_DUMP_H