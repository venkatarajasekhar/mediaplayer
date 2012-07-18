INSTALL_DIR	= $(PROJ_ROOT)/img/fs/root/root/nand

CROSS   = mipsel-linux-
CC      = $(CROSS)gcc
LD      = $(CROSS)ld
AR      = $(CROSS)ar
RANLIB  = $(CROSS)ranlib
STRIP   = $(CROSS)strip

GCCINCDIR := ${shell $(CC) -print-search-dirs | sed -ne "s/install: \(.*\)/\1include/gp"}
CFLAGS := -I$(GCCINCDIR)-O2 -Wall

TARGET	= nand_fs_test 

all: $(TARGET) 

nand_fs_test: nand_fs_test.c
		$(CC) $(CFLAGS) nand_fs_test.c -o nand_fs_test
		$(STRIP) $@
		chmod +x $@
       
distclean clean:
	rm -f *.o $(TARGET)
