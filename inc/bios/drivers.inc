; ==============================================================================
; BIOS_DRIVERS_BASE.INC - Общие определения драйверов BIOS Aleste (ASM версия)
; Должно совпадать с include/bios/drivers_base.h!
; ==============================================================================

; ==============================================================================
; ФЛАГИ ВОЗМОЖНОСТЕЙ ДРАЙВЕРОВ
; ==============================================================================

API_HAS_IRQ_BIT       equ 1 << 0  ; Использует прерывания
API_STATIC_BIT        equ 1 << 1  ; Статическое устройство (встроенное)
API_USES_DMA_BIT      equ 1 << 2  ; Использует DMA
API_BANKED_BIT        equ 1 << 3  ; Работает с банковой памятью
API_POWER_SAVE_BIT    equ 1 << 4  ; Поддерживает энергосбережение
API_HOTPLUG_BIT       equ 1 << 5  ; Поддерживает горячее подключение
API_NEEDS_CONTEXT_BIT equ 1 << 6  ; Требует контекст (для MMU)
API_THREAD_SAFE_BIT   equ 1 << 7  ; Потокобезопасный

; ==============================================================================
; СТРУКТУРА ИНФОРМАЦИИ О ДРАЙВЕРЕ
; ==============================================================================

; Размер структуры DriverInfo в байтах
DRIVER_INFO_SIZE  equ 8        ; dw + db + db + dw + db + db

; Макрос для создания структуры DriverInfo в ASM
; Использование: DRIVER_INFO name_addr, major, minor, flags, irq
MACRO DRIVER_INFO name_addr, major, minor, flags_val, irq_val
    dw name_addr               ; name
    db major, minor            ; version_major, version_minor
    dw flags_val               ; api_flags
    db irq_val, 0              ; irq_vector, reserved
ENDM

; Альтернативный макрос (если нужны имена полей)
MACRO DRIVER_INFO_FIELDS
.name:          dw 0           ; const char* name
.version_major: db 0           ; uint8_t version_major
.version_minor: db 0           ; uint8_t version_minor  
.api_flags:     dw 0           ; uint16_t api_flags
.irq_vector:    db 0           ; uint8_t irq_vector
.reserved:      db 0           ; uint8_t reserved
ENDM

; ==============================================================================
; БАЗОВЫЕ МЕТОДЫ ДРАЙВЕРА
; ==============================================================================

; Индексы методов в VTable (смещения)
DRV_METHOD_DETECT    equ 0     ; +0  - проверка наличия устройства
DRV_METHOD_INSTALL   equ 1     ; +3  - инициализация
DRV_METHOD_GET_INFO  equ 2     ; +6  - информация о драйвере
DRV_METHOD_COMMAND   equ 3     ; +9  - универсальная команда (IOCTL)
DRV_METHOD_UNINSTALL equ 4     ; +12 - выгрузка драйвера

; Размер VTable
DRIVER_VTABLE_BASE_SIZE   equ 15       ; 5 методов × 3 байта
DRIVER_MAX_EXTRA_METHODS  equ 10       ; Максимум специфичных методов
DRIVER_VTABLE_FULL_SIZE   equ DRIVER_VTABLE_BASE_SIZE + (DRIVER_MAX_EXTRA_METHODS * 3)

; ==============================================================================
; СИСТЕМНЫЕ КОМАНДЫ (поддерживаются всеми драйверами)
; ==============================================================================

CMD_RESET          equ 0x00    ; Сброс устройства
CMD_STATUS         equ 0x01    ; Получить статус
CMD_SUSPEND        equ 0x02    ; Приостановить работу
CMD_RESUME         equ 0x03    ; Возобновить работу
CMD_GET_CAPABILITIES equ 0x04  ; Получить возможности
CMD_SET_POWER      equ 0x05    ; Установить режим питания

; ==============================================================================
; ДИАПАЗОНЫ КОМАНД ДЛЯ ТИПОВ УСТРОЙСТВ
; ==============================================================================

