-include ../../../../branch_src_sharedMemory_integration/Tools/MConfig/.config
include ../../../../include/MakeConfig
CXX=$(CROSS_COMPILE)g++
all: usertest

INCLUDE_PATH = ../../../Include

usertest: usertest.o
	$(CXX) -o usertest usertest.o -L../../../../lib -lmp

usertest.o: usertest.cpp
	$(CXX) -c -I$(INCLUDE_PATH) usertest.cpp

clean:
	rm -rf *.o usertest *~
