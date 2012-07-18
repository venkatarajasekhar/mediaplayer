-include ../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../../include/MakeConfig

SUBDIRS = exe lib

all: 
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done 
clean: 
	set -e; for i in $(SUBDIRS); do $(MAKE) clean -C $$i; done

f:
	@echo $(subst |, , $(APCFG_FLAGS))
