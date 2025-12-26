; ==============================================================================
; MEMORY_ALESTE@INC - Карта памяти Aleste LX с режимами Supervisor/User
; Соответствует спецификации MMU Architecture v2@0
; ==============================================================================

; ==============================================================================
; БАЗОВЫЕ КОНСТАНТЫ ПАМЯТИ
; ==============================================================================

; Адресное пространство (24 бита)
MEMORY_SIZE        equ 0x01000000  ; 16 МБ общее адресное пространство
PHYSICAL_RAM_SIZE  equ 0x00100000  ; 1 МБ физической оперативной памяти

; Базовые адреса
RAM_BASE           equ 0x00000000  ; Начало оперативной памяти
MMIO_BASE          equ 0x00FF0000  ; Начало MMIO пространства (последние 64К)

; ==============================================================================
; ОКОННАЯ БАНКОВАЯ ПАМЯТЬ (WINDOWED BANK MEMORY)
; ==============================================================================

; Общее оконное пространство (до MMIO)
WB_MEMORY_BEGIN    equ 0x000000    ; Начало оконной памяти
WB_MEMORY_END      equ 0xFEFFFF    ; Конец оконной памяти (перед MMIO)
WB_MEMORY_SIZE     equ 0xFF0000    ; Размер: 16МБ - 64К

; Логические окна (по 16К каждое)
WB_WINDOW_SIZE     equ 0x00004000  ; 16 КБ на окно

; Окно 0: 0000-3FFF
WB_WINDOW0_BEGIN   equ 0x000000
WB_WINDOW0_END     equ 0x003FFF
WB_WINDOW0_DEFAULT equ 0           ; Банк по умолчанию

; Окно 1: 4000-7FFF
WB_WINDOW1_BEGIN   equ 0x004000
WB_WINDOW1_END     equ 0x007FFF
WB_WINDOW1_DEFAULT equ 1

; Окно 2: 8000-BFFF
WB_WINDOW2_BEGIN   equ 0x008000
WB_WINDOW2_END     equ 0x00BFFF
WB_WINDOW2_DEFAULT equ 2

; Окно 3: C000-FFFF
WB_WINDOW3_BEGIN   equ 0x00C000
WB_WINDOW3_END     equ 0x00FFFF
WB_WINDOW3_DEFAULT equ 3

; Размер банка (64К)
BANK_SIZE          equ 0x00010000  ; 64 КБ

; ==============================================================================
; MMIO ПРОСТРАНСТВО (Memory Mapped I/O)
; ==============================================================================

MMIO_BEGIN         equ 0xFF0000    ; Начало MMIO (адрес 16МБ - 64К)
MMIO_END           equ 0xFFFFFF    ; Конец MMIO (адрес 16МБ - 1)
MMIO_SIZE          equ 0x00010000  ; 64 КБ

; MMIO_LO (новые устройства Aleste)
MMIO_LO_BEGIN      equ 0xFF0000    ; Начало MMIO_LO
MMIO_LO_END        equ 0xFF3FFF    ; Конец MMIO_LO
MMIO_LO_SIZE       equ 0x00004000  ; 16 КБ

; MMIO_HI (эмуляция Legacy устройств)
MMIO_HI_BEGIN      equ 0xFF4000    ; Начало MMIO_HI
MMIO_HI_END        equ 0xFFFFFF    ; Конец MMIO_HI
MMIO_HI_SIZE       equ 0x0000C000  ; 48 КБ

; ==============================================================================
; УСТРОЙСТВА В MMIO_LO (новые устройства Aleste)
; ==============================================================================

; Страница 0 MMIO_LO (FF0000-FF007F)
MMIO_PAGE0_BEGIN   equ 0xFF0000

; PIC Controller (Прерывания)
MMIO_PIC_BASE      equ 0xFF0000
MMIO_PIC_SIZE      equ 32

; IPC Mailbox (Системный почтовый ящик)
MMIO_IPC_BASE      equ 0xFF0020
MMIO_IPC_SIZE      equ 32

; System Timer
MMIO_TIMER_BASE    equ 0xFF0040
MMIO_TIMER_SIZE    equ 32

