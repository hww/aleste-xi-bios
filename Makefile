# Проект: Aleste BIOS
Z88DK_PATH = /home/valery/z88dk
export PATH := $(Z88DK_PATH)/bin:$(OSS_CAD_PATH)/bin:$(PATH)
export ZCCCFG := $(Z88DK_PATH)/lib/config

TARGET = aleste
BUILD_DIR = build
OUTPUT_DIR = output

IMAGE = $(OUTPUT_DIR)/$(TARGET).bin
BUILD = $(BUILD_DIR)/$(TARGET)

CC = zcc
AS = z80asm

# -startup=0: свой crt0; -m: map-файл; -zorg: раскладка; стек
CFLAGS = +z80 -vn -O3 -startup=0 -m -Iinclude -I. -s \
         -clib=sdcc_iy \
         -pragma-define:CRT_INTERRUPT_MODE=1 \
         -zorg 0xC000 \
         -pragma-define:CRT_STACK_ADDRESS=0x3F00

SECTIONS = asm/sections.asm
CRT = asm/crt0_aleste.asm

# ---- Драйверы: старые + новые overlay ----
DRIVERS = \
	bios/errors.asm \
	bios/undefined_method.asm \
	bios/irqsave.asm \
	bios/boot.asm \
	bios/dosgate.asm \
	drivers/mmu/mmu.asm \
	drivers/mmu/mmu_mem.asm \
	drivers/crtc/crtc.asm \
	drivers/palette/palette.asm \
	drivers/palette/palette_12b.asm \
	drivers/vbuf/vbuf_16k1b.asm \
	drivers/vbuf/vbuf.asm \
	drivers/audio/aduio_ay8910.asm \
	drivers/audio/audio.asm \
	drivers/int/int.asm \
	drivers/int/int_lx.asm \
	drivers/timer/timer.asm \
	drivers/timer/timer_lx.asm \
	drivers/dma/dma.asm \
	drivers/dma/dma_lx.asm \
	drivers/kbd/kbd.asm \
	drivers/kbd/kbd_lx.asm \
	drivers/snd/snd.asm \
	drivers/snd/snd_ms2.asm \
	drivers/fdc/fdc.asm \
	drivers/fdc/fdc_765.asm \
	src/drivers/mmu.c \
	src/drivers/mmu_mem.c \
	src/drivers/crtc.c \
	src/drivers/palette.c \
	src/drivers/vbuf.c \
	src/drivers/audio.c \
	src/drivers/int.c \
	src/drivers/timer.c \
	src/drivers/dma.c \
	src/drivers/kbd.c \
	src/drivers/snd.c \
	src/drivers/fdc.c \
	src/console.c \
	src/tune.c \
	src/mod.c \
	src/fat.c

FONTS = fonts/font_win1252.asm
SOURCES = main.c

all: prepare $(TARGET).bin

prepare:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OUTPUT_DIR)

$(TARGET).bin: $(SECTIONS) $(CRT) $(DRIVERS) $(SOURCES)
	$(CC) $(CFLAGS) \
	-o $(BUILD_DIR)/$(TARGET) \
	$(SECTIONS) \
	$(CRT) \
	$(FONTS) \
	$(DRIVERS) \
	$(SOURCES)

# 1. Болванка 64 Кбайт
	dd if=/dev/zero of=$(IMAGE) bs=1 count=65536

# 2. CRT0 и вектора
	dd if=$(BUILD)_code_crt_start.bin of=$(IMAGE) conv=notrunc bs=1 seek=0
# 3. Основной код BIOS
	dd if=$(BUILD)_code_compiler.bin of=$(IMAGE) conv=notrunc bs=1 seek=512
# 4. Тела драйверов (DRV_CODE) - адрес 0xC000
	dd if=$(BUILD)_DRV_CODE.bin of=$(IMAGE) conv=notrunc bs=1 seek=49152
# 5. Шрифты - адрес 0xE000
	dd if=$(BUILD)_FONT_DATA.bin of=$(IMAGE) conv=notrunc bs=1 seek=57344
# 6. Данные драйверов - адрес 0xF000
	dd if=$(BUILD)_DRV_DATA.bin of=$(IMAGE) conv=notrunc bs=1 seek=61440
# 7. Таблица прыжков (VTABLE) - адрес 0xFD00
	dd if=$(BUILD)_DRV_VTABLE.bin of=$(IMAGE) conv=notrunc bs=1 seek=64768

	@echo "✅ Образ $(IMAGE) успешно собран."


preprocess: prepare
	$(CC) $(CFLAGS) -E -o $(BUILD_DIR)/$(TARGET).i $(SOURCES) --list

debug_vbuf: prepare
	$(CC) +z80 -vn -Iinclude -I. -clib=sdcc_iy -E \
	src/drivers/vbuf.c \
	-o $(BUILD_DIR)/vbuf.i
	@echo "Препроцессорный файл создан: $(BUILD_DIR)/vbuf.i"
	@grep -n "P" $(BUILD_DIR)/vbuf.i || echo "Символ P не найден в препроцессорном файле"

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

run: $(TARGET).bin
	bin/z80_emu output/aleste.bin build/aleste.sym

.PHONY: all prepare rom preprocess debug_vbuf clean run