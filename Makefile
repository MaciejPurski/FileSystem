CC=gcc
CFLAGS=-I.
DEPS = filesystem.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o filesystem.o 
	gcc -o filesystem main.o filesystem.o -I.
