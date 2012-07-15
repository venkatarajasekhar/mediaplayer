INCLUDE(../../../include/config.txt)

SET(LIBNAME ebase)
SET(LIBRARY_OUTPUT_PATH ../../../lib/)

SET(SHELL " /bin/sh")





IF(NOT TARGET_BOARD STREQUAL NO)
SET(DEFINES ${DEFINES} " -DTARGET_BOARD")
ENDIF(NOT TARGET_BOARD STREQUAL NO)

SET(INCLUDES ${INCLUDES} ../../Include)

SET(CFLAGS ${CFLAGS} ${DEFINES})

SET(SRCS src/datadriver.cpp src/ebarrays.cpp src/ebase.cpp src/ebasedef.cpp
src/eberror.cpp src/ebglobal.cpp src/eblock.cpp src/ebtypedkey.cpp
src/hashtable.cpp src/indexdriver.cpp  src/myebase.cpp src/record.cpp
src/datadrivers/int32.cpp src/datadrivers/raw.cpp
src/indexdrivers/idxint32.cpp src/common/src/carray.cpp
src/datafile/src/datafile.cpp src/datafile/src/dfhandlearray.cpp)


INCLUDE_DIRECTORIES(${INCLUDES})
ADD_DEFINITIONS(${CFLAGS} ${DEBUGFLAG} ${WARNING})
ADD_LIBRARY(${LIBNAME} ${SRCS})
string(REGEX REPLACE  ";" " " APCFG_DVR  ${APCFG_DVR})
string(REGEX REPLACE  ";" " " CHIP_MODEL_DEFS ${CHIP_MODEL_DEFS})
string(REGEX REPLACE  ";" " " APCFG_DEFS ${APCFG_DEFS})
#string(REGEX REPLACE  ";" " " BUILD_OPTIONS ${BUILD_OPTIONS})
string(REGEX REPLACE  ";" " " DEFINES ${DEFINES})
string(REGEX REPLACE  ";" " " APCFG_FLAGS ${APCFG_FLAGS})
