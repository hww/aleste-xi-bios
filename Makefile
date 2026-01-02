# Проект: Aleste BIOS
Z88DK_PATH = /home/valery/bin/z88dk
# Добавляем их в PATH самого процесса Make
export PATH := $(Z88DK_PATH)/bin:$(OSS_CAD_PATH)/bin:$(PATH)
# Для z88dk обязательна эта переменная
export ZCCCFG := $(Z88DK_PATH)/lib/config

TARGET = aleste
BUILD_DIR = build
OUTPUT_DIR = output

IMAGE = $(OUTPUT_DIR)/$(TARGET).bin
BUILD = $(BUILD_DIR)/$(TARGET)


# Инструменты
CC = zcc
AS = z80asm

# Флаги компиляции
# -startup=0: используем свой crt0
# -m: генерировать map-файл (критично для отладки адресов)
CFLAGS = +z80 -vn -O3 -startup=0 -m -Iinclude -I. \
         -clib=sdcc_iy \
         -pragma-define:CRT_INTERRUPT_MODE=1
         -zorg 0xC000 \
		 -pragma-define:CRT_STACK_ADDRESS=0x3F00

# Исходники
SECTIONS = asm/sections.asm
CRT = asm/crt0_aleste.asm
DRIVERS = \
	bios/errors.asm \
	bios/undefined_method.asm \
	drivers/mmu/mmu.asm \
	drivers/mmu/mmu_mem.asm \
	drivers/crtc/crtc.asm \
	drivers/palette/palette.asm \
	drivers/palette/palette_12b.asm \
	drivers/vbuf/vbuf_16k1b.asm \
	drivers/vbuf/vbuf.asm \
	drivers/audio/aduio_ay8910.asm \
	drivers/audio/audio.asm \
	src/drivers/mmu.c \
	src/drivers/mmu_mem.c \
	src/drivers/crtc.c \
	src/drivers/palette.c \
	src/drivers/vbuf.c \
	src/drivers/audio.c \

FONTS = fonts/font_win1252.asm

# Сюда будешь добавлять новые: drivers/audio/ay8910.asm и т.д.

SOURCES = main.c

# Правило сборки
all: prepare $(TARGET).bin

prepare:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OUTPUT_DIR)

$(TARGET).bin: $(SECTIONS) $(CRT) $(DRIVERS) $(SOURCES)
# 1. Компиляция (уже работает)
	$(CC) $(CFLAGS) \
	-o $(BUILD_DIR)/$(TARGET) \
	$(SECTIONS) \
	$(CRT) \
	$(FONTS) \
	$(DRIVERS) \
	$(SOURCES)
# 1. Создаем чистую болванку 64 Кбайт (заполняем нулями)
	dd if=/dev/zero of=$(IMAGE) bs=1 count=65536

# 2. CRT0 и вектора (с адреса 0x0000, хотя код начинается со 0x0100)
	dd if=$(BUILD)_code_crt_start.bin of=$(IMAGE) conv=notrunc bs=1 seek=0
# 3. Основной код BIOS (main.c и библиотеки) - адрес 0x0200
	dd if=$(BUILD)_code_compiler.bin of=$(IMAGE) conv=notrunc bs=1 seek=512
# 4. Тела драйверов (DRV_CODE) - адрес 0xC000
	dd if=$(BUILD)_DRV_CODE.bin of=$(IMAGE) conv=notrunc bs=1 seek=49152
# 5. Ассеты (Шрифты) - адрес 0xE000
	dd if=$(BUILD)_FONT_DATA.bin of=$(IMAGE) conv=notrunc bs=1 seek=57344
# 6. Таблица прыжков (VTABLE) - адрес 0xFD00
	dd if=$(BUILD)_DRV_VTABLE.bin of=$(IMAGE) conv=notrunc bs=1 seek=64768
# 7. Данные драйверов (Переменные) - адрес 0xFF00
	dd if=$(BUILD)_DRV_DATA.bin of=$(IMAGE) conv=notrunc bs=1 seek=65280

	@echo "✅ Образ $(IMAGE) успешно собран."

preprocess: prepare
	$(CC) $(CFLAGS) -E \
	-o $(BUILD_DIR)/$(TARGET).i \
	$(SOURCES) \
	--list
debug_vbuf: prepare
	$(CC) +z80 -vn -Iinclude -I. -clib=sdcc_iy \
	-E \
	src/drivers/vbuf.c \
	-o $(BUILD_DIR)/vbuf.i
	@echo "Препроцессорный файл создан: $(BUILD_DIR)/vbuf.i"
	@grep -n "P" $(BUILD_DIR)/vbuf.i || echo "Символ P не найден в препроцессорном файле"
clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

run: $(TARGET).bin
	bin/z80_emu ../output/aleste.bin