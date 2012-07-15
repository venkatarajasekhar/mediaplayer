#
# neon source directory Makefile
# 
# Use the NEON_NORMAL_BUILD or NEON_LIBTOOL_BUILD autoconf 
# macros to set up this Makefile correctly.
#

SHELL = /bin/sh

# Installation paths
prefix = /usr/local
exec_prefix = ${prefix}
libdir = ${exec_prefix}/lib

# Build paths

top_builddir = ..
top_srcdir = ..

# Toolchain settings.
CC = /usr/local/toolchain_mipsel/bin/mipsel-linux-gcc
AR = /usr/local/toolchain_mipsel/bin/mipsel-linux-ar
RANLIB = /usr/local/toolchain_mipsel/bin/mipsel-linux-ranlib
LIBTOOL = $(SHELL) $(top_builddir)/libtool

# Flags
CPPFLAGS = -DHAVE_CONFIG_H  -D_LARGEFILE64_SOURCE -DNE_LFS  -I${top_builddir}
CFLAGS = -g -O2 -prefer-pic
LDFLAGS = 
NEON_LINK_FLAGS = -version-info 28:6:1 -export-symbols-regex '^ne_[^_]'
# Note: don't substitute  in here; during a bundled
# build of this directory,  may include -lneon.
LIBS =  -lz -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err -lexpat 

COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS)
LINK = $(LIBTOOL) --mode=link $(CC) -no-undefined $(LDFLAGS)

NEON_BASEOBJS = ne_request.lo ne_session.lo 	    \
	ne_basic.lo  ne_string.lo 		    \
	ne_uri.lo ne_dates.lo ne_alloc.lo  \
	ne_md5.lo ne_utils.lo    \
	ne_socket.lo ne_auth.lo 			    \
	ne_redirect.lo ne_compress.lo		    \
	ne_i18n.lo ne_pkcs11.lo

NEON_DAVOBJS = $(NEON_BASEOBJS) \
	ne_207.lo ne_xml.lo \
	ne_props.lo ne_locks.lo \
	ne_xmlreq.lo

OBJECTS =  $(NEON_BASEOBJS)  ne_acl.lo ne_stubssl.lo

.SUFFIXES:
.SUFFIXES: .c .lo .o

NEON_TARGET = libneon.la

# Thanks to gettext for this neat trick.
all: all-yes

all-yes: $(NEON_TARGET)
all-no:
	@echo "Bundled neon build not being used."

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $< -o $@
.c.o:
	$(COMPILE) -c $< -o $@

libneon.la: $(OBJECTS)
	$(LINK) -rpath $(libdir) $(NEON_LINK_FLAGS) -o $@ $(OBJECTS) $(LIBS)

libneon.a: $(OBJECTS)
	@rm -f $@
	$(AR) cru $@ $(OBJECTS)
	$(RANLIB) $@

clean:
	rm -f $(NEON_TARGET) *.o *.lo *.bbg *.bb *.gc*
	rm -rf .libs

c++.c:
	find . -name ne_\*.h -print | sed 's/.*/#include "&"/;/ne_priv/d' > $@
	echo "int main(void) {}" >> $@

check-c++: c++.c
	c++ -I. c++.c

check-incl:
	@for f in ne_*.h; do \
	 echo Checking $$f...; \
	 echo "#include \"$$f\"" > checkincl.c; \
	 $(COMPILE) -c checkincl.c -o checkincl.o || exit 1; done

# Update generated dependencies below; requires cc -MM as in gcc.
update-deps:
	for f in `echo $(OBJECTS) | sed 's/\\.lo/.c/g'`; do \
	  $(CC) $(CPPFLAGS) -MM -c $$f; \
	done | sed 's, \.\./, $$(top_builddir)/,g;s, /[^ ]*.h,,g;/^  .$$/d;s/\.o: /.@NEON''_OBJEXT@: /' > new-deps
	sed '/[-]--CUT---/q' Makefile.in > Makefile.new
	cat Makefile.new new-deps > Makefile.in
	rm new-deps Makefile.new

#### ---CUT--- DO NOT REMOVE THIS LINE.  Generated dependencies follow.  ####
ne_request.lo: ne_request.c $(top_builddir)/config.h ne_internal.h ne_defs.h ne_alloc.h \
  ne_request.h ne_utils.h ne_string.h ne_session.h ne_ssl.h ne_uri.h \
  ne_socket.h ne_private.h ne_pkcs11.h
ne_session.lo: ne_session.c $(top_builddir)/config.h ne_session.h ne_ssl.h ne_defs.h \
  ne_uri.h ne_socket.h ne_alloc.h ne_utils.h ne_internal.h ne_string.h \
  ne_dates.h ne_private.h ne_request.h ne_pkcs11.h
