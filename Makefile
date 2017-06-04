CC=gcc
CFLAGS=-I -std=c99.
DEPS = filesystem.h utils.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o filesystem.o utils.o
	gcc -o filesystem main.o filesystem.o utils.o -I.
