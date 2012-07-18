#!/bin/sh
# CONFIGURATION SCRIPT
# Refined to fit DVR Project Makefile Hierarchy
MPLIB=mplib-1.0.1/

BUILD_DIR=build/

cd $BUILD_DIR
../$MPLIB/configure --host=$1