ne_basic.lo: ne_basic.c $(top_builddir)/config.h ne_request.h ne_utils.h ne_defs.h \
  ne_string.h ne_alloc.h ne_session.h ne_ssl.h ne_uri.h ne_socket.h \
  ne_basic.h ne_207.h ne_xml.h ne_locks.h ne_dates.h ne_internal.h
ne_string.lo: ne_string.c $(top_builddir)/config.h ne_alloc.h ne_defs.h ne_string.h
ne_uri.lo: ne_uri.c $(top_builddir)/config.h ne_string.h ne_defs.h ne_alloc.h ne_uri.h
ne_dates.lo: ne_dates.c $(top_builddir)/config.h ne_alloc.h ne_defs.h ne_dates.h \
  ne_string.h
ne_alloc.lo: ne_alloc.c $(top_builddir)/config.h ne_alloc.h ne_defs.h
ne_md5.lo: ne_md5.c $(top_builddir)/config.h ne_md5.h ne_defs.h ne_string.h ne_alloc.h
ne_utils.lo: ne_utils.c $(top_builddir)/config.h ne_utils.h ne_defs.h ne_string.h \
  ne_alloc.h ne_dates.h
ne_socket.lo: ne_socket.c $(top_builddir)/config.h ne_privssl.h ne_ssl.h ne_defs.h \
  ne_socket.h ne_internal.h ne_utils.h ne_string.h ne_alloc.h ne_sspi.h
ne_auth.lo: ne_auth.c $(top_builddir)/config.h ne_md5.h ne_defs.h ne_dates.h \
  ne_request.h ne_utils.h ne_string.h ne_alloc.h ne_session.h ne_ssl.h \
  ne_uri.h ne_socket.h ne_auth.h ne_internal.h
ne_redirect.lo: ne_redirect.c $(top_builddir)/config.h ne_session.h ne_ssl.h ne_defs.h \
  ne_uri.h ne_socket.h ne_request.h ne_utils.h ne_string.h ne_alloc.h \
  ne_redirect.h ne_internal.h
ne_compress.lo: ne_compress.c $(top_builddir)/config.h ne_request.h ne_utils.h \
  ne_defs.h ne_string.h ne_alloc.h ne_session.h ne_ssl.h ne_uri.h \
  ne_socket.h ne_compress.h ne_internal.h
ne_i18n.lo: ne_i18n.c $(top_builddir)/config.h ne_i18n.h ne_defs.h
ne_pkcs11.lo: ne_pkcs11.c $(top_builddir)/config.h ne_pkcs11.h ne_defs.h ne_session.h \
  ne_ssl.h ne_uri.h ne_socket.h
ne_207.lo: ne_207.c $(top_builddir)/config.h ne_alloc.h ne_defs.h ne_utils.h ne_xml.h \
  ne_207.h ne_request.h ne_string.h ne_session.h ne_ssl.h ne_uri.h \
  ne_socket.h ne_basic.h ne_internal.h
ne_xml.lo: ne_xml.c $(top_builddir)/config.h ne_internal.h ne_defs.h ne_alloc.h \
  ne_xml.h ne_utils.h ne_string.h
ne_props.lo: ne_props.c $(top_builddir)/config.h ne_alloc.h ne_defs.h ne_xml.h \
  ne_props.h ne_request.h ne_utils.h ne_string.h ne_session.h ne_ssl.h \
  ne_uri.h ne_socket.h ne_207.h ne_basic.h ne_locks.h ne_internal.h
ne_locks.lo: ne_locks.c $(top_builddir)/config.h ne_alloc.h ne_defs.h ne_request.h \
  ne_utils.h ne_string.h ne_session.h ne_ssl.h ne_uri.h ne_socket.h \
  ne_xml.h ne_locks.h ne_basic.h ne_props.h ne_207.h ne_internal.h \
  ne_xmlreq.h
ne_xmlreq.lo: ne_xmlreq.c $(top_builddir)/config.h ne_internal.h ne_defs.h ne_xmlreq.h \
  ne_request.h ne_utils.h ne_string.h ne_alloc.h ne_session.h ne_ssl.h \
  ne_uri.h ne_socket.h ne_xml.h ne_basic.h
ne_acl.lo: ne_acl.c $(top_builddir)/config.h ne_request.h ne_utils.h ne_defs.h \
  ne_string.h ne_alloc.h ne_session.h ne_ssl.h ne_uri.h ne_socket.h \
  ne_locks.h ne_acl.h ne_xml.h
ne_openssl.lo: ne_openssl.c $(top_builddir)/config.h ne_ssl.h ne_defs.h ne_string.h \
  ne_alloc.h ne_session.h ne_uri.h ne_socket.h ne_internal.h ne_private.h \
  ne_request.h ne_utils.h ne_pkcs11.h ne_privssl.h
