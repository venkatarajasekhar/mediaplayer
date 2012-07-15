-include ../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../include/MakeConfig
CXX=$(MYCXX)
CC=$(MYCC)
MPLIB=mplib-1.0.1/

PREFIX=`pwd`
BUILD_DIR=build

all: depend copy 
#simple_example

depend: 
	@if [ ! -r $(BUILD_DIR)/Makefile ]; then $(SH) CC=$(CC) ./preconf.sh $(HOST); fi
	$(MAKE) -C $(BUILD_DIR) prefix=$(PREFIX) 
	$(MAKE) -C $(BUILD_DIR) prefix=$(PREFIX) install

copy:
	cp lib/libmp.a ../../../lib 
	cp include/mplib.h ../../Include/Utility/mp3info
        
#simple_example:
#	$(MAKE) -C example/ 
        
clean:
	rm -rf *~ lib include $(BUILD_DIR)/*
	rm -rf ../../../lib/libmp.a ../../Include/Utility/mp3info/*
	$(MAKE) -C example/ clean
	
