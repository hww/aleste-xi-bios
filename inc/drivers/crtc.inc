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
extern _crtc_vtable
extern _crtc_detect      ; +0  - проверка наличия
extern _crtc_install     ; +3  - инициализация
extern _crtc_get_info    ; +6  - информация
extern _crtc_command     ; +9  - универсальные команды
extern _crtc_uninstall   ; +12 - выгрузка
; Специфичные методы CRTC
extern _crtc_set_mode    ; +15 - установка видеорежима
extern _crtc_set_view    ; +18 - установка адреса отображения