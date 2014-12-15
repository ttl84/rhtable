CC=gcc
CFLAGS+=-std=c99 -pedantic-errors -Wstrict-aliasing=0 -Wall -g -O2
LIB_PREFIX=/usr/lib
INCLUDE_PREFIX=/usr/include
all: a.out librhtable.a

install: librhtable.a
	mkdir -p $(INCLUDE_PREFIX)/rhtable
	mkdir -p $(LIB_PREFIX)
	cp src/*.h $(INCLUDE_PREFIX)/rhtable
	cp librhtable.a $(LIB_PREFIX)
	

a.out:  src/test_rhtable.o librhtable.a
	$(CC) -o $@ $< -L"./" -lrhtable

librhtable.a: src/rhtable.o
	ar rs $@ $^

src/rhtable.o: src/rhtable.c src/rhtable.h

src/test_rhtable.o: src/test_rhtable.c src/rhtable.h

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@
