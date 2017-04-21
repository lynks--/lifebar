# flags for $(CC)
CFLAGS=`pkg-config --cflags --libs x11 xft cairo` -lcurl -g
# a C compiler
CC=/usr/bin/gcc

# variables for convenience
SOURCES=$(wildcard *.c)
OBJECTS=$(subst .c,.o,$(SOURCES))

# the main target
# only the linking is done here
lifebar: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o lifebar $(OBJECTS)

# pattern to compile every .c file to a .o file
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

# removes files created by the compiler
.PHONY: clean
clean:
	rm -f lifebar
	rm -f *.o
