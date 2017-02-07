
CC=gcc
CFLAGS=-I. -I./z80emu/ -Wall -O3 -std=c99 -lncurses
DEPS = bus.h kimp1emu.h pit.h fdc.h usart.h rtc.h

all: kimp1emu

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

kimp1emu: kimp1emu.o bus.o pit.o fdc.o usart.o rtc.o z80emu/z80emu.o 
	$(CC) -o $@ kimp1emu.o bus.o pit.o fdc.o usart.o rtc.o z80emu/z80emu.o $(CFLAGS)

clean:
	rm -f *.o kimp1emu

.PHONY: all clean