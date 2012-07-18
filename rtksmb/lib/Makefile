##----------------------------------------------------------------------------
# Created with genmake.pl v1.1 on Thu Jun 16 13:32:16 2005
# genmake.pl home: http://muquit.com/muquit/software/
# Copryright: GNU GPL (http://www.gnu.org/copyleft/gpl.html)
##----------------------------------------------------------------------------
-include ../../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
##include ../../../../../include/MakeConfig
include ../../../../../include/MakeConfig
rm=/bin/rm -f
CXX = $(MYCXX)
DEFS=  
AR= $(MYAR) cq
RANLIB= $(MYRANLIB)
LIBNAME= ../../../../../lib/librtksmb.a

INCLUDES=  -I../../../../Include/ 

#DEFS= $(INCLUDES) $(DEFINES) -DSYS_UNIX=1

#CFLAGS= -g $(DEFS) -Wall
CFLAGS= -DHAVE_CONFIG_H -I. $(INCLUDES) -g -O2 -Wall -fno-exceptions

SRCS = rtksmblib.cpp

OBJS = rtksmblib.o

.cpp.o:
	$(rm) -f $@
	$(CXX) $(CFLAGS) -c $*.cpp

all: $(LIBNAME)

$(LIBNAME) : $(OBJS)
	$(rm) $@
	$(AR) $@ $(OBJS)
	$(RANLIB) $@

clean:
	$(rm) $(OBJS) $(LIBNAME) core *~
