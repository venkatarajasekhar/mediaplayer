-include ../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../include/MakeConfig
# system config
SHELL = /bin/sh

# variables
SRCDIR = ./src
SUBDIRS =  datadrivers  indexdrivers
SUBMODULES = common datafile

EXPORT_HEADER_DIR = ../../Include

CXX = $(MYCXX)

# build a list of all object files
# get a list of first level src files
SRCS = $(wildcard $(SRCDIR)/*.cpp)

# get the second level (subdir) src files
subdir_files = $(wildcard $(SRCDIR)/$(subdir)/*.cpp)
SRCS += $(foreach subdir, $(SUBDIRS), $(subdir_files))

# get the object files from sub-modules
submodule_files = $(wildcard $(SRCDIR)/$(submodule)/src/*.cpp)
SRCS += $(foreach submodule, $(SUBMODULES), $(submodule_files))

# get all ascii files
EXPORT_HEADER_FILES = $(wildcard $(EXPORT_HEADER_DIR)/*.h)
ASCII_FILES = $(SRCS)  \
	$(wildcard SRCDIR/*.h) \
	$(wildcard $(EXPORT_HEADER_DIR)/*.h)  \
	$(wildcard ./doc/*) \
	$(wildcard ./test/*)
subdir_hfiles = $(wildcard $(SRCDIR)/$(subdir)/*.h)
ASCII_FILES += $(foreach subdir, $(SUBDIRS), $(subdir_hfiles))
submodule_hfiles = $(wildcard $(SRCDIR)/$(submodule)/include/*.h)
ASCII_FILES += $(foreach submodule, $(SUBMODULES), $(submodule_hfiles))

OBJS = $(SRCS:.cpp=.o)

# for gcc 2.7.x use -fhandle-exceptions flag
#CXXFLAGS = -g -fhandle-exceptions
ifneq ($(TARGET_BOARD), NO)
DEFINES += -DTARGET_BOARD
endif

CXXFLAGS = $(DEFINES) -g -I$(EXPORT_HEADER_DIR) 

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(DEFINES) $< -o $@

# all : default usertest 
all : default lib 

default: 
	-@echo make targets : lib, mytest, usertest, clean
	-@echo default target: lib
	
lib : ./output/libebase.a ../../../lib/libebase.a
	-@echo libebase is created under ./output

./output/libebase.a : ./output $(OBJS)
	$(MYAR) crv $@ $(OBJS) 
	cp $@ ../../../lib

./output : 
	mkdir ./output

../../../lib/libebase.a:
	-@echo libebase is not in ../../../lib
	cp ./output/libebase.a ../../../lib

mytest : ./output/mytest

usertest : ./output/usertest
	
./output/mytest : ./output/libebase.a ./test/mytest.o
	$(CXX) -o ./output/mytest ./test/mytest.o ./output/libebase.a -lpthread
	rm ./test/mytest.o

./output/usertest : ./output/libebase.a ./test/usertest.o
	$(CXX) -o ./output/usertest ./test/usertest.o ./output/libebase.a -lpthread
	rm ./test/usertest.o

clean :
	rm -f $(OBJS)
	rm -f ./output/*
	rm -f ./msvc/mklib/Debug/*
	rm -f ./msvc/mklib/Release/*
	rm -f ./msvc/test/Debug/*
	rm -f ./msvc/test/Release/*

# dependencies
# $(OBJS) : $(EXPORT_HEADER_DIR)/Utility/database/ebase.h $(EXPORT_HEADER_DIR)/Utility/database/ebglobal.h $(EXPORT_HEADER_DIR)/Utility/database/ebarrays.h ./src/internal.h
$(OBJS) : $(EXPORT_HEADER_DIR)/Utility/database/ebase.h $(EXPORT_HEADER_DIR)/Utility/database/ebglobal.h ./src/ebase_impl.h ./src/ebarrays.h ./src/internal.h
