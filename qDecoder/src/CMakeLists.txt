INCLUDE(../../../../include/config.txt)

SET(LIBNAME qDecoder)
SET(LIBRARY_OUTPUT_PATH ../../../../lib/)


SET(prefix " /usr/local")
SET(exec_prefix " ${prefix}")

SET(PRGNAME qDecoder)

#SET(LIBNAME ../../../../lib/lib${PRGNAME}.a)

SET(SLIBVERSION 2)
SET(SLIBNAME lib${PRGNAME}.so)
SET(SLIBREALNAME " ${SLIBNAME}.${SLIBVERSION}")

SET(LIBDIR " ${exec_prefix}/lib")
SET(HEADERDIR " ${prefix}/include")

SET(CC " ${MYCC}")
SET(CFLAGS " -g -O2")

SET(DEFS " -DSTDC_HEADERS=1 -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_STDARG_H=1 -DHAVE_CTYPE_H=1 -DHAVE_TIME_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_DIRENT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_FILE_H=1 -DHAVE_STRFTIME=1 -DHAVE_VPRINTF=1 -DHAVE_STRDUP=1 -DHAVE_STRSTR=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_OPENDIR=1 -DHAVE_CLOSEDIR=1 -DHAVE_FLOCK=1")
SET(INCLUDES  ${INCLUDES} " ${CMAKE_CURRENT_SOURCE_DIR}/.")


SET(AR " ${MYAR}")
SET(ARFLAGS " -rc")
SET(CHMOD " /bin/chmod")
SET(INSTALL " /usr/bin/install -c -m 444")
SET(LD " ${MYCXX}")
SET(LDFLAGS " -shared")
SET(LN_S ln -s)
SET(RANLIB " ${MYRANLIB}")
SET(RM " /bin/rm")

SET(SRCS  qDecoder.c      qSession.c               qfDecoder.c
qsDecoder.c             qHttpHeader.c          qError.c           qEnv.c
qEncode.c           qString.c          qFile.c      qValid.c          qArg.c
qAwk.c           qSed.c         qCount.c          qDownload.c
qTime.c          qSocket.c         qMisc.c         qInternalCommon.c
qInternalEntry.c         md5/md5c.c)




INCLUDE_DIRECTORIES(${INCLUDES})
ADD_DEFINITIONS(${CFLAGS} ${DEBUGFLAG} ${WARNING})
ADD_LIBRARY(${LIBNAME} ${SRCS})
string(REGEX REPLACE  ";" " " APCFG_DVR  ${APCFG_DVR})
string(REGEX REPLACE  ";" " " CHIP_MODEL_DEFS ${CHIP_MODEL_DEFS})
string(REGEX REPLACE  ";" " " APCFG_DEFS ${APCFG_DEFS})
#string(REGEX REPLACE  ";" " " BUILD_OPTIONS ${BUILD_OPTIONS})
string(REGEX REPLACE  ";" " " DEFINES ${DEFINES})
string(REGEX REPLACE  ";" " " APCFG_FLAGS ${APCFG_FLAGS})
