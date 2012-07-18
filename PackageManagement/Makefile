-include ../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../include/MakeConfig

RM=/bin/rm -f
CC=$(MYCC)
AR=$(MYAR)
RANLIB=$(MYRANLIB)

PACKAGE_DIR=ipkg-0.99.163
TOOL_DIR=ipkg-utils-1.7

all: 
	@if [ ! -r $(PACKAGE_DIR)/Makefile ]; then $(SH) ./preconf.sh $(PACKAGE_DIR) $(HOST) $(MYCC); fi
	$(MAKE) -C $(PACKAGE_DIR) 
	$(MAKE) -C $(PACKAGE_DIR) install

clean: 
	@if [ -r $(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(PACKAGE_DIR) distclean; fi

