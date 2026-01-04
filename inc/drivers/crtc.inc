; inc/bios/drivers.inc - создаём если нет

; Базовые команды для всех драйверов
extern CRTC_CMD_RESET
extern CRTC_CMD_STATUS
extern CRTC_CMD_SUSPEND
extern CRTC_CMD_RESUME

; Диапазоны команд по устройствам
extern CRTC_CMD_VIDEO_BASE
extern CRTC_CMD_AUDIO_BASE
extern CRTC_CMD_STORAGE_BASE 
extern CRTC_CMD_MMU_BASE

; Флаги драйверов
extern CRTC_FLAG_IRQ_BIT 
extern CRTC_FLAG_STATIC_BIT
extern CRTC_FLAG_DMA_BIT
extern CRTC_FLAG_BANKED_BIT
extern CRTC_FLAG_POWER_SAVE

; Методы драйвера
extern __crtc_vtable
extern __crtc_detect      ; +0  - проверка наличия
extern __crtc_install     ; +3  - инициализация
extern __crtc_get_info    ; +6  - информация
extern __crtc_command     ; +9  - универсальные команды
extern __crtc_uninstall   ; +12 - выгрузка
; Специфичные методы CRTC
extern __crtc_set_mode    ; +15 - установка видеорежима
extern __crtc_set_view    ; +18 - установка адреса отображения



extern __crtc_mode_tables:
extern __mode_cpc_mode2       ; 0
extern __mode_cpc_mode1       ; 1
extern __mode_cpc_mode0       ; 2
extern __mode_16k1b           ; 3
extern __mode_16k2b           ; 4
extern __mode_16k4b           ; 5
extern __mode_32k2b           ; 6
extern __mode_32k4b           ; 7
extern __mode_32k8b           ; 8
extern __mode_64k4b           ; 9
extern __mode_64k8b           ; 10