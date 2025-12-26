; ==============================================================================
; PORTS_CPC.INC - Порты ввода-вывода Amstrad CPC
; ==============================================================================

; ==============================================================================
; ОСНОВНЫЕ ПОРТЫ
; ==============================================================================

; Gate Array (GA)
GA_PORT           equ 0x7F00    ; Базовый адрес (XX = любое значение)

; CRTC 6845 (видеоконтроллер)
CRTC_INDEX_PORT   equ 0xBC00    ; Индекс регистра CRTC
CRTC_DATA_OUT_PORT equ 0xBD00   ; Запись данных в CRTC
CRTC_STATUS_PORT  equ 0xBE00    ; Чтение статуса CRTC
CRTC_DATA_IN_PORT equ 0xBF00    ; Чтение данных из CRTC

; Верхний банк ПЗУ
UPPER_ROM_PORT    equ 0xDF00    ; Выбор банка верхнего ПЗУ

; Порт принтера
PRINTER_PORT      equ 0xEF00    ; Порт принтера

; 8255 PPI (программируемый периферийный интерфейс)
PPI_PORTA_PORT    equ 0xF400    ; Порт A - данные PSG
PPI_PORTB_PORT    equ 0xF500    ; Порт B - статус (Vsync, принтер, кассета)
PPI_PORTC_PORT    equ 0xF600    ; Порт C - управление (клавиатура, PSG, кассета)
PPI_CTRL_PORT     equ 0xF700    ; Управляющий регистр PPI

; Контроллер дисковода 765 FDC
FDC_MOTOR_PORT    equ 0xFA7E    ; Управление мотором дисковода
FDC_STATUS_PORT   equ 0xFB7E    ; Регистр статуса FDC
FDC_DATA_PORT     equ 0xFB7F    ; Регистр данных FDC

; ==============================================================================
; БИТОВЫЕ МАСКИ И ЗНАЧЕНИЯ
; ==============================================================================

; Gate Array (порт 0x7FXX)
GA_PENR_MASK      equ 0x40      ; Бит 6: PENR (разрешение прерываний)

; Порт B PPI (0xF5XX) - биты входа
PPIB_VSYNC_BIT    equ 0         ; Бит 0: VSYNC (вертикальная синхронизация)
PPIB_PRINT_BUSY   equ 1         ; Бит 1: PRN_BUSY (принтер занят)
PPIB_CASSETTE_IN  equ 2         ; Бит 2: CASSETTE IN (ввод с кассеты)
PPIB_EXP_BIT      equ 3         ; Бит 3: EXP (расширение, обычно 1)
PPIB_SOUND_ON     equ 4         ; Бит 4: SOUND ON (звук включен)
PPIB_PRINT_DATA   equ 5         ; Бит 5: PRN_DATA (данные принтера)
PPIB_CASSETTE_OUT equ 6         ; Бит 6: CASSETTE OUT (вывод на кассету)
PPIB_KEYBOARD_RESET equ 7       ; Бит 7: KB_RESET (сброс клавиатуры)

; Порт C PPI (0xF6XX) - биты выхода
PPIC_PSG_BDIR     equ 0         ; Бит 0: PSG BDIR (направление данных)
PPIC_PSG_BC1      equ 1         ; Бит 1: PSG BC1 (выбор регистра)
PPIC_CASSETTE_MOTOR equ 2       ; Бит 2: CASSETTE MOTOR (мотор кассеты)
PPIC_KEYB_ROW0    equ 3         ; Бит 3: Keyboard row 0
PPIC_KEYB_ROW1    equ 4         ; Бит 4: Keyboard row 1
PPIC_KEYB_ROW2    equ 5         ; Бит 5: Keyboard row 2
PPIC_KEYB_ROW3    equ 6         ; Бит 6: Keyboard row 3
PPIC_KEYB_ROW4    equ 7         ; Бит 7: Keyboard row 4

; ==============================================================================
; PSG (AY-3-8910/8912) через PPI
; ==============================================================================

; Состояния управления PSG (биты BDIR и BC1)
PSG_IDLE          equ 0x00      ; 00: Неактивно
PSG_ADDR_LATCH    equ 0x02      ; 01: Защелкивание адреса (BC1=1, BDIR=0)
PSG_READ_DATA     equ 0x01      ; 10: Чтение данных (BC1=0, BDIR=1)
PSG_WRITE_DATA    equ 0x03      ; 11: Запись данных (BC1=1, BDIR=1)

; ==============================================================================
; КОНСТАНТЫ CRTC 6845
; ==============================================================================

