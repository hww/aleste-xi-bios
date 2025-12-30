CXX = g++
CXXFLAGS = -I./src -I../deps/z80ex/include -Wall -std=c++17 `sdl2-config --cflags`
LIBS = `sdl2-config --libs` -L../deps/z80ex/lib -lz80ex -lz80ex_dasm

SRCS = src/main.cpp src/debugger.cpp src/screen.cpp \
	src/memory.cpp src/ports.cpp src/buffer_printer.cpp \
	src/mmu_native.cpp src/palette.cpp src/hex_dump.cpp \


OBJ = $(SRCS:.cpp=.o)
TARGET = ../bin/z80_emu

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

run: all
	test/build.sh 
	../bin/z80_emu test/test.bin

.PHONY: all clean run-test