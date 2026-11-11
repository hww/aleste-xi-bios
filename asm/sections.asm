; Global memory layout (Aleste LX BIOS, fixed for linking)
SECTION code_crt_start
org 0x0000

SECTION code_compiler
org 0x0200

; Library code and data follow code_compiler (no org: chained)
SECTION CODE
SECTION code_crt_init
SECTION code_crt_main
SECTION code_crt_exit
SECTION code_crt_return
SECTION code_crt_common
SECTION code_l
SECTION code_l_sdcc
SECTION code_math
SECTION code_error
SECTION code_user
SECTION rodata_compiler
SECTION rodata_user
SECTION data_compiler
SECTION data_user
SECTION bss_compiler
SECTION bss_user
SECTION BSS_END

SECTION DRV_CODE
org 0xC000

SECTION FONT_DATA
org 0xE000

SECTION DRV_DATA
org 0xF000

SECTION DRV_VTABLE
org 0xFD00
