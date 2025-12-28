; ==============================================================================
; PORTS_ALESTE.INC - Порты и регистры Aleste LX
; Соответствует правилам именования Aleste BIOS
; Версия 1.0
; ==============================================================================

; ==============================================================================
; ОБЩИЕ ПОРТЫ СИСТЕМЫ
; ==============================================================================

; ==================== MMU УПРАВЛЕНИЕ ====================

; Главные порты управления MMU
MMU_PORT_CTRL           equ 0xF0    ; Регистр управления режимами
MMU_PORT_MMIO_PAGE      equ 0xF1    ; Выбор страницы MMIO
MMU_PORT_SYSCALL        equ 0xF2    ; Системный вызов
MMU_PORT_CLOCK_CTRL     equ 0xF3    ; Управление частотой CPU
MMU_PORT_SUPER_SLOT     equ 0xF9    ; Активный слот для Supervisor
MMU_PORT_USER_SLOT      equ 0xFB    ; Активный слот для User
MMU_PORT_BANK_0         equ 0xFC    ; Банк для окна 0 (0000-3FFF)
MMU_PORT_BANK_1         equ 0xFD    ; Банк для окна 1 (4000-7FFF)
MMU_PORT_BANK_2         equ 0xFE    ; Банк для окна 2 (8000-BFFF)
MMU_PORT_BANK_3         equ 0xFF    ; Банк для окна 3 (C000-FFFF)

; ==================== ЗНАЧЕНИЯ ДЛЯ MMU_PORT_CTRL ====================

MMU_CTRL_SUPERVISOR_BIT equ 0x01    ; Бит 0: режим супервизора (1=вкл)
MMU_CTRL_NATIVE_BIT     equ 0x02    ; Бит 1: нативный режим (1=вкл)
MMU_CTRL_TRAP_BIT       equ 0x04    ; Бит 2: захват прерываний (1=вкл)
MMU_CTRL_USER_LOCK_BIT  equ 0x10    ; Бит 4: блокировка MMIO в User режиме

; Предустановленные режимы MMU
MMU_CTRL_DEFAULT        equ MMU_CTRL_SUPERVISOR_BIT | MMU_CTRL_NATIVE_BIT

; ==================== ЗНАЧЕНИЯ ДЛЯ MMU_PORT_CLOCK_CTRL ====================

MMU_CLOCK_DIV_2         equ 0x02    ; Делитель частоты 2
MMU_CLOCK_DIV_3         equ 0x03    ; Делитель частоты 3
MMU_CLOCK_DIV_4         equ 0x04    ; Делитель частоты 4 (6.25МГц при 25МГц)
MMU_CLOCK_DIV_5         equ 0x05    ; Делитель частоты 5
MMU_CLOCK_DIV_6         equ 0x06    ; Делитель частоты 6
MMU_CLOCK_DIV_7         equ 0x07    ; Делитель частоты 7
MMU_CLOCK_DIV_8         equ 0x08    ; Делитель частоты 8

MMU_CLOCK_CPC_BIT       equ 0x10    ; Бит режима совместимости с CPC

; Маска для битов делителя
MMU_CLOCK_DIV_MASK      equ 0x0F    ; Маска битов 0-3 (делитель)

; ==================== MMIO ОКНО ====================

MMIO_WINDOW_BEGIN       equ 0x00    ; Начало окна MMIO (порт 0x00)
MMIO_WINDOW_END         equ 0x7F    ; Конец окна MMIO (порт 0x7F)
MMIO_WINDOW_SIZE        equ 0x80    ; Размер окна MMIO (128 портов)

; ==============================================================================
; УСТРОЙСТВА ЧЕРЕЗ MMIO (MMIO_PAGE значения)
; ==============================================================================

MMIO_PAGE_PIC           equ 0x00    ; Страница контроллера прерываний
MMIO_PAGE_TIMER         equ 0x00    ; Страница системного таймера
MMIO_PAGE_PALETTE       equ 0x02    ; Страница палитры цветов
MMIO_PAGE_CRTC          equ 0x02    ; Страница CRTC контроллера
MMIO_PAGE_PSG           equ 0x03    ; Страница PSG (AY-3-8910)
MMIO_PAGE_GA            equ 0x04    ; Страница Gate Array
MMIO_PAGE_FDC           equ 0x05    ; Страница контроллера дисковода
MMIO_PAGE_DMA           equ 0x06    ; Страница DMA контроллера
MMIO_PAGE_SOUND         equ 0x07    ; Страница звукового чипа
MMIO_PAGE_INPUT         equ 0x08    ; Страница клавиатуры/мыши
MMIO_PAGE_RTC           equ 0x09    ; Страница часов реального времени

