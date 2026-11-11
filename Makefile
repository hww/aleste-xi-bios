# Aleste LX BIOS (LX.ROM) and LX-DOS (LXDOS.SYS)
#
#   make            ROM + ABI + LX-DOS  ->  output/LX.ROM, output/LXDOS.SYS
#   make rom        only the ROM and its ABI (output/bios_abi.asm)
#   make dos        LX-DOS (needs the ROM's ABI)
#   make clean
#
# z88dk: set Z88DK_PATH here or on the command line (make Z88DK_PATH=...), or
# leave zcc in the PATH.
Z88DK_PATH ?= /home/valery/z88dk
export PATH := $(Z88DK_PATH)/bin:$(PATH)
export ZCCCFG ?= $(Z88DK_PATH)/lib/config

BUILD_DIR  = build
OUTPUT_DIR = output
BUILD      = $(BUILD_DIR)/aleste
ROM        = $(BUILD_DIR)/LX.ROM
ABI        = $(BUILD_DIR)/bios_abi.asm
ABI_ID     = $(BUILD_DIR)/abi_id.inc
DOS_DIR    = $(BUILD_DIR)/dos
DOS        = $(DOS_DIR)/LXDOS.SYS

CC = zcc
# --no-crt: our own crt0 (asm/crt0_aleste.asm); -m: map file (the ABI is
# taken from it)
CFLAGS = +z80 -vn -O3 --no-crt -m -Iinclude -I. -s -clib=sdcc_iy

# ---- ROM: the order matters (sections first, then crt0)
SECTIONS = asm/sections.asm
CRT      = asm/crt0_aleste.asm
FONTS    = fonts/font_win1252.asm
DRIVERS  = \
	bios/errors.asm bios/undefined_method.asm \
	drivers/mmu/mmu.asm drivers/mmu/mmu_mem.asm drivers/crtc/crtc.asm \
	drivers/palette/palette.asm drivers/palette/palette_12b.asm \
	drivers/vbuf/vbuf_16k1b.asm drivers/vbuf/vbuf_64k8b.asm drivers/vbuf/vbuf.asm \
	drivers/audio/aduio_ay8910.asm drivers/audio/audio.asm \
	drivers/int/int.asm drivers/int/int_lx.asm \
	drivers/timer/timer.asm drivers/timer/timer_lx.asm \
	drivers/dma/dma.asm drivers/dma/dma_lx.asm \
	drivers/kbd/kbd.asm drivers/kbd/kbd_lx.asm \
	drivers/snd/snd.asm drivers/snd/snd_ms2.asm bios/irqsave.asm bios/boot.asm bios/dosgate.asm \
	drivers/fdc/fdc.asm drivers/fdc/fdc_765.asm \
	src/drivers/mmu.c src/drivers/mmu_mem.c src/drivers/crtc.c src/drivers/palette.c src/drivers/vbuf8.c \
	src/drivers/vbuf.c src/drivers/audio.c \
	src/drivers/int.c src/drivers/timer.c src/drivers/dma.c src/drivers/kbd.c src/drivers/snd.c src/drivers/fdc.c
SOURCES  = src/console.c src/tune.c src/mod.c src/fat.c main.c

# ---- LX-DOS: linked against the ROM's ABI, with its own copy of some C code
DOS_SOURCES = dos/sections.asm dos/crt0_dos.asm $(ABI) \
	src/drivers/int.c src/drivers/timer.c src/drivers/dma.c \
	src/drivers/kbd.c src/drivers/snd.c src/drivers/fdc.c \
	src/drivers/vbuf.c src/console.c src/fat.c src/mod.c dos/gfx8.c dos/bdos.c dos/main.c

all: rom dos

rom: $(OUTPUT_DIR)/LX.ROM
dos: $(OUTPUT_DIR)/LXDOS.SYS

# ROM image (64 KB): each section at its fixed address (see asm/sections.asm)
$(ROM): $(SECTIONS) $(CRT) $(FONTS) $(DRIVERS) $(SOURCES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD) $(SECTIONS) $(CRT) $(FONTS) $(DRIVERS) $(SOURCES)
	head -c 65536 /dev/zero > $@
	dd if=$(BUILD)_code_crt_start.bin of=$@ bs=1 seek=$$((0x0000)) conv=notrunc 2>/dev/null
	dd if=$(BUILD)_code_compiler.bin  of=$@ bs=1 seek=$$((0x0200)) conv=notrunc 2>/dev/null
	dd if=$(BUILD)_DRV_CODE.bin       of=$@ bs=1 seek=$$((0xC000)) conv=notrunc 2>/dev/null
	dd if=$(BUILD)_FONT_DATA.bin      of=$@ bs=1 seek=$$((0xE000)) conv=notrunc 2>/dev/null
	dd if=$(BUILD)_DRV_DATA.bin       of=$@ bs=1 seek=$$((0xF000)) conv=notrunc 2>/dev/null
	dd if=$(BUILD)_DRV_VTABLE.bin     of=$@ bs=1 seek=$$((0xFD00)) conv=notrunc 2>/dev/null
	@[ "$$(wc -c < $@)" -eq 65536 ] || { echo "LX.ROM is not 64 KB: a section overflows"; exit 1; }
