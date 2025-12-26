Предлагаемая структура проекта

Чтобы соблюсти SRP и не запутаться в include, предлагаю такую иерархию:
Plaintext

project/
├── core/                # Ядро (находится в фиксированном месте)
│   ├── kernel.asm       # Планировщик, прерывания
│   ├── memory.asm       # Memcpy, memset
│   └── math.asm         # Random, div/mul
├── drivers/             # Низкоуровневое железо
│   ├── audio/           # AY8910, YM2149...
│   ├── storage/         # FDD, HDD, SD...
│   └── video/           # CRTC, VDP...
├── services/            # Высокоуровневые сервисы
│   ├── fs/              # FAT16/32, File API
│   ├── console/         # Печать текста, ввод
│   └── players/         # PT3 player, VTX...
├── include/             # Общие интерфейсы
│   ├── bios.h           # Точки входа для C
│   └── bios.inc         # Константы для ASM
└── build/               # Скрипты сборки и мапы памяти
