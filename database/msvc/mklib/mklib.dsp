# Microsoft Developer Studio Project File - Name="mklib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mklib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mklib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mklib.mak" CFG="mklib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mklib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mklib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mklib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O1 /I "../../../../Include" /I "../../../../../Include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x404 /d "NDEBUG"
# ADD RSC /l 0x404 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mklib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../../../Include" /I "../../../../../Include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x404 /d "_DEBUG"
# ADD RSC /l 0x404 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "mklib - Win32 Release"
# Name "mklib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\common\src\carray.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\datadriver.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\datafile\src\datafile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\datafile\src\dfhandlearray.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ebarrays.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ebase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ebasedef.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\eberror.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ebglobal.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\eblock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\ebtypedkey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\hashtable.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\indexdrivers\idxint32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\indexdriver.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\datadrivers\int32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\datadrivers\raw.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\record.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\common\include\carray.h
# End Source File
# Begin Source File

SOURCE=..\..\src\datadriver.h
# End Source File
# Begin Source File

SOURCE=..\..\src\datafile\include\datafile.h
# End Source File
# Begin Source File

SOURCE=..\..\src\datafile\include\dfhandlearray.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ebarrays.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\Utility\database\ebase.h
# End Source File
# Begin Source File

SOURCE=..\..\src\ebase_impl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Include\Utility\database\ebglobal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\fielddef.h
# End Source File
# Begin Source File

SOURCE=..\..\src\hashtable.h
# End Source File
# Begin Source File

SOURCE=..\..\src\indexdriver.h
# End Source File
# Begin Source File

SOURCE=..\..\src\internal.h
# End Source File
# Begin Source File

SOURCE=..\..\src\record.h
# End Source File
# End Group
# End Target
# End Project