; RTC Controller
MMIO_RTC_BASE      equ 0xFF0060
MMIO_RTC_SIZE      equ 32

; Расширенный доступ к MMU
MMIO_MMU_EXT_BEGIN equ 0xFF0080
MMIO_MMU_EXT_END   equ 0xFF00BF
MMIO_MMU_EXT_SIZE  equ 64          ; 16 регистров × 4 байта

; Legacy Emulation (страница 2: FF0100-FF017F)
MMIO_LEGACY_BEGIN  equ 0xFF0100
MMIO_LEGACY_END    equ 0xFF017F
MMIO_LEGACY_SIZE   equ 128

; Палитра цветов (в Legacy области)
MMIO_PALETTE_BASE  equ 0xFF0100    ; Первые 32 байта Legacy области
MMIO_PALETTE_SIZE  equ 32

; 6845 CRTC (в Legacy области)
MMIO_6845_BASE     equ 0xFF0120    ; Следующие 32 байта
MMIO_6845_SIZE     equ 32

; ==============================================================================
; MMIO_HI (эмуляция Legacy устройств CPC)
; ==============================================================================

; Gate Array (7FXXh порты)
MMIO_GA_BASE       equ 0xFF7F00
MMIO_GA_SIZE       equ 256         ; Полное отображение портов 00-FF

; CRTC 6845 (BCXXh/BDXXh порты)
MMIO_CRTC_INDEX    equ 0xFFBC00    ; BCXXh
MMIO_CRTC_DATA     equ 0xFFBD00    ; BDXXh

; Верхний ROM (DFXXh)
MMIO_UPPER_ROM     equ 0xFFDF00

; PPI 8255 (F4XXh-F7XXh)
MMIO_PPI_PORTA     equ 0xFFF400    ; F4XXh
MMIO_PPI_PORTB     equ 0xFFF500    ; F5XXh
MMIO_PPI_PORTC     equ 0xFFF600    ; F6XXh
MMIO_PPI_CTRL      equ 0xFFF700    ; F7XXh

; ==============================================================================
; СИСТЕМНЫЕ РЕГИСТРЫ MMU (порты ввода-вывода)
; ==============================================================================

; Регистры Native Mode (порты F0-FF)
PORT_GLOBAL_CTRL   equ 0xF0        ; Главный регистр управления
PORT_MMIO_PAGE     equ 0xF1        ; Выбор страницы MMIO
PORT_SYSCALL       equ 0xF2        ; Системный вызов
PORT_CLOCK_CTRL    equ 0xF3        ; Управление частотой CPU
PORT_SUPER_SLOT    equ 0xF9        ; Активный слот для Supervisor
PORT_USER_SLOT     equ 0xFB        ; Активный слот для User
PORT_BANK_0        equ 0xFC        ; Банк для окна 0
PORT_BANK_1        equ 0xFD        ; Банк для окна 1
PORT_BANK_2        equ 0xFE        ; Банк для окна 2
PORT_BANK_3        equ 0xFF        ; Банк для окна 3

; Регистры Legacy Mode
PORT_LEGACY_RMR    equ 0x7F00      ; RMR регистр CPC
PORT_LEGACY_MMR    equ 0x7F00      ; MMR регистр CPC
PORT_LEGACY_UPPER_ROM equ 0xDF00   ; Выбор верхнего ROM
PORT_LEGACY_SYSCALL equ 0xF200     ; Системный вызов в Legacy

; ==============================================================================
; БИТЫ РЕГИСТРА GLOBAL_CTRL
; ==============================================================================

; Позиции битов в регистре GLOBAL_CTRL (порт F0h)
GLOBAL_SUPERVISOR  equ 0           ; Бит 0: supervisor_mode
GLOBAL_NATIVE      equ 1           ; Бит 1: native_mode
GLOBAL_TRAP_ENABLE equ 2           ; Бит 2: supervisor_hook (включение trap)
GLOBAL_RESERVED1   equ 3           ; Бит 3: зарезервировано
GLOBAL_MMIO_UNLOCK equ 4           ; Бит 4: mmio_user_unlock
GLOBAL_RESERVED2   equ 5           ; Бит 5: зарезервировано
GLOBAL_RESERVED3   equ 6           ; Бит 6: зарезервировано
GLOBAL_RESERVED4   equ 7           ; Бит 7: зарезервировано

