CC = gcc
CFLAGS = -Wall -ansi -pedantic -O2 -fomit-frame-pointer

all: z80emu.o

tables.h: maketables.c
	$(CC) -Wall $< -o maketables
	./maketables > $@

z80emu.o: z80emu.c z80emu.h z80config.h z80user.h \
	instructions.h macros.h tables.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o maketables
