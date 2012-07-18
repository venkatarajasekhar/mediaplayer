INCLUDE(../../../include/config.txt)

#SET(CMAKE_C_COMPILER /usr/local/toolchain_mipsel/bin/mipsel-linux-gcc)
#SET(CMAKE_CXX_COMPILER /usr/local/toolchain_mipsel/bin/mipsel-linux-g++)
#message(${CMAKE_CXX_COMPILER})
SET(CC ${CMAKE_C_COMPILER})

#FILE(GLOB testfile build/Makefile)
#message(${testfile})
#IF(NOT testfile)
  message("do shell cmd. ==>produce Makefile for mp3info")
 
EXEC_PROGRAM(make ${CMAKE_CURRENT_SOURCE_DIR} ARGS -f makefile.mp)
#EXEC_PROGRAM(./depend.sh ${CMAKE_CURRENT_SOURCE_DIR} ARGS  ${HOST})
#ENDIF(NOT testfile)

#EXEC_PROGRAM(sed ${CMAKE_CURRENT_SOURCE_DIR}/build ARGS -e 's/CC = mipsel-linux-gcc/CC =
#${CC}/'
#${CMAKE_CURRENT_SOURCE_DIR}/build/Makefile
#>${CMAKE_CURRENT_SOURCE_DIR}/build/makefile_tmp)
#EXEC_PROGRAM(mv ARGS ${CMAKE_CURRENT_SOURCE_DIR}/build/makefile_tmp
#${CMAKE_CURRENT_SOURCE_DIR}/build/Makefile)
#EXEC_PROGRAM(make ${CMAKE_CURRENT_SOURCE_DIR}/build/)

#EXEC_PROGRAM(make ${CMAKE_CURRENT_SOURCE_DIR}/build/ ARGS CC=${CC})

#EXEC_PROGRAM(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/build/src/.libs/libmp.a
#${CMAKE_CURRENT_SOURCE_DIR}/../../../lib)
#EXEC_PROGRAM(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/mplib-1.0.1/src/mplib.h
#${CMAKE_CURRENT_SOURCE_DIR}/../../Include/Utility/mp3info)

#ADD_CUSTOM_TARGET(clean COMMAND rm -fr *~ ${CMAKE_CURRENT_SOURCE_DIR}/lib
#${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/build/*
#COMMAND rm -fr ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/libmp.a
#${CMAKE_CURRENT_SOURCE_DIR}/../../Include/Utility/mp3info/*)
#EXEC_PROGRAM(make build/ ARGS install)