; ==============================================================================
; ПАЛИТРА ЦВЕТОВ (при MMIO_PAGE = MMIO_PAGE_PALETTE)
; ==============================================================================

; Регистры палитры (смещения в MMIO_WINDOW)
PALETTE_REG_INDEX       equ 0x00    ; Индекс цвета (0-15)
PALETTE_REG_DATA_LO     equ 0x01    ; Младший байт цвета
PALETTE_REG_DATA_HI     equ 0x02    ; Старший байт цвета
PALETTE_REG_BORDER_LO   equ 0x03    ; Младший байт цвета бордюра
PALETTE_REG_BORDER_HI   equ 0x04    ; Старший байт цвета бордюра
PALETTE_REG_CTRL        equ 0x05    ; Регистр управления
PALETTE_REG_MODIFIER    equ 0x06    ; Модификатор цвета

; Биты регистра управления палитрой (PALETTE_REG_CTRL)
PALETTE_CTRL_MODE_MASK  equ 0x03    ; Маска режимов (биты 0-1)
PALETTE_CTRL_MODE_12BIT equ 0x00    ; 12-битный RGB (по умолчанию)
PALETTE_CTRL_MODE_CPC   equ 0x01    ; Режим CPC (9-битный цвет)
PALETTE_CTRL_MODE_MSX   equ 0x02    ; Режим MSX
PALETTE_CTRL_MODE_YJK   equ 0x03    ; Режим YJK

PALETTE_CTRL_AUTO_INC_BIT   equ 0x20    ; Автоинкремент при записи
PALETTE_CTRL_MOD_XOR_BIT    equ 0x40    ; XOR модификатор
PALETTE_CTRL_MOD_ON_BIT     equ 0x80    ; Включить модификатор

; ==============================================================================
; CRTC 6845 КОНТРОЛЛЕР (при MMIO_PAGE = MMIO_PAGE_CRTC)
; ==============================================================================

; Базовые регистры CRTC
CRTC_PORT_ADDR          equ 0x00    ; Регистр адреса (выбор регистра)
CRTC_PORT_DATA          equ 0x01    ; Регистр данных (чтение/запись)

; Стандартные регистры CRTC 6845
CRTC_REG_HTOTAL         equ 0x00    ; R0: Всего символов по горизонтали
CRTC_REG_HDISPLAY       equ 0x01    ; R1: Отображаемых символов
CRTC_REG_HSYNCPOS       equ 0x02    ; R2: Позиция горизонтальной синхроимпульса
CRTC_REG_HSYNCWIDTH     equ 0x03    ; R3: Ширина синхроимпульса
CRTC_REG_VTOTAL         equ 0x04    ; R4: Всего строк
CRTC_REG_VADJUST        equ 0x05    ; R5: Корректировка вертикального
CRTC_REG_VDISPLAY       equ 0x06    ; R6: Отображаемых строк
CRTC_REG_VSYNCPOS       equ 0x07    ; R7: Позиция вертикальной синхроимпульса
CRTC_REG_INTERLACE      equ 0x08    ; R8: Чересстрочность
CRTC_REG_MAXSCAN        equ 0x09    ; R9: Максимальный адрес строки
CRTC_REG_CURSTART       equ 0x0A    ; R10: Начало курсора
CRTC_REG_CUREND         equ 0x0B    ; R11: Конец курсора
CRTC_REG_START_H        equ 0x0C    ; R12: Старший байт начального адреса
CRTC_REG_START_L        equ 0x0D    ; R13: Младший байт начального адреса
CRTC_REG_CURH           equ 0x0E    ; R14: Старший байт адреса курсора
CRTC_REG_CURL           equ 0x0F    ; R15: Младший байт адреса курсора

; Расширенные регистры CRTC (Aleste LX)
CRTC_REG_VIDEO_CTRL     equ 0x12    ; R18: Управление видео
CRTC_REG_HIGH_ADDRESS   equ 0x13    ; R19: Старшие биты адреса (A16-A23)
CRTC_REG_ADDR_MODE      equ 0x14    ; R20: Режим адресации
CRTC_REG_PIXEL_CTRL     equ 0x15    ; R21: Управление пиксельными часами

; ==================== CRTC_REG_VIDEO_CTRL значения ====================

