
CC_FOR_BUILD = $(CC)
CFLAGS_FOR_BUILD = -O2 -Wall

CFLAGS = -O2 -g -Wall

all: demo

decomp.h: UnicodeData.txt build_decomp
	./build_decomp < $< > $@.tmp
	mv $@.tmp $@

build_decomp: build_decomp.c
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) -o $@ $<

demo_OBJS = demo.o collate.o nfd.o lookup.o strxfrm.o
demo: $(demo_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $($@_OBJS)