CMD_VIDEO_BASE     equ 0x10    ; Видео (0x10-0x1F)
CMD_AUDIO_BASE     equ 0x20    ; Аудио (0x20-0x2F)
CMD_STORAGE_BASE   equ 0x30    ; Хранилище (0x30-0x3F)
CMD_INPUT_BASE     equ 0x40    ; Ввод (0x40-0x4F)
CMD_NETWORK_BASE   equ 0x50    ; Сеть (0x50-0x5F)
CMD_MMU_BASE       equ 0x60    ; MMU (0x60-0x6F)
CMD_SYSTEM_BASE    equ 0x70    ; Системные (0x70-0x7F)

; ==============================================================================
; МАКРОСЫ ДЛЯ ПРОВЕРКИ КОМАНД
; ==============================================================================

; Проверка принадлежности команды диапазону
; Использование: IS_VIDEO_CMD command_code
; Выход: Z=1 если команда видео, Z=0 если нет
MACRO IS_VIDEO_CMD cmd
    ld a, cmd
    cp CMD_VIDEO_BASE
    jr c, .not_video
    cp CMD_AUDIO_BASE
    jr c, .is_video
.not_video:
    or 1                ; Z=0
    ret
.is_video:
    xor a               ; Z=1
ENDM

MACRO IS_AUDIO_CMD cmd
    ld a, cmd
    cp CMD_AUDIO_BASE
    jr c, .not_audio
    cp CMD_STORAGE_BASE
    jr c, .is_audio
.not_audio:
    or 1
    ret
.is_audio:
    xor a
ENDM

MACRO IS_MMU_CMD cmd
    ld a, cmd
    cp CMD_MMU_BASE
    jr c, .not_mmu
    cp CMD_SYSTEM_BASE
    jr c, .is_mmu
.not_mmu:
    or 1
    ret
.is_mmu:
    xor a
ENDM

; Быстрая проверка (без переходов, для inline)
; Вход: A = код команды
; Выход: CF=1 если команда принадлежит диапазону
MACRO CHECK_VIDEO_CMD
    cp CMD_VIDEO_BASE
    jr c, .not_video
    cp CMD_AUDIO_BASE
    jr nc, .not_video
    scf                 ; CF=1 (это видео команда)
    ret
.not_video:
    or a                ; CF=0 (не видео)
ENDM

MACRO CHECK_MMU_CMD
    cp CMD_MMU_BASE
    jr c, .not_mmu
    cp CMD_SYSTEM_BASE
    jr nc, .not_mmu
    scf                 ; CF=1 (это MMU команда)
    ret
.not_mmu:
    or a                ; CF=0 (не MMU)
ENDM

; ==============================================================================
; МАКРОСЫ ДЛЯ РАБОТЫ С ДРАЙВЕРАМИ
; ==============================================================================

; Проверка флага драйвера
; Использование: DRIVER_HAS_FLAG info_ptr, flag_constant
; Вход: HL = указатель на DriverInfo
; Выход: Z=1 если флаг установлен, Z=0 если нет
MACRO DRIVER_HAS_FLAG info_ptr, flag
    push hl
    ld hl, info_ptr
    inc hl
    inc hl
    inc hl              ; HL указывает на api_flags (смещение +3)
    ld a, (hl)
    inc hl
    ld h, (hl)
    ld l, a             ; HL = api_flags
    ld a, flag & 0xFF
    and l
    jr nz, .has_flag
    ld a, flag >> 8
    and h
.has_flag:
    pop hl
ENDM

; Быстрая проверка статического устройства
MACRO IS_STATIC_DEVICE info_ptr
    DRIVER_HAS_FLAG info_ptr, API_STATIC_DEVICE
ENDM

; Проверка использования прерываний
MACRO DRIVER_USES_IRQ info_ptr
    DRIVER_HAS_FLAG info_ptr, API_HAS_IRQ
ENDM

; ==============================================================================
; РАБОТА С VTABLE
; ==============================================================================

