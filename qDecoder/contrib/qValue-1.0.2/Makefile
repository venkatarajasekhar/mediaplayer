# Generated automatically from Makefile.in by configure.
###########################################################################
# qDecoder - Web Application Interface for C/C++    http://www.qDecoder.org
#
# Copyright (C) 2001 The qDecoder Project.
# Copyright (C) 1999,2000 Hongik Internet, Inc.
# Copyright (C) 1998 Nobreak Technologies, Inc.
# Copyright (C) 1996,1997 Seung-young Kim.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Copyright Disclaimer:
#   Hongik Internet, Inc., hereby disclaims all copyright interest.
#   President, Christopher Roh, 6 April 2000
#
#   Nobreak Technologies, Inc., hereby disclaims all copyright interest.
#   President, Yoon Cho, 6 April 2000
#
#   Seung-young Kim, hereby disclaims all copyright interest.
#   Author, Seung-young Kim, 6 April 2000
###########################################################################
-include ../../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../../../include/MakeConfig

# Which compiler
CC	= $(MYCC)
CFLAGS	= -g -O2
DEFS	=  -DSTDC_HEADERS=1 -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_STDARG_H=1 -DHAVE_CTYPE_H=1 -DHAVE_TIME_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_DIRENT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_FILE_H=1 -DHAVE_STRFTIME=1 -DHAVE_VPRINTF=1 -DHAVE_STRDUP=1 -DHAVE_STRSTR=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_OPENDIR=1 -DHAVE_CLOSEDIR=1 -DHAVE_FLOCK=1 
INCLUDE	= -I. -I../../src

# qDecoder Library Path
#QLIB	= ../../src/libqDecoder.a
QLIB	= ../../../../../lib/libqDecoder.a
#QLIB	= ../../src/libqDecoder.so

## Make Library
all:	qValue

qValue: $(QLIB) qValue.o
	$(CC) ${CFLAGS} ${DEFS} ${INCLUDE} -o qValue.cgi qValue.o $(QLIB)

$(QLIB):
	(cd ../../src/; make all)

## Compile Module
.c.o:
	$(CC) ${CFLAGS} ${DEFS} ${INCLUDE} -c -o $@ $<

## Clear Module
clean:
	rm -f *.o qValue.cgi
