PREFIX = /usr/local
CC ?= cc
LD ?= ld
CFLAGS += -std=c99 -Wall -ansi -pedantic
LIBS = -lm $(LIBS.$(OS))
LIBS.freebsd = -lcam
SOURCES = ataidle.c
MAN = ataidle.8
PROG = ataidle
MAINTAINER = Bruce Cran <bruce@cran.org.uk>
OS_CMD = uname -s | tr "[:upper:]" "[:lower:]"
OS:sh = $(OS_CMD)
OS ?= $(shell $(OS_CMD))
REV:sh = uname -r | head -c 1

all:	ataidle

ataidle: ataidle.o util.o main.o
	$(CC) $(CFLAGS) $(LIBS) -o ataidle main.o ataidle.o util.o

main.o: main.c mi/atadefs.h mi/atagen.h mi/util.h
	$(CC) $(CFLAGS) -c main.c

ataidle.o: $(OS)/ataidle.c mi/atagen.h mi/atadefs.h mi/util.h 
	$(CC) $(CFLAGS) -c $(OS)/ataidle.c

util.o: mi/util.c mi/util.h mi/atadefs.h mi/atagen.h
	$(CC) $(CFLAGS) -c mi/util.c

install: install-$(OS)

install-common: ataidle ataidle.8 freebsd/ataidle_rc
	install $(PROG) $(PREFIX)/sbin

install-linux: install-common
	if [ ! -d $(PREFIX)/share/man/man8 ]; then \
	  mkdir -p $(PREFIX)/share/man/man8; \
	fi;
	install $(MAN) $(PREFIX)/share/man/man8

install-freebsd: install-common
	install $(MAN) $(PREFIX)/man/man8
	install freebsd/ataidle_rc $(PREFIX)/etc/rc.d/ataidle

uninstall:
	rm $(PREFIX)/sbin/$(PROG)
	rm $(PREFIX)/man/man8/$(MAN)

clean:
	rm -f *.o $(PROG)
