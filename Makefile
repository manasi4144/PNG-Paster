CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz
 
TARGETS = multi_thread.c paster.c paster catpng.c is_png.c crc.c crc.h zutil.c zutil.h

all: paster

paster: paster.c
	$(CC) -o paster paster.c -pthread -lcurl zutil.c $(LDLIBS)

.PHONY: clean
clean:
	rm -f *.d *.o o* all.png paster
