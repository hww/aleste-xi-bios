; Глобальная разметка памяти для линковщика
SECTION code_crt_start
org 0x0000

SECTION code_compiler
org 0x0200    ; Код после векторов и системных данных

SECTION DRV_CODE
org 0xC000    ; Код драйверов

SECTION FONT_DATA
org 0xE000    ; Данные шрифта

SECTION DRV_VTABLE
org 0xFD00    ; Тот самый фиксированный адрес для всех JP драйверов

SECTION DRV_DATA
org 0xFF00    ; Данные драйверов