; Биты на пиксель (биты 0-1)
CRTC_VIDEO_CTRL_BPP_MASK    equ 0x03
CRTC_VIDEO_CTRL_BPP_1BPP    equ 0x00    ; 1 бит/пиксель (2 цвета)
CRTC_VIDEO_CTRL_BPP_2BPP    equ 0x01    ; 2 бита/пиксель (4 цвета)
CRTC_VIDEO_CTRL_BPP_4BPP    equ 0x02    ; 4 бита/пиксель (16 цветов)
CRTC_VIDEO_CTRL_BPP_8BPP    equ 0x03    ; 8 бит/пиксель (256 цветов)

; Другие биты
CRTC_VIDEO_CTRL_LINEAR_BIT  equ 0x10    ; Бит 4: линейные пиксели
                                        ; 0 = CPC палитра, 1 = Линейный RGB

CRTC_VIDEO_CTRL_CPC_MODES_BIT equ 0x20  ; Бит 5: режимы CPC
                                        ; 0 = Внутренние BPP режимы
                                        ; 1 = CPC GateArray графика

; Предустановленные видеорежимы
CRTC_VIDEO_MODE_CPC_0   equ CRTC_VIDEO_CTRL_CPC_MODES_BIT | CRTC_VIDEO_CTRL_BPP_1BPP
CRTC_VIDEO_MODE_CPC_1   equ CRTC_VIDEO_CTRL_CPC_MODES_BIT | CRTC_VIDEO_CTRL_BPP_2BPP
CRTC_VIDEO_MODE_CPC_2   equ CRTC_VIDEO_CTRL_CPC_MODES_BIT | CRTC_VIDEO_CTRL_BPP_4BPP

; ==================== CRTC_REG_ADDR_MODE значения ====================

; Базовые биты
CRTC_ADDR_MODE_LINEAR_BIT   equ 0x01    ; Бит 0: линейный режим адресации
CRTC_ADDR_MODE_STEP_BIT     equ 0x02    ; Бит 1: шаг адреса
CRTC_ADDR_MODE_RATE_BIT     equ 0x04    ; Бит 2: частота адресации

; Режимы адресации (биты 5:4)
CRTC_ADDR_MODE_MASK         equ 0x30    ; Маска режимов адресации
CRTC_ADDR_MODE_CPC_16K      equ 0x00    ; CPC 16KB (стандартный CPC)
CRTC_ADDR_MODE_EX_32K       equ 0x10    ; EX 32KB (расширенный)
CRTC_ADDR_MODE_LX_32K       equ 0x20    ; LX 32KB (Aleste LX)
CRTC_ADDR_MODE_LX_64K       equ 0x30    ; LX 64KB (Aleste LX расширенный)
CRTC_ADDR_MODE_LINEAR       equ 0x40    ; Полностью линейный

; ==================== CRTC_REG_PIXEL_CTRL значения ====================

CRTC_PIXEL_CTRL_RATE_MASK   equ 0x03    ; Маска скорости пикселей (биты 0-1)
CRTC_PIXEL_CTRL_RATE_2BYTES equ 0x00    ; 2 байта за 16 тактов
CRTC_PIXEL_CTRL_RATE_4BYTES equ 0x01    ; 4 байта за 16 тактов
CRTC_PIXEL_CTRL_RATE_8BYTES equ 0x02    ; 8 байт за 16 тактов
CRTC_PIXEL_CTRL_RATE_16BYTES equ 0x03   ; 16 байт за 16 тактов

; ==============================================================================
; LEGACY ПОРТЫ (CPC СОВМЕСТИМОСТЬ)
; ==============================================================================

; Эти порты работают в Legacy режиме
CPC_PORT_RMR            equ 0x7F00  ; RMR регистр CPC
CPC_PORT_DFXX           equ 0xDF00  ; Выбор верхнего ROM
CPC_PORT_SYSCALL        equ 0xF200  ; Legacy системный вызов

; ==============================================================================
; СИСТЕМНЫЕ КОНСТАНТЫ
; ==============================================================================

; Максимальные значения
MMU_BANK_MAX            equ 255     ; Максимальный номер банка (0-255)
MMU_MMIO_PAGE_MAX       equ 127     ; Максимальный номер страницы MMIO (0-127)

; Размеры
MMU_WINDOW_SIZE         equ 0x4000  ; Размер окна памяти (16К)
MMU_BANK_SIZE           equ 0x10000 ; Размер банка памяти (64К)
