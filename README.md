# XiAleste BIOS

## Правильная структура для твоего проекта

```text
XiAlesteBIOS/
├── include/                    ← для C программ
│   ├── aleste.h               ← главный для C
│   ├── bios/                  ← подсистемы BIOS (C)
│   │   ├── drivers.h          ← DriverInfo, флаги (C)
│   │   ├── errors.h           ← ERR_SUCCESS (C)
│   │   └── system.h           ← системные вызовы (C)
│   └── drivers/               ← публичные API драйверов (C)
│       ├── mmu.h
│       └── video.h
│
├── inc/                       ← для ассемблера (те же константы)
│   ├── aleste.inc             ← главный для ASM
│   ├── bios/                  ← подсистемы BIOS (ASM)
│   │   ├── drivers.inc        ← DRV_FLAG_* (ASM эквивалент)
│   │   ├── errors.inc         ← ERR_SUCCESS equ (ASM)
│   │   └── system.inc         ← системные вызовы (ASM)
│   ├── drivers/               ← интерфейсы драйверов (ASM)
│   │   ├── mmu.inc            ← метки методов
│   │   └── video.inc
│   └── hardware/
│       ├── ports.inc
│       └── memory.inc
│
└── drivers/                   ← РЕАЛИЗАЦИЯ драйверов
    ├── mmu/
    │   ├── mmu.asm            ← include "../../inc/bios/drivers.inc"
    │   └── mmu_private.inc    ← приватные константы
    └── video/
        ├── video.asm          ← include "../../inc/bios/drivers.inc"
        └── video_private.inc
```

**Ключевое правило:**

- include/ - что видят ВНЕШНИЕ программы (твой main.c и другие приложения)
- drivers/ - внутренняя реализация (собирается в библиотеку BIOS)