; Значения по умолчанию после сброса
GLOBAL_DEFAULT     equ (1 << GLOBAL_SUPERVISOR) | (1 << GLOBAL_NATIVE)
                                   ; supervisor=1, native=1, mmio_userlock=0

; ==============================================================================
; СЛОТЫ ПАМЯТИ (для SUPER_SLOT/USER_SLOT)
; ==============================================================================

; Доступные слоты (0-3)
SLOT_0             equ 0           ; Слот 0
SLOT_1             equ 1           ; Слот 1
SLOT_2             equ 2           ; Слот 2
SLOT_3             equ 3           ; Слот 3

; ==============================================================================
; МАКРОСЫ ДЛЯ РАБОТЫ С MMU
; ==============================================================================

; Получить текущий активный слот (в зависимости от режима)
; Выход: A = текущий слот (0-3)
MACRO GET_CURRENT_SLOT
    in a, (PORT_GLOBAL_CTRL)
    bit GLOBAL_SUPERVISOR, a
    jr z, @user_mode
    ; Supervisor mode
    in a, (PORT_SUPER_SLOT)
    jr @end
@user_mode:
    ; User mode
    in a, (PORT_USER_SLOT)
@end:
ENDM

; Переключить банк в текущем активном слоте
; Вход: window (0-3), bank (0-255)
MACRO SWITCH_BANK window, bank
    ld a, bank
    if window = 0
        out (PORT_BANK_0), a
    else if window = 1
        out (PORT_BANK_1), a
    else if window = 2
        out (PORT_BANK_2), a
    else if window = 3
        out (PORT_BANK_3), a
    endif
ENDM

; Быстрое переключение банка кода (окно 1)
MACRO SWITCH_CODE_BANK bank
    ld a, bank
    out (PORT_BANK_1), a
ENDM

; Быстрое переключение банка данных (окно 2)
MACRO SWITCH_DATA_BANK bank
    ld a, bank
    out (PORT_BANK_2), a
ENDM

; Системный вызов
; Вход: A = код функции
MACRO SYSCALL code
    ld a, code
    out (PORT_SYSCALL), a
ENDM

; Legacy системный вызов
MACRO SYSCALL_LEGACY code
    ld a, code
    out (PORT_LEGACY_SYSCALL), a
ENDM

; Вход в Supervisor Mode через trap
; Установить обработчики прерываний и включить trap
MACRO ENABLE_TRAP
    ld a, GLOBAL_DEFAULT | (1 << GLOBAL_TRAP_ENABLE)
    out (PORT_GLOBAL_CTRL), a
ENDM

; Выход из Supervisor Mode
MACRO EXIT_SUPERVISOR native_mode
    if native_mode = 1
        ld a, (1 << GLOBAL_NATIVE)  ; supervisor=0, native=1
    else
        xor a                       ; supervisor=0, native=0
    endif
    out (PORT_GLOBAL_CTRL), a
ENDM

; ==============================================================================
; MMIO РАБОТА (доступ через окно)
; ==============================================================================

; Выбрать страницу MMIO
; Вход: A = номер страницы (0-127)
MACRO SET_MMIO_PAGE page
    ld a, page
    out (PORT_MMIO_PAGE), a
ENDM

; Запись в регистр MMIO
; Вход: page = страница, offset = смещение (0-127), value = значение
MACRO WRITE_MMIO page, offset, value
    ld a, page
    out (PORT_MMIO_PAGE), a
    ld a, value
    out (offset), a
ENDM

; Чтение из регистра MMIO
; Вход: page = страница, offset = смещение (0-127)
; Выход: A = значение
MACRO READ_MMIO page, offset
    ld a, page
    out (PORT_MMIO_PAGE), a
    in a, (offset)
ENDM

; ==============================================================================
; РАБОТА С РАСШИРЕННЫМ MMU (MMU_EXT область)
; ==============================================================================

