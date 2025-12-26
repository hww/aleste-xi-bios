; ==============================================================================
; PORTS_ALESTE.INC - Порты специфичные для Aleste (дополнение к CPC)
; ==============================================================================

; MMIO WINDOW (только в Aleste)
MMIO_WINDOW_BEGIN  equ 0x00
MMIO_WINDOW_END    equ 0x7F

; MMU порты (только в Aleste)
MMU_CONTROLR       equ 0xF0
MMU_MMIO_WINDOW    equ 0xF1
MMU_SYSCAL         equ 0xF2
MMU_CLOCK          equ 0xF3
MMU_SUPER_SLOT     equ 0xFA
MMU_USER_SLOT      equ 0xFB
MMU_MAPPER_0       equ 0xFC    ; Окно 0000-3FFF
MMU_MAPPER_1       equ 0xFD    ; Окно 4000-7FFF
MMU_MAPPER_2       equ 0xFE    ; Окно 8000-BFFF
MMU_MAPPER_3       equ 0xFF    ; Окно C000-FFFF

; EXTENDED MMU порты (только в Aleste)
MMU_MAPPER_S0_0    equ 0xE0    ; Окно 0000-3FFF
MMU_MAPPER_S0_1    equ 0xE1    ; Окно 4000-7FFF
MMU_MAPPER_S0_2    equ 0xE2    ; Окно 8000-BFFF
MMU_MAPPER_S0_3    equ 0xE3    ; Окно C000-FFFF
MMU_MAPPER_S1_0    equ 0xE4    ; Окно 0000-3FFF
MMU_MAPPER_S1_1    equ 0xE5    ; Окно 4000-7FFF
MMU_MAPPER_S1_2    equ 0xE6    ; Окно 8000-BFFF
MMU_MAPPER_S1_3    equ 0xE7    ; Окно C000-FFFF
MMU_MAPPER_S2_0    equ 0xE8    ; Окно 0000-3FFF
MMU_MAPPER_S2_1    equ 0xE9    ; Окно 4000-7FFF
MMU_MAPPER_S2_2    equ 0xEA    ; Окно 8000-BFFF
MMU_MAPPER_S2_3    equ 0xEB    ; Окно C000-FFFF
MMU_MAPPER_S3_0    equ 0xEC    ; Окно 0000-3FFF
MMU_MAPPER_S3_1    equ 0xED    ; Окно 4000-7FFF
MMU_MAPPER_S3_2    equ 0xEE    ; Окно 8000-BFFF
MMU_MAPPER_S3_3    equ 0xEF    ; Окно C000-FFFF

; Макрос для переключения банков памяти в Aleste
MACRO MMU_SELECT_BANK window, bank
    ld a, bank
    if window = 0
        out (MMU_PORT_FC), a
    else if window = 1
        out (MMU_PORT_FD), a
    else if window = 2
        out (MMU_PORT_FE), a
    else if window = 3
        out (MMU_PORT_FF), a
    endif
ENDM