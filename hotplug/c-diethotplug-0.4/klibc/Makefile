# If you are running a cross compiler, you may want to set this
# to something more interesting, like "arm-linux-".
CROSS = #/usr/i386-linux-uclibc/usr/bin/i386-uclibc-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

# code taken from uClibc to determine the current arch
ARCH := ${shell $(CC) -dumpmachine | sed -e s'/-.*//' -e 's/i.86/i386/' -e 's/sparc.*/sparc/' \
	-e 's/arm.*/arm/g' -e 's/m68k.*/m68k/' -e 's/ppc/powerpc/g'}
        
# code taken from uClibc to determine the gcc include dir
GCCINCDIR := ${shell $(CC) -print-search-dirs | sed -ne "s/install: \(.*\)/\1include/gp"}


OBJDIR=bin-$(ARCH)

HOME=$(shell pwd)

all: $(OBJDIR) $(OBJDIR)/start.o $(OBJDIR)/klibc.a

CFLAGS=-pipe -nostdinc -I$(GCCINCDIR)

#VPATH=lib:libstdio:libugly:libcruft:libcrypt:libshell:liblatin1:libdl:librpc:libregex:libm:syscalls.c
VPATH=lib:syscalls.c

SYSCALLOBJ=$(patsubst syscalls.s/%.S,$(OBJDIR)/%.o,$(wildcard syscalls.s/*.S))

LIBOBJ=$(patsubst lib/%.c,$(OBJDIR)/%.o,$(wildcard lib/*.c))
LIBUGLYOBJ=$(patsubst libugly/%.c,$(OBJDIR)/%.o,$(wildcard libugly/*.c))
LIBSTDIOOBJ=$(patsubst libstdio/%.c,$(OBJDIR)/%.o,$(wildcard libstdio/*.c))
LIBCRUFTOBJ=$(patsubst libcruft/%.c,$(OBJDIR)/%.o,$(wildcard libcruft/*.c))
LIBCRYPTOBJ=$(patsubst libcrypt/%.c,$(OBJDIR)/%.o,$(wildcard libcrypt/*.c))
LIBSHELLOBJ=$(patsubst libshell/%.c,$(OBJDIR)/%.o,$(wildcard libshell/*.c))
LIBMATH=$(patsubst libm/%.c,%.o,$(wildcard libm/*.c))

LIBRPCOBJ=$(patsubst librpc/%.c,$(OBJDIR)/%.o,$(wildcard librpc/*.c))
LIBREGEXOBJ=$(patsubst libregex/%.c,$(OBJDIR)/%.o,$(wildcard libregex/*.c))

LIBDLOBJ=$(patsubst libdl/%.c,$(OBJDIR)/%.o,$(wildcard libdl/*.c)) $(OBJDIR)/_dl_jump.o

include $(ARCH)/Makefile.add

LIBMATHOBJ=$(patsubst %,$(OBJDIR)/%,$(LIBMATH))

ifeq ($(CFLAGS),-pipe -nostdinc)
CFLAGS+=-O -fomit-frame-pointer
endif

ifneq ($(DEBUG),)
CFLAGS = -g
COMMENT = :
endif
CFLAGS += -Wall -W -Wchar-subscripts -Wconversion -Wmissing-prototypes -Wmissing-declarations -Wno-switch -Wredundant-decls

PWD=$(shell pwd)

.SUFFIXES:
.SUFFIXES: .S .c

$(OBJDIR):
	mkdir $@

% :: %,v

$(OBJDIR)/%.o: %.S
	$(CROSS)$(CC) -I. -Iinclude $(CFLAGS) -c $< -o $@


$(OBJDIR)/%.o: %.c
	$(CROSS)$(CC) -I. -Iinclude $(CFLAGS) -c $< -o $@
	$(COMMENT) $(CROSS)strip -x -R .comment -R .note $@

DIETLIBC_OBJ = $(SYSCALLOBJ) $(LIBOBJ) $(LIBSTDIOOBJ) $(LIBUGLYOBJ) \
$(LIBCRUFTOBJ) $(LIBCRYPTOBJ) $(LIBSHELLOBJ) $(LIBREGEXOBJ) $(OBJDIR)/unified.o

$(OBJDIR)/klibc.a: $(DIETLIBC_OBJ) $(OBJDIR)/start.o
	$(AR) cru $@ $(DIETLIBC_OBJ)

$(SYSCALLOBJ): syscalls.h

$(OBJDIR)/compile:
	echo 'exec' $(CC) '$(CFLAGS) -I$(PWD)/$(OBJDIR)/include -c $${1+"$$@"}' > $@
	chmod 755 $@

$(OBJDIR)/load:
	echo 'main="$$1"; shift; exec' $(CC) '-nostdlib -o "$$main" $(PWD)/$(OBJDIR)/start.o "$$main".o $${1+"$$@"} $(PWD)/$(OBJDIR)/klibc.a -lgcc'  > $@
	chmod 755 $@

clean:
	-find . \( -not -type d \) -and \( -name '*~' -o -name '*.[oas]' \) -type f -print \
	| xargs rm -f
	rm -rf bin-* pic-*

#VERSION=dietlibc-$(shell head -1 CHANGES|sed 's/://')
VERSION=0.1
CURNAME=$(notdir $(shell pwd))

tar: clean rename
	cd ..; tar cvvf $(VERSION).tar.bz2 $(VERSION) --use=bzip2 --exclude CVS

rename:
	if test $(CURNAME) != $(VERSION); then cd .. && mv $(CURNAME) $(VERSION); fi

$(OBJDIR)/exports: $(OBJDIR)/klibc.a
	nm -g $(OBJDIR)/klibc.a | grep -w T | awk '{ print $$3 }' | sort -u > $(OBJDIR)/exports

.PHONY: sparc ppc mips arm alpha i386 s390

arm sparc ppc alpha i386 mips s390:
	$(MAKE) ARCH=$@ CROSS=$@-linux- all t bin-$@/libdietc.so

mips-gnu:
	$(MAKE) ARCH=$@ CROSS=$@-linux-gnu- all t bin-$@/libdietc.so

cross:
	$(MAKE) arm sparc ppc alpha i386 mips


# these depend on dietfeatures.h for large file backward compatibility
$(OBJDIR)/__fstat64.o $(OBJDIR)/__lstat64.o $(OBJDIR)/__stat64.o $(OBJDIR)/lseek64.o $(OBJDIR)/readdir64.o: dietfeatures.h

# these depend on dietfeatures.h for thread support
#$(OBJDIR)/alloc.o $(OBJDIR)/perror.o $(OBJDIR)/logging.o $(OBJDIR)/unified.o $(OBJDIR)/clone.o $(OBJDIR)/set_errno.o: dietfeatures.h

# these depend on dietfeatures.h for linker warnings
$(OBJDIR)/assert_fail.o $(OBJDIR)/sprintf.o $(OBJDIR)/vsnprintf.o $(OBJDIR)/___div.o $(OBJDIR)/fflush.o $(OBJDIR)/setvbuf.o $(OBJDIR)/system.o $(OBJDIR)/sendfile.o $(OBJDIR)/setenv.o: dietfeatures.h

# these depend on dietfeatures.h for buffered stdio
fclose.o $(OBJDIR)/fdglue.o $(OBJDIR)/fflush.o $(OBJDIR)/fgetc.o $(OBJDIR)/fputc.o $(OBJDIR)/fread.o $(OBJDIR)/fseek.o $(OBJDIR)/printf.o $(OBJDIR)/setvbuf.o $(OBJDIR)/stderr.o $(OBJDIR)/stdin.o $(OBJDIR)/stdout.o $(OBJDIR)/fwrite.o $(OBJDIR)/puts.o: dietfeatures.h

# these depend on dietfeatures.h for fast string routines
strcasecmp.o $(OBJDIR)/strcat.o $(OBJDIR)/strchr.o $(OBJDIR)/strcmp.o $(OBJDIR)/strcpy.o $(OBJDIR)/strlen.o $(OBJDIR)/strncasecmp.o $(OBJDIR)/strncat.o $(OBJDIR)/strrchr.o: dietfeatures.h

# these depend on dietfeatures.h for /proc
$(OBJDIR)/ttyname.o: dietfeatures.h

# these depend on dietfeatures.h for WANT_TZFILE_PARSER
$(OBJDIR)/localtime_r.o: dietfeatures.h

# these depend on dietfeatures.h for WANT_BUGGY_GLIBC_STRSEP
$(OBJDIR)/strsep.o: dietfeatures.h

# these depend on dietfeatures.h for WANT_SMALL_STDIO_BUFS
$(LIBSTDIOOBJ): dietfeatures.h

# these depend on dietfeatures.h for WANT_FULL_RESOLV_CONF
$(OBJDIR)/dnscruft.o $(OBJDIR)/dnscruft2.o: dietfeatures.h

# these depend on dietfeatures.h for WANT_THREAD_SAFE and errno
#$(LIBRPCOBJ) $(OBJDIR)/logging.o $(OBJDIR)/alloc.o $(OBJDIR)/cfsetospeed.o $(OBJDIR)/cfsetispeed.o \
#$(OBJDIR)/execl.o $(OBJDIR)/execlp.o $(OBJDIR)/execv.o $(OBJDIR)/execvp.o $(OBJDIR)/isatty.o \
#$(OBJDIR)/lockf.o $(OBJDIR)/perror.o $(OBJDIR)/remove.o $(OBJDIR)/set_errno.o $(OBJDIR)/sigemptyset.o \
#$(OBJDIR)/tcsetattr.o $(OBJDIR)/dnscruft2.o $(OBJDIR)/dnscruft.o $(OBJDIR)/entlib.o \
#$(OBJDIR)/gethostbyaddr.o $(OBJDIR)/gethostbyaddr_r.o $(OBJDIR)/gethostbyname.o \
#$(OBJDIR)/gethostbyname_r.o $(OBJDIR)/gethostbyname2.o $(OBJDIR)/gethostbyname2_r.o \
#$(OBJDIR)/inet_pton.o $(OBJDIR)/mkstemp.o $(OBJDIR)/mktemp.o $(OBJDIR)/tempnam.o \
#$(OBJDIR)/glob.o $(OBJDIR)/realpath.o $(OBJDIR)/fdglue.o $(OBJDIR)/fdglue2.o \
#$(OBJDIR)/getaddrinfo.o $(OBJDIR)/getnameinfo.o $(OBJDIR)/getprotoent.o \
#$(OBJDIR)/getservent.o $(OBJDIR)/iconv.o $(OBJDIR)/iconv_open.o \
#$(OBJDIR)/netent.o $(OBJDIR)/system.o: dietfeatures.h

# these depend on dietfeatures.h for WANT_CRYPT_MD5
$(OBJDIR)/crypt.o: dietfeatures.h

#$(OBJDIR)/unified.o: dietuglyweaks.h

$(OBJDIR)/adjtimex.o: include/sys/timex.h

$(OBJDIR)/fgetc_unlocked.o $(OBJDIR)/fread.o $(OBJDIR)/ungetc.o: dietstdio.h

# CFLAGS+=-W -Wshadow -Wid-clash-31 -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings

RELEASE_NAME=klibc-$(VERSION)
DISTFILES = $(shell find . \( -not -name '.' \) -print | grep -v CVS | grep -v "\.tar\.gz" | grep -v "\/\." | grep -v releases )
distdir := $(RELEASE_NAME)
srcdir = .
release: clean
	@echo $(DISTFILES)
	@-rm -rf $(distdir)
	@mkdir $(distdir)
	@-chmod 777 $(distdir)
	@for file in $(DISTFILES); do                   \
		if test -d $$file; then                 \
			mkdir $(distdir)/$$file;        \
	        else                                    \
	                cp -p $$file $(distdir)/$$file; \
	        fi;                                     \
	done
	@tar -c $(distdir) | gzip -9 > $(RELEASE_NAME).tar.gz
	@rm -rf $(distdir)
	@echo "Built $(RELEASE_NAME).tar.gz"

