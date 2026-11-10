CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS  ?=
LDLIBS   ?=

OBJS := main.o gpu.o memory.o pixel_buffer.o pixel_packer.o addr_packer.o gpu_mmio.o
BIN  := sim

.PHONY: all run clean debug

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

main.o: main.cpp memory.h gpu.h pixel_buffer.h pixel_packer.h addr_packer.h gpu_mmio.h
gpu.o: gpu.cpp memory.h gpu.h pixel_buffer.h pixel_packer.h addr_packer.h
memory.o: memory.cpp memory.h
pixel_buffer.o: pixel_buffer.cpp pixel_buffer.h memory.h
pixel_packer.o: pixel_packer.cpp pixel_packer.h pixel_buffer.h addr_packer.h
addr_packer.o: addr_packer.cpp addr_packer.h
gpu_mmio.o: gpu_mmio.cpp gpu_mmio.h gpu.h pixel_buffer.h pixel_packer.h addr_packer.h memory.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(BIN)
	./$(BIN)

debug:
	$(CXX) -std=c++17 -O0 -g3 -Wall -Wextra -Wpedantic \
	    -fsanitize=address,undefined -fno-omit-frame-pointer \
	    -o $(BIN)-debug main.cpp gpu.cpp memory.cpp pixel_buffer.cpp pixel_packer.cpp addr_packer.cpp gpu_mmio.cpp
	./$(BIN)-debug

clean:
	rm -f $(OBJS) $(BIN) $(BIN)-debug *.d