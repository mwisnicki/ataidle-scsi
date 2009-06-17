PREFIX = /usr/local
CC ?= gcc
LD ?= ld
CFLAGS += -Wall -ansi -pedantic
LIBS = -lm
SOURCES = ataidle.c
MAN = ataidle.8
PROG = ataidle
MAINTAINER = Bruce Cran <bruce@cran.org.uk>
OS:sh = uname -s
OS ?= $(shell uname -s)
REV:sh = uname -r | head -c 1

all:	ataidle

ataidle:  ataidle.o util.o main.o
	$(CC) $(CFLAGS) $(LIBS) -o ataidle main.o ataidle.o util.o

main.o: main.c mi/atadefs.h mi/atagen.h mi/util.h
	$(CC) $(CFLAGS) -c main.c

ataidle.o: linux/ataidle.c freebsd/ataidle.c mi/atagen.h mi/atadefs.h mi/util.h 
	@if [ "$(OS)" = Linux ]; then \
	  $(CC) $(CFLAGS) -c linux/ataidle.c; \
	elif [ "$(OS)" = FreeBSD ]; then \
	  $(CC) $(CFLAGS) -c freebsd/ataidle.c; \
	fi

util.o: mi/util.c mi/util.h mi/atadefs.h mi/atagen.h
	$(CC) $(CFLAGS) -c mi/util.c

install: ataidle ataidle.8 freebsd/ataidle_rc
	install $(PROG) $(PREFIX)/sbin

	if [ "$(OS)" = Linux ]; then \
	  if [ ! -d $(PREFIX)/share/man/man8 ]; then \
	    mkdir -p $(PREFIX)/share/man/man8; \
	  fi; \
	  install $(MAN) $(PREFIX)/share/man/man8; \
	else \
	  install $(MAN) $(PREFIX)/man/man8; \
	  install freebsd/ataidle_rc $(PREFIX)/etc/rc.d/ataidle; \
	fi

uninstall:
	rm $(PREFIX)/sbin/$(PROG)
	rm $(PREFIX)/man/man8/$(MAN)

clean:
	rm -f *.o $(PROG)