; Сохранить состояние маппера
; Вход: HL = указатель на буфер (64 байта)
MACRO SAVE_MMU_STATE buffer
    ld hl, buffer
    ld bc, 0040h           ; 64 байта
    ld a, 0                ; Страница 0
    out (PORT_MMIO_PAGE), a
    ld a, 80h              ; Начало MMU_EXT области
    ld d, 16               ; 16 регистров
@save_loop:
    ; Здесь должен быть код чтения через MMIO_WINDOW
    ; (зависит от конкретной реализации доступа)
    inc a
    dec d
    jr nz, @save_loop
ENDM

; ==============================================================================
; КОНСТАНТЫ ДЛЯ СИСТЕМНЫХ ВЫЗОВОВ
; ==============================================================================

; Коды системных вызовов
SYS_MMIO_ACCESS_REQUEST equ 0xFE  ; Запрос доступа к MMIO

; ==============================================================================
; СИСТЕМНЫЕ ОБЛАСТИ ПАМЯТИ
; ==============================================================================

; Таблица векторов прерываний (в начале памяти)
IVT_BEGIN          equ 0x000000
IVT_END            equ 0x0000FF
IVT_SIZE          equ 0x00000100  ; 256 байт

; Системный стек (в Supervisor слоте)
SYS_STACK_BEGIN    equ 0x00FF00    ; В конце окна 3
SYS_STACK_END      equ 0x00FFFF
SYS_STACK_SIZE     equ 0x00000100  ; 256 байт

; Область системных переменных
SYS_VARS_BEGIN     equ 0x000100
SYS_VARS_END       equ 0x0003FF
SYS_VARS_SIZE      equ 0x00000300  ; 768 байт

; ==============================================================================
; РАЗМЕЩЕНИЕ ПРОГРАММ
; ==============================================================================

; Базовый адрес загрузки программ (окно 1)
PROGRAM_LOAD_BASE  equ 0x004000
PROGRAM_LOAD_END   equ 0x007FFF
PROGRAM_LOAD_SIZE  equ 0x00004000  ; 16К

; Область данных программы (окно 2)
PROGRAM_DATA_BASE  equ 0x008000
PROGRAM_DATA_END   equ 0x00BFFF
PROGRAM_DATA_SIZE  equ 0x00004000  ; 16К

; Стек программы (окно 3)
PROGRAM_STACK_BASE equ 0x00C000
PROGRAM_STACK_TOP  equ 0x00FFFF    ; Растёт вниз
PROGRAM_STACK_SIZE equ 0x00004000  ; 16К

; ==============================================================================
; РЕЖИМЫ ДОСТУПА
; ==============================================================================

; Режимы пользователя (сочетания supervisor/native)
MODE_SUPERVISOR_NATIVE equ (1 << GLOBAL_SUPERVISOR) | (1 << GLOBAL_NATIVE)
MODE_USER_NATIVE      equ (1 << GLOBAL_NATIVE)
MODE_USER_LEGACY      equ 0

; Проверка режима доступа
; Вход: mode = режим для проверки
; Выход: Z=1 если текущий режим совпадает
MACRO CHECK_MODE mode
    in a, (PORT_GLOBAL_CTRL)
    and (1 << GLOBAL_SUPERVISOR) | (1 << GLOBAL_NATIVE)
    cp mode
ENDM

; ==============================================================================
; ФЛАГИ БЕЗОПАСНОСТИ
; ==============================================================================

; Проверка блокировки MMIO в User Mode
; Выход: Z=0 если доступ заблокирован (и мы в User Mode)
MACRO CHECK_MMIO_LOCK
    in a, (PORT_GLOBAL_CTRL)
    bit GLOBAL_SUPERVISOR, a
    jr nz, @supervisor    ; В Supervisor - всегда разрешено
    bit GLOBAL_MMIO_UNLOCK, a
    jr z, @locked         ; В User и mmio_userlock=0
@supervisor:
    xor a                 ; Z=1 (разрешено)
    ret
@locked:
    or 1                  ; Z=0 (запрещено)
ENDM