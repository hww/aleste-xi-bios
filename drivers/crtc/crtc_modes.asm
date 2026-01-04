; ==============================================================================
; CRTC GRAPHICS MODES DATA (ALESTE LX)
; Format: db NumberOfRegisters, (db Register, db Value) * N
; ==============================================================================

PUBLIC __crtc_mode_tables:
PUBLIC __mode_cpc_mode2       ; 0
PUBLIC __mode_cpc_mode1       ; 1
PUBLIC __mode_cpc_mode0       ; 2
PUBLIC __mode_16k1b           ; 3
PUBLIC __mode_16k2b           ; 4
PUBLIC __mode_16k4b           ; 5
PUBLIC __mode_32k2b           ; 6
PUBLIC __mode_32k4b           ; 7
PUBLIC __mode_32k8b           ; 8
PUBLIC __mode_64k4b           ; 9
PUBLIC __mode_64k8b           ; 10

__crtc_mode_tables:
    dw __mode_cpc_mode2       ; 0
    dw __mode_cpc_mode1       ; 1
    dw __mode_cpc_mode0       ; 2
    dw __mode_16k1b           ; 3
    dw __mode_16k2b           ; 4
    dw __mode_16k4b           ; 5
    dw __mode_32k2b           ; 6
    dw __mode_32k4b           ; 7
    dw __mode_32k8b           ; 8
    dw __mode_64k4b           ; 9
    dw __mode_64k8b           ; 10

; --- CPC Modes ---

__mode_cpc_mode2:
    db 15 ; регистров
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00000000, 0x14, 0b00000000, 0x15, 0b00000000

__mode_cpc_mode1:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00000001, 0x14, 0b00000000, 0x15, 0b00000000

__mode_cpc_mode0:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00000010, 0x14, 0b00000000, 0x15, 0b00000000

; --- 16 KB LX Modes ---

__mode_16k1b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010000, 0x14, 0b00000001, 0x15, 0b00000000

__mode_16k2b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010001, 0x14, 0b00000001, 0x15, 0b00000000

__mode_16k4b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010010, 0x14, 0b00000001, 0x15, 0b00000000

; --- 32 KB LX Modes ---

__mode_32k2b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010001, 0x14, 0b00000011, 0x15, 0b00000001

__mode_32k4b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010010, 0x14, 0b00000011, 0x15, 0b00000001

__mode_32k8b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010011, 0x14, 0b00000011, 0x15, 0b00000001

; --- 64 KB LX Modes ---

__mode_64k4b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010010, 0x14, 0b00000111, 0x15, 0b00000010

__mode_64k8b:
    db 15
    db 0x00, 63, 0x01, 40, 0x02, 46, 0x03, 142, 0x04, 38
    db 0x05, 0,  0x06, 25, 0x07, 30, 0x08, 0,   0x09, 7
    db 0x0C, 0,  0x0D, 0
    db 0x12, 0b00010011, 0x14, 0b00000111, 0x15, 0b00000010