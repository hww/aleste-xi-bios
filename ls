; Порты MMU
MMU_PORT_BANK0     equ 0xFC
MMU_PORT_BANK1     equ 0xFD
MMU_PORT_BANK2     equ 0xFE
MMU_PORT_BANK3     equ 0xFF

; Команды MMU
MMU_CMD_GET_TOTAL  equ 0x10
MMU_CMD_GET_FREE   equ 0x11
MMU_CMD_RESERVE    equ 0x12
MMU_CMD_RELEASE    equ 0x13

; Методы драйвера
_mmu_vtable        equ $    ; Начало таблицы
_mmu_detect        equ $    ; +0
_mmu_install       equ $+3  ; +3
_mmu_get_info      equ $+6  ; +6
_mmu_command       equ $+9  ; +9
_mmu_uninstall     equ $+12 ; +12
_mmu_set_context   equ $+15 ; +15
_mmu_get_context   equ $+18 ; +18
_mmu_map_page      equ $+21 ; +21

; Методы менеджера памяти
_mmu_mem_init      equ $
_mmu_alloc_pages   equ $+3
_mmu_free_pages    equ $+6
_mmu_get_stats     equ $+9
