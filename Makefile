CC = /usr/bin/env clang

# Remove -DSHM to disable using shm
CFLAGS = -DSHM -Wall -fPIC

AR = /usr/bin/env ar
ARFLAGS = -c -v -r

SRCDIR = src
OBJECT = $(SRCDIR)/rbuf.o
STATIC = librbuf.a
DYNAMIC = librbuf.so

all: dynamic static

static: $(STATIC)

dynamic: $(DYNAMIC)

$(STATIC): $(OBJECT)
	$(AR) $(ARFLAGS) $@ $<

$(DYNAMIC): $(OBJECT)
	$(CC) $(CFLAGS) -shared -o $@ $<

$(OBJECT): $(SRCDIR)/rbuf.c $(SRCDIR)/rbuf.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.{a,so}
	rm -f $(SRCDIR)/*.o
