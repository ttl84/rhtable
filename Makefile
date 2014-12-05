ifeq ($(OS), Windows_NT)
	LDFLAGS += -lmingw32
endif

CC=gcc
CFLAGS+=-std=c99 -pedantic-errors -Wstrict-aliasing=0 -Wall -g
	

a.out: src/rhtable.o src/test_rhtable.o
	$(CC) $^  $(LDFLAGS) -o $@

src/rhtable.o: src/rhtable.c src/rhtable.h

src/test_rhtable.o: src/test_rhtable.c src/rhtable.h

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@
