CC=gcc
CFLAGS+=-std=c99 -pedantic-errors -Wstrict-aliasing=0 -Wall -g -O2
	
all: a.out librhtable.a

install: librhtable.a
	mkdir /usr/include/rhtable
	cp librhtable.a /usr/lib
	cp src/rhtable.h /usr/include/rhtable

a.out:  src/test_rhtable.o librhtable.a
	$(CC) -o $@ $< -L"./" -lrhtable

librhtable.a: src/rhtable.o
	ar rs $@ $^

src/rhtable.o: src/rhtable.c src/rhtable.h

src/test_rhtable.o: src/test_rhtable.c src/rhtable.h

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@
