#!/bin/sh
TARGET_DIR=./source3
cd $TARGET_DIR
CC=$2 CXX=$3 ./configure --host=$1 --enable-shared-libs=no --with-smbmount --disable-cups CFLAGS='-g -O2 -ffunction-sectionsa' LDFLAGS='-ffunction-sections  -Wl,--gc-sections'
echo "#define HAVE_IFACE_IFCONF 1" >> include/config.h
echo "#define HAVE_C99_VSNPRINTF 1" >> include/config.h
