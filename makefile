CFLAGS=-lX11 -lXft -I/usr/include/freetype2 -g
CC=/usr/bin/gcc

all:
	$(CC) $(CFLAGS) lifebar.c ipc.c render.c lifebar.h -o lifebar

