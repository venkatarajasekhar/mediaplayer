-include ../../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../../../include/MakeConfig

MYDEFS=
rm=/bin/rm -f
cp=/bin/cp -f
CC= $(MYCXX)
AR= $(MYAR) cq
RANLIB=$(MYRANLIB)
PROGRAM= rtksmb

INCLUDES=  -I../../../../Include/

CFLAGS= $(INCLUDES) -DSYS_UNIX=1 $(MYDEFS) -ggdb -D_REENTRANT -static -ffunction-sections  

SRCS = rtksmb.cpp

OBJS = rtksmb.o

LIBS = -L../../../../../lib_release/ -L../../../../../lib -lSamba$(TOOLCHAIN_VER)$(SAMBA_VER) -lini
#LIBS += -L. -lefence -lpthread
LIBS+= -lresolv -ldl

.cpp.o:
	$(rm) -f $@
	$(CC) $(CFLAGS) $(DEBUGFLAG) $(WARNING) -c $*.cpp

all: $(PROGRAM)
       	
$(PROGRAM) : $(OBJS)
	$(CC) -o $(PROGRAM) $(CFLAGS) $(OBJS) $(LIBS) -ffunction-sections -Wl,--gc-sections
	

clean:
	$(rm) $(OBJS) $(PROGRAM) core *~
