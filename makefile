CFLAGS=`pkg-config --cflags --libs x11 xft cairo` -lcurl -g
CC=/usr/bin/gcc

all:
	$(CC) $(CFLAGS) *.c -o lifebar