; Получить адрес метода из VTable
; Использование: GET_VTABLE_METHOD base, index
; Вход: HL = база VTable, A = индекс метода
; Выход: HL = адрес метода (jp инструкция)
MACRO GET_VTABLE_METHOD base, index
    push de
    ld hl, base
    ld e, a
    ld d, 0
    add hl, de
    add hl, de
    add hl, de          ; HL = base + index * 3
    pop de
ENDM

; Вызов метода через VTable
; Использование: CALL_VTABLE_METHOD base, index
MACRO CALL_VTABLE_METHOD base, index
    GET_VTABLE_METHOD base, index
    jp (hl)             ; Косвенный вызов
ENDM

; Вызов базового метода (detect, install, etc.)
MACRO CALL_DRV_DETECT base
    CALL_VTABLE_METHOD base, DRV_METHOD_DETECT
ENDM

MACRO CALL_DRV_INSTALL base
    CALL_VTABLE_METHOD base, DRV_METHOD_INSTALL
ENDM

MACRO CALL_DRV_COMMAND base
    CALL_VTABLE_METHOD base, DRV_METHOD_COMMAND
ENDM

; ==============================================================================
; ВСПОМОГАТЕЛЬНЫЕ МАКРОСЫ
; ==============================================================================

; Создание VTable в памяти
; Использование: CREATE_VTABLE detect, install, get_info, command, uninstall
MACRO CREATE_VTABLE detect_addr, install_addr, get_info_addr, command_addr, uninstall_addr
    jp detect_addr      ; +0
    jp install_addr     ; +3
    jp get_info_addr    ; +6
    jp command_addr     ; +9
    jp uninstall_addr   ; +12
ENDM

; Добавить специфичный метод в VTable
; Использование: ADD_VTABLE_METHOD address, method_addr
; Предполагается, что HL указывает на следующую свободную позицию в VTable
MACRO ADD_VTABLE_METHOD method_addr
    jp method_addr
ENDM

; Проверка поддержки команды драйвером
; Использование: CHECK_CMD_SUPPORT cmd_code, first_specific, last_specific
; Выход: CF=1 если команда поддерживается
MACRO CHECK_CMD_SUPPORT cmd, first, last
    ld a, cmd
    ; Проверяем базовые команды (0x00-0x05)
    cp CMD_SET_POWER + 1
    jr c, .supported
    
    ; Проверяем специфичные команды драйвера
    cp first
    jr c, .not_supported
    cp last + 1
    jr nc, .not_supported
    
.supported:
    scf
    ret
    
.not_supported:
    or a
    ret
ENDM

; ==============================================================================
; КОНСТАНТЫ ДЛЯ РАБОТЫ С ПРЕРЫВАНИЯМИ
; ==============================================================================

; Если драйвер использует прерывания (API_HAS_IRQ),
; irq_vector содержит номер вектора (0-255)
; 0 = не использует, 1-255 = номер вектора прерывания

IRQ_VECTOR_NONE     equ 0

; ==============================================================================
; МАКРОСЫ ДЛЯ СОЗДАНИЯ ДРАЙВЕРОВ
; ==============================================================================

; Начало объявления драйвера
MACRO DRIVER_BEGIN name_str
    SECTION DRV_VTABLE
    PUBLIC _##name_str##_vtable
_##name_str##_vtable:
    
    SECTION DRV_DATA
    PUBLIC _##name_str##_info
_##name_str##_info:
    
    SECTION DRV_CODE
    PUBLIC _##name_str##_detect, _##name_str##_install, _##name_str##_get_info
    PUBLIC _##name_str##_command, _##name_str##_uninstall
ENDM

; Создание информации о драйвере
MACRO DRIVER_INFO_BLOCK name_str, major, minor, flags, irq
_##name_str##_info:
    DRIVER_INFO .name, major, minor, flags, irq
.name:
    db name_str, 0
ENDM

; Создание VTable драйвера
MACRO DRIVER_VTABLE detect, install, get_info, command, uninstall
_##name_str##_vtable:
    CREATE_VTABLE detect, install, get_info, command, uninstall
ENDM
