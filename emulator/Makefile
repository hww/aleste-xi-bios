CC = gcc
CFLAGS = -I../deps/z80ex/include -Wall `sdl2-config --cflags`
LIBS = `sdl2-config --libs` -L../deps/z80ex/lib -lz80ex -lz80ex_dasm

SRCS = main.c debugger.c screen.c memory.c ports.c buffer_printer.c
OBJ = $(SRCS:.c=.o)
TARGET = ../bin/z80_emu

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)

run-test: all
	../bin/z80_emu test/test.bin
