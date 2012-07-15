#!/bin/sh
# Refined to fit DVR Project Makefile Hierarchy
PACKAGE_DIR=$1
cd $PACKAGE_DIR
PREFIX=`cd ../ipkg && pwd`
BINDIR=`cd .. && pwd`
#INCDIR=`cd ../../../Include && pwd`
#LIBDIR=`cd ../../../../lib && pwd`
CC=$3 ./configure --prefix=$PREFIX --exec-prefix=$PREFIX --bindir=$BINDIR --host=$2 --enable-shared=no --with-ipkglibdir=/tmp/package
