#include "hex_dump.h"
#include <cstdio>
#include <cctype>

static void print_address(uint32_t addr) {
    printf("%02X %04X", (addr >> 16) & 0xFF, addr & 0xFFFF);
}

static char ascii_char(uint8_t byte) {
    if (byte >= 32 && byte <= 126 && byte != '\t' && byte != '\n' && byte != '\r') {
        return byte;
    }
    return '.';
}

void hex_dump(const uint8_t* buffer, uint32_t start, uint32_t end) {
    if (!buffer || start > end) {
        return;
    }
    
    const int BYTES_PER_LINE = 16;
    const int GROUP_SIZE = 4;
    
    uint32_t total_bytes = end - start + 1;
    uint32_t lines = (total_bytes + BYTES_PER_LINE - 1) / BYTES_PER_LINE;
    
    for (uint32_t line = 0; line < lines; line++) {
        uint32_t line_start = start + line * BYTES_PER_LINE;
        
        // Адрес
        print_address(line_start);
        printf(" | ");
        
        // Hex байты
        for (int i = 0; i < BYTES_PER_LINE; i++) {
            uint32_t current_addr = line_start + i;
            
            if (current_addr <= end) {
                printf("%02X", buffer[current_addr - start]);
            } else {
                printf("  ");
            }
            
            // Разделители
            if (i < BYTES_PER_LINE - 1) {
                if ((i + 1) % GROUP_SIZE == 0) {
                    printf("  ");
                } else {
                    printf(" ");
                }
            }
        }
        
        printf(" | ");
        
        // ASCII
        for (int i = 0; i < BYTES_PER_LINE; i++) {
            uint32_t current_addr = line_start + i;
            
            if (current_addr <= end) {
                printf("%c", ascii_char(buffer[current_addr - start]));
            } else {
                printf(" ");
            }
        }
        
        printf(" |\n");
    }
}

void hex_dump_len(const uint8_t* buffer, uint32_t start, uint32_t length) {
    if (!buffer || length == 0) {
        return;
    }
    hex_dump(buffer, start, start + length - 1);
}