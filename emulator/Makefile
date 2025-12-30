CXX = g++

# Флаги компиляции
CFLAGS = -O0 -g -Wall -std=c++17 -I../deps/z80ex/include `sdl2-config --cflags` -DDEBUG 
# -DDEBUG вместо -DEBUG

# Флаги линковки
LDFLAGS = `sdl2-config --libs` -L../deps/z80ex/lib -lz80ex -lz80ex_dasm -lreadline -lpthread 

# Исходные файлы
SRCS = src/main.cpp \
	src/debugger.cpp \
	src/screen.cpp \
	src/memory.cpp \
	src/ports.cpp \
	src/buffer_printer.cpp \
	src/mmu_native.cpp \
	src/palette.cpp \
	src/hex_dump.cpp \
	src/minibuffer.cpp

# Объектные файлы (меняем src/ на obj/)
OBJS = $(SRCS:src/%.cpp=obj/%.o)

# Целевой файл
TARGET = ../bin/z80_emu

all: $(TARGET)

# Правило линковки
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)  # Создаем папку bin
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Правило компиляции
obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)  # Создаем папку obj/src
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/* ../bin/*

run: all
	../bin/z80_emu test/test.bin

debug: all
	gdb --args ../bin/z80_emu test/test.bin

# Для тестирования
run-test: all
	test/build.sh 
	../bin/z80_emu test/test.bin

.PHONY: all clean run debug run-test