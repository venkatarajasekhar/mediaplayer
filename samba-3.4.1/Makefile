-include ../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../../include/MakeConfig

rm = /bin/rm -f
CXX=$(MYCXX)
CC=$(MYCC)
AR=$(MYAR)
AR2=$(MYRANLIB)


rm = /bin/rm -f
SUBDIRS  = ./source3
CFLAGS += -I$(INC_DIR)
LDFLAGS +=
CFLAGS += $(DEFINES) -Wall

all: 
	@if [ ! -r $(SUBDIRS)/Makefile ]; then $(SH) ./preconf$(TOOLCHAIN_VER).sh $(HOST) $(MYCC) $(MYCXX); fi
	$(MAKE) -C $(SUBDIRS) 
	$(MAKE) -C $(SUBDIRS) -f Makefile2


clean: 
	$(MAKE) -C $(SUBDIRS) clean
	$(MAKE) -C $(SUBDIRS) -f Makefile2 clean
	$(rm) $(SUBDIRS)/Makefile

distclean:
	@if [ -r $(SUBDIRS)/Makefile ]; then $(MAKE) -C $(SUBDIRS) distclean; fi


