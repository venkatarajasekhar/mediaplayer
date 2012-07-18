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
-include ../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../../include/MakeConfig

prefix		= /usr/local
exec_prefix	= ${prefix}

# Name
PRGNAME		= qDecoder

# Static Library Name
LIBNAME		= ../../../../lib/lib${PRGNAME}.a

# Shared Library Name
SLIBVERSION	= 2
SLIBNAME	= lib${PRGNAME}.so
SLIBREALNAME	= ${SLIBNAME}.${SLIBVERSION}

# System library directory
LIBDIR		= ${exec_prefix}/lib
HEADERDIR	= ${prefix}/include

# Which compiler & options for release
CC		= $(MYCC)
CFLAGS		= -g -O2
DEFS		=  -DSTDC_HEADERS=1 -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_STDARG_H=1 -DHAVE_CTYPE_H=1 -DHAVE_TIME_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_DIRENT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_FILE_H=1 -DHAVE_STRFTIME=1 -DHAVE_VPRINTF=1 -DHAVE_STRDUP=1 -DHAVE_STRSTR=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_OPENDIR=1 -DHAVE_CLOSEDIR=1 -DHAVE_FLOCK=1 
INCLUDE		= -I.

# Utilities
AR		= $(MYAR)
ARFLAGS		= -rc
CHMOD		= /bin/chmod
INSTALL		= /usr/bin/install -c -m 444
LD		= $(MYCXX)
LDFLAGS		= -shared
LN_S		= ln -s
RANLIB		= $(MYRANLIB)
RM		= /bin/rm


# Objects List
OBJ		= qDecoder.o \
		qSession.o \
		qfDecoder.o \
		qsDecoder.o \
		qHttpHeader.o \
		qError.o \
		qEnv.o \
		qEncode.o \
		qString.o \
		qFile.o \
		qValid.o \
		qArg.o \
		qAwk.o \
		qSed.o \
		qCount.o \
		qDownload.o \
		qTime.o \
		qSocket.o \
		qMisc.o \
		qInternalCommon.o \
		qInternalEntry.o \
		md5/md5c.o

## Make Library
all: ${OBJ}
	${AR} ${ARFLAGS} ${LIBNAME} ${OBJ}
	${RANLIB} ${LIBNAME}
	${LD} ${LDFLAGS} -o ${SLIBREALNAME} ${OBJ}
	${CHMOD} 0644 ${SLIBREALNAME}
	${RM} -f ${SLIBNAME}
	${LN_S} ${SLIBREALNAME} ${SLIBNAME}

## Compile Module
.c.o:
	${CC} ${CFLAGS} ${DEFS} ${INCLUDE} -c -o $@ $<

## Install Module
install: all
	${INSTALL} qDecoder.h ${HEADERDIR}/qDecoder.h
	${INSTALL} ${LIBNAME} ${LIBDIR}/${LIBNAME}
	${INSTALL} ${SLIBREALNAME} ${LIBDIR}/${SLIBREALNAME}
	${RM} -f ${LIBDIR}/${SLIBNAME}
	${LN_S} ${LIBDIR}/${SLIBREALNAME} ${LIBDIR}/${SLIBNAME}

uninstall: deinstall
deinstall:
	${RM} -f ${HEADERDIR}/qDecoder.h
	${RM} -f ${LIBDIR}/${LIBNAME}
	${RM} -f ${LIBDIR}/${SLIBREALNAME}
	${RM} -f ${LIBDIR}/${SLIBNAME}

## Clear Module
clean:
	${RM} -f ${OBJ} ${LIBNAME} ${SLIBREALNAME} ${SLIBNAME}
