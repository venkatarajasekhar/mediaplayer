
# -*- sh -*-

# $Id: Makefile,v 1.55 2005/02/08 17:45:52 nico Exp $

SBINDIR=/usr/sbin
MANDIR=/usr/man
INCLUDEDIR=/usr/include
CROSS=mipsel-linux-
CC := $(CROSS)gcc
STRIP   = $(CROSS)strip

GCCINCDIR := ${shell $(CC) -print-search-dirs | sed -ne "s/install: \(.*\)/\1include/gp"}

#CFLAGS := -I../include -O2 -Wall
CFLAGS := -I../include -I$(GCCINCDIR)-O2 -Wall

#TARGETS = nanddump nandwrite flash_erase flash_erasea_all
TARGETS = flash_erase flash_eraseall nanddump nandwrite reload_bbt encode_ecc \
	
SYMLINKS = compr_lzari.c compr_lzo.c

%: %.o
	$(CC) $(LDFLAGS) -static -g -o $@ $^
	$(STRIP) $@
	chmod +x $@
	
%.o: %.c
	$(CC) $(CFLAGS) -g -c -o $@ $< -g -Wp,-MD,.$<.dep

.SUFFIXES:

all: $(TARGETS)

IGNORE=${wildcard .*.c.dep} 
-include ${IGNORE}

clean:
	rm -f *.o $(TARGETS) .*.c.dep $(SYMLINKS)

$(SYMLINKS):
	ln -sf ../fs/jffs2/$@ $@

mkfs.jffs2: crc32.o compr_rtime.o mkfs.jffs2.o compr_zlib.o compr.o compr_lzari.o compr_lzo.o
	$(CC) $(LDFLAGS) -o $@ $^ -lz

flash_eraseall: crc32.o flash_eraseall.o
	$(CC) $(LDFLAGS) -o $@ $^
	$(STRIP) $@
	chmod +x $@

jffs2reader: jffs2reader.o
	$(CC) $(LDFLAGS) -o $@ $^ -lz 

jffs2dump: jffs2dump.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

jffs3dump: jffs3dump.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

sumtool: sumtool.o crc32.o
	$(CC) $(LDFLAGS) -o $@ $^

install: ${TARGETS}
	mkdir -p ${DESTDIR}/${SBINDIR}
	install -m0755 -oroot -groot ${TARGETS} ${DESTDIR}/${SBINDIR}/
	mkdir -p ${DESTDIR}/${MANDIR}/man1
	gzip -c mkfs.jffs2.1 > ${DESTDIR}/${MANDIR}/man1/mkfs.jffs2.1.gz
	mkdir -p ${DESTDIR}/${INCLUDEDIR}/mtd
	install -m0644 -oroot -groot ../include/mtd/*.h ${DESTDIR}/${INCLUDEDIR}/mtd/
