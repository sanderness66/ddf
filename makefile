CC = tcc
CFLAGS = -O
LDFLAGS = -s

all:	ddf

clean:
	rm -f ddf *.o *~ a.out

install: ddf ddf.1
	cp ddf /usr/local/bin/ddf
	cp ddf.1 /usr/local/share/man/man1/ddf.1
