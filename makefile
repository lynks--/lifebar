CFLAGS=`pkg-config --cflags --libs x11 xft cairo` -g
CC=/usr/bin/gcc

all:
	$(CC) $(CFLAGS) lifebar.c ipc.c render.c lifebar.h -o lifebar

