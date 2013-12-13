CFLAGS=`pkg-config --cflags --libs x11 xft cairo` -g -lapm
CC=/usr/bin/gcc

all:
	$(CC) $(CFLAGS) *.c *.h -o lifebar