# ABI: the public symbols of the resident part (DRV_CODE, DRV_DATA,
# DRV_VTABLE, FONT_DATA), for programs linked against the ROM (LX-DOS)
	awk '$$1 !~ /_(head|tail|size)$$/ && $$6 == "public," && ($$9 == "DRV_CODE," || $$9 == "DRV_DATA," || $$9 == "DRV_VTABLE," || $$9 == "FONT_DATA,") { \
	        a = substr($$3, 2); print "PUBLIC " $$1; print "defc " $$1 " = 0x" a }' \
	    $(BUILD).map > $(ABI)
	@echo "$(ABI): $$(grep -c '^PUBLIC' $(ABI)) symbols"
# ABI fingerprint: 16 bits of the checksum of bios_abi.asm, written into the
# ROM at _bios_abi_id; LX-DOS checks it at boot
	@ABI_NUM=$$(( $$(cksum < $(ABI) | cut -d' ' -f1) & 0xFFFF )); \
	ABI_AT=$$(awk '$$1 == "_bios_abi_id" { print substr($$3, 2) }' $(BUILD).map); \
	[ -n "$$ABI_AT" ] || { echo "_bios_abi_id missing in the ROM"; exit 1; }; \
	printf "$$(printf '\\%03o\\%03o' $$((ABI_NUM & 255)) $$((ABI_NUM >> 8)))" | \
	    dd of=$@ bs=1 seek=$$((0x$$ABI_AT)) conv=notrunc 2>/dev/null; \
	printf 'defc ABI_ID = 0x%04X\n' $$ABI_NUM > $(ABI_ID); \
	echo "ABI fingerprint: $$(printf '%04X' $$ABI_NUM) (at $$ABI_AT)"

$(ABI) $(ABI_ID): $(ROM)

# LX-DOS: built in its own folder (crt0_dos.asm includes abi_id.inc)
$(DOS): $(ROM) $(DOS_SOURCES) dos/bdos.h dos/gfx8.h
	rm -rf $(DOS_DIR)
	mkdir -p $(DOS_DIR)
	cp dos/*.c dos/*.h dos/*.asm $(ABI_ID) $(DOS_DIR)/
	cd $(DOS_DIR) && $(CC) +z80 -vn -O3 --no-crt -m -I../../include -I../.. -s -clib=sdcc_iy -DFAT_WRITE -o lxdos \
	    sections.asm crt0_dos.asm ../bios_abi.asm \
	    ../../src/drivers/int.c ../../src/drivers/timer.c ../../src/drivers/dma.c \
	    ../../src/drivers/kbd.c ../../src/drivers/snd.c ../../src/drivers/fdc.c \
	    ../../src/drivers/vbuf.c ../../src/console.c ../../src/fat.c ../../src/mod.c gfx8.c bdos.c main.c
	cat $(DOS_DIR)/lxdos_code_crt_start.bin > $@
	head -c $$((0x100 - $$(wc -c < $(DOS_DIR)/lxdos_code_crt_start.bin))) /dev/zero >> $@
	cat $(DOS_DIR)/lxdos_code_compiler.bin >> $@
	@[ "$$(wc -c < $@)" -le 45056 ] || { echo "LXDOS.SYS is over 44 KB (the stack is at the top of window 2)"; exit 1; }
	@echo "$@: $$(wc -c < $@) bytes"

$(OUTPUT_DIR)/LX.ROM: $(ROM)
	mkdir -p $(OUTPUT_DIR)
	cp $(ROM) $(ABI) $(OUTPUT_DIR)/

$(OUTPUT_DIR)/LXDOS.SYS: $(DOS)
	mkdir -p $(OUTPUT_DIR)
	cp $(DOS) $(OUTPUT_DIR)/

preprocess:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -E -o $(BUILD_DIR)/main.i main.c --list

debug_vbuf:
	mkdir -p $(BUILD_DIR)
	$(CC) +z80 -vn -Iinclude -I. -clib=sdcc_iy -E src/drivers/vbuf.c -o $(BUILD_DIR)/vbuf.i
	@echo "Preprocessed file: $(BUILD_DIR)/vbuf.i"

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

.PHONY: all rom dos clean preprocess debug_vbuf