; Регистры CRTC (для записи в CRTC_INDEX_PORT)
CRTC_H_TOTAL      equ 0         ; R0: Total Horizontal Characters
CRTC_H_DISPLAYED  equ 1         ; R1: Horizontal Displayed
CRTC_H_SYNC_POS   equ 2         ; R2: Horizontal Sync Position
CRTC_SYNC_WIDTH   equ 3         ; R3: Sync Widths (H and V)
CRTC_V_TOTAL      equ 4         ; R4: Total Vertical Lines
CRTC_V_ADJUST     equ 5         ; R5: Vertical Total Adjust
CRTC_V_DISPLAYED  equ 6         ; R6: Vertical Displayed
CRTC_V_SYNC_POS   equ 7         ; R7: Vertical Sync Position
CRTC_INTERLACE    equ 8         ; R8: Interlace and Skew
CRTC_MAX_SCAN     equ 9         ; R9: Maximum Scan Line Address
CRTC_CURSOR_START equ 10        ; R10: Cursor Start
CRTC_CURSOR_END   equ 11        ; R11: Cursor End
CRTC_START_HI     equ 12        ; R12: Start Address (High)
CRTC_START_LO     equ 13        ; R13: Start Address (Low)
CRTC_CURSOR_HI    equ 14        ; R14: Cursor Address (High)
CRTC_CURSOR_LO    equ 15        ; R15: Cursor Address (Low)
CRTC_LIGHT_PEN_HI equ 16        ; R16: Light Pen (High)
CRTC_LIGHT_PEN_LO equ 17        ; R17: Light Pen (Low)

; ==============================================================================
; МАКРОСЫ ДЛЯ РАБОТЫ С ПОРТАМИ
; ==============================================================================

; Чтение порта B PPI (статус)
MACRO READ_PPI_B
    in a, (PPI_PORTB_PORT)
ENDM

; Проверка VSYNC
MACRO CHECK_VSYNC
    in a, (PPI_PORTB_PORT)
    bit PPIB_VSYNC_BIT, a
ENDM

; Ожидание VSYNC (вертикальной синхронизации)
MACRO WAIT_VSYNC
.wait_vsync_loop:
    in a, (PPI_PORTB_PORT)
    bit PPIB_VSYNC_BIT, a
    jr nz, .wait_vsync_loop
.wait_vsync_active:
    in a, (PPI_PORTB_PORT)
    bit PPIB_VSYNC_BIT, a
    jr z, .wait_vsync_active
ENDM

; Запись в регистр PSG
; Использование: WRITE_PSG_REG номер_регистра, значение
MACRO WRITE_PSG_REG reg, value
    ; Защелкиваем адрес регистра
    ld a, reg
    out (PPI_PORTA_PORT), a
    ld a, PSG_ADDR_LATCH
    out (PPI_PORTC_PORT), a
    ld a, PSG_IDLE
    out (PPI_PORTC_PORT), a
    
    ; Записываем значение
    ld a, value
    out (PPI_PORTA_PORT), a
    ld a, PSG_WRITE_DATA
    out (PPI_PORTC_PORT), a
    ld a, PSG_IDLE
    out (PPI_PORTC_PORT), a
ENDM

; Чтение из регистра PSG
; Использование: READ_PSG_REG номер_регистра -> значение в A
MACRO READ_PSG_REG reg
    ; Защелкиваем адрес регистра
    ld a, reg
    out (PPI_PORTA_PORT), a
    ld a, PSG_ADDR_LATCH
    out (PPI_PORTC_PORT), a
    ld a, PSG_IDLE
    out (PPI_PORTC_PORT), a
    
    ; Читаем значение
    ld a, PSG_READ_DATA
    out (PPI_PORTC_PORT), a
    in a, (PPI_PORTA_PORT)
    push af
    ld a, PSG_IDLE
    out (PPI_PORTC_PORT), a
    pop af
ENDM

; Запись в регистр CRTC
; Использование: WRITE_CRTC_REG номер_регистра, значение
MACRO WRITE_CRTC_REG reg, value
    ld a, reg
    out (CRTC_INDEX_PORT), a
    ld a, value
    out (CRTC_DATA_OUT_PORT), a
ENDM

; ==============================================================================
; КОНСТАНТЫ ДИСКОВОДА
; ==============================================================================

; Команды FDC 765
FDC_CMD_SPECIFY    equ 0x03     ; Specify (установка параметров)
FDC_CMD_SENSE_DRIVE equ 0x04    ; Sense Drive Status
FDC_CMD_WRITE_DATA equ 0x05     ; Write Data
FDC_CMD_READ_DATA  equ 0x06     ; Read Data
FDC_CMD_RECALIBRATE equ 0x07    ; Recalibrate
FDC_CMD_SENSE_INT  equ 0x08     ; Sense Interrupt Status
FDC_CMD_WRITE_DEL  equ 0x09     ; Write Deleted Data
FDC_CMD_READ_ID    equ 0x0A     ; Read Sector ID
FDC_CMD_READ_DEL   equ 0x0C     ; Read Deleted Data
FDC_CMD_FORMAT     equ 0x0D     ; Format Track
FDC_CMD_SEEK       equ 0x0F     ; Seek
FDC_CMD_SCAN_EQUAL equ 0x11     ; Scan Equal
FDC_CMD_SCAN_LOW   equ 0x19     ; Scan Low or Equal
FDC_CMD_SCAN_HIGH  equ 0x1D     ; Scan High or Equal

; Статус FDC
FDC_ST_READY      equ 0x80      ; Готовность FDC
FDC_ST_NOT_READY  equ 0x08      ; Дисковод не готов
FDC_ST_WRITE_PROT equ 0x40      ; Защита от записи
FDC_ST_SEEK_END   equ 0x20      ; Конец поиска
FDC_ST_CRC_ERROR  equ 0x10      ; Ошибка CRC
FDC_ST_TRACK_0    equ 0x04      ; Дорожка 0
FDC_ST_INDEX      equ 0x02      ; Сигнал INDEX
FDC_ST_BUSY       equ 0x01      ; Занят выполнением команды