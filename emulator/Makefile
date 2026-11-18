CXX = g++

# Путь к сабмодулю lx-gpu-rtl относительно директории emulator/
GPU_EMU_DIR = ../deps/lx-gpu-rtl/emul
GPU_TOOLS_DIR = ../deps/lx-gpu-rtl/tools

# Флаги компиляции
CFLAGS = -O0 -g -Wall -std=c++17 \
         -I../deps/z80ex/include \
         -I$(GPU_EMU_DIR) \
         -I$(GPU_TOOLS_DIR) \
         -Ilx \
         `sdl2-config --cflags` -DDEBUG

# Флаги линковки
LDFLAGS = `sdl2-config --libs` -L../deps/z80ex/lib -lz80ex -lz80ex_dasm -lreadline -lpthread

# Исходные файлы ядра Z80-эмулятора
SRCS = src/main.cpp \
	src/debugger.cpp \
	src/screen.cpp \
	src/memory.cpp \
	src/crtc.cpp \
	src/ports.cpp \
	src/mmu_native.cpp \
	src/palette.cpp \
	src/utils/hex_dump.cpp \
	src/utils/buffer_printer.cpp \
	src/utils/minibuffer.cpp \
	src/utils/ibuffer.cpp \
	src/globals.cpp \
	src/cli.cpp \
	src/commands.cpp

# Исходные файлы нового LX-GPU и связки gpu_mmio
LX_SRCS = lx/gpu_mmio.cpp

GPU_SRCS = $(GPU_EMU_DIR)/memory.cpp \
           $(GPU_EMU_DIR)/ch_addr_gen.cpp \
           $(GPU_EMU_DIR)/ch_blender.cpp \
           $(GPU_EMU_DIR)/ch_cache.cpp \
           $(GPU_EMU_DIR)/core_alu.cpp \
           $(GPU_EMU_DIR)/gpu_top.cpp \
           $(GPU_EMU_DIR)/host_if.cpp

# Объектные файлы
OBJS = $(SRCS:src/%.cpp=obj/%.o) \
       $(LX_SRCS:lx/%.cpp=obj/lx/%.o) \
       $(GPU_SRCS:$(GPU_EMU_DIR)/%.cpp=obj/gpu_emul/%.o)

# Целевой файл
TARGET = ../bin/z80_emu

all: $(TARGET)

# Правило линковки
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Правила компиляции
obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

obj/lx/%.o: lx/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

obj/gpu_emul/%.o: $(GPU_EMU_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/* ../bin/* ./src/*.o ./src/utils/*.o ./lx/*.o ./*.o

run: all
	test/build.sh 
	../bin/z80_emu test/test.bin test/test.sym

debug: all
	gdb --args ../bin/z80_emu test/test.bin

.PHONY: all clean run debug run-test
