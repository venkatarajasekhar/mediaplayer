#Makefile for mkyaffs
#
# NB this is not yet suitable for putting into the kernel tree.
# YAFFS: Yet another Flash File System. A NAND-flash specific file system. 
#
# Copyright (C) 2002 Aleph One Ltd.
#   for Toby Churchill Ltd and Brightstar Engineering
#
# Created by Charles Manning <charles@aleph1.co.uk>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

## Change or override  KERNELDIR to your kernel

#KERNELDIR = /usr/src/kernel-headers-2.4.18

## Change if you are using a cross-compiler
MAKETOOLS = mipsel-linux-

CC=$(MAKETOOLS)gcc
STRIP=$(MAKETOOLS)strip

GCCINCDIR := ${shell $(CC) -print-search-dirs | sed -ne "s/install: \(.*\)/\1include/gp"}

#CFLAGS =   -I/usr/include -I.. -O2 -Wall -DCONFIG_YAFFS_UTIL
CFLAGS =   -I../../mtd//include -I$(GCCINCDIR) -I.. -O2 -Wall -DCONFIG_YAFFS_UTIL
CFLAGS+=   -Wshadow -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations
CFLAGS+=   -Wmissing-prototypes -Wredundant-decls -Wnested-externs -Winline

COMMONLINKS = yaffs_ecc.c
COMMONOBJS = $(COMMONLINKS:.c=.o)

MKYAFFSSOURCES = mkyaffsimage.c
MKYAFFSIMAGEOBJS = $(MKYAFFSSOURCES:.c=.o)

MKYAFFS2SOURCES = mkyaffs2image.c
MKYAFFS2LINKS = yaffs_packedtags2.c yaffs_tagsvalidity.c
MKYAFFS2IMAGEOBJS = $(MKYAFFS2SOURCES:.c=.o) $(MKYAFFS2LINKS:.c=.o)


MKYAFFS2SOURCES_4K = mkyaffs2image_4kcmd.c
MKYAFFS2IMAGEOBJS_4K = $(MKYAFFS2SOURCES_4K:.c=.o) $(MKYAFFS2LINKS:.c=.o)


#all: mkyaffsimage mkyaffs2image
all: mkyaffs2image mkyaffs2image_4kcmd


$(COMMONLINKS) $(MKYAFFSLINKS) $(MKYAFFS2LINKS):
	ln -s ../$@ $@

$(COMMONOBJS) $(MKYAFFSIMAGEOBJS) $(MKYAFFS2IMAGEOBJS) : %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

mkyaffsimage: $(COMMONOBJS) $(MKYAFFSIMAGEOBJS)
	$(CC) -o $@ $(COMMONOBJS) $(MKYAFFSIMAGEOBJS)

mkyaffs2image: $(COMMONOBJS) $(MKYAFFS2IMAGEOBJS)
	$(CC) -o $@ $(COMMONOBJS) $(MKYAFFS2IMAGEOBJS)
	$(STRIP) $@
	chmod +x $@	

mkyaffs2image_4kcmd: $(COMMONOBJS) $(MKYAFFS2IMAGEOBJS_4K)
	$(CC) -o $@ $(COMMONOBJS) $(MKYAFFS2IMAGEOBJS_4K)
	$(STRIP) $@
	chmod +x $@

clean:
	rm -f $(COMMONOBJS) $(MKYAFFSIMAGEOBJS) $(MKYAFFS2IMAGEOBJS) $(COMMONLINKS) $(MKYAFFSLINKS) $(MKYAFFS2LINKS) mkyaffs2image mkyaffs2image_4kcmd core
