CFLAGS+= -Wall
LDADD+= -lxcb -lxcb-keysyms
LDFLAGS=
EXEC=catwm-xcb

PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

catwm-xcb: catwm-xcb.o
	$(CC) $(LDFLAGS) -Os -Wfatal-errors -o $@ $+ $(LDADD)

install: all
	install -Dm 755 catwm-xcb $(DESTDIR)$(BINDIR)/catwm-xcb

clean:
	rm -f catwm-xcb *.o
