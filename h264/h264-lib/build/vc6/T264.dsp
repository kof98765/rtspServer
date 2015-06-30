# Microsoft Developer Studio Project File - Name="T264" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=T264 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "T264.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "T264.mak" CFG="T264 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "T264 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "T264 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "T264 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\bin\"
# PROP BASE Intermediate_Dir "Debug\exe"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin\"
# PROP Intermediate_Dir "Debug\exe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /I "../../common/" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /TC /GZ /c
# ADD CPP /nologo /W3 /Gm /ZI /Od /I "../../common" /I "../../decoder" /I "../../encoder" /I "../../encoder/sse2" /I "../../encoder/plugins" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /TC /GZ /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 $(OutDir)/t264libd.lib /nologo /subsystem:console /pdb:"$(OutDir)/T264.pdb" /debug /machine:IX86 /out:"$(OutDir)/T264.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\bin\t264libd.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:console /pdb:"$(OutDir)/T264.pdb" /debug /machine:IX86 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "T264 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\bin\"
# PROP BASE Intermediate_Dir "Release\exe"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin\"
# PROP Intermediate_Dir "Release\exe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /W3 /Zi /I "../../common/" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /c
# ADD CPP /nologo /W3 /Zi /O2 /I "../../common" /I "../../encoder/plugins" /I "../../encoder" /I "../../encoder/sse2" /I "../../decoder" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 $(OutDir)/t264lib.lib /nologo /subsystem:console /debug /machine:IX86 /out:"$(OutDir)/T264.exe" /pdbtype:sept /opt:ref /opt:icf /fixed:no
# ADD LINK32 ../bin/t264lib.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:console /debug /machine:IX86 /pdbtype:sept /opt:ref /opt:icf /fixed:no

!ENDIF 

# Begin Target

# Name "T264 - Win32 Debug"
# Name "T264 - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx"
# Begin Source File

SOURCE=..\..\encoder\T264.c
DEP_CPP_T264_=\
	"..\..\common\config.h"\
	"..\..\common\inttypes.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\encoder\display.h"\
	"..\..\encoder\win.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\win.c
DEP_CPP_WIN_C=\
	"..\..\encoder\win.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\win.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\yuvrgb24.c
DEP_CPP_YUVRG=\
	"..\..\encoder\display.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc;xsd"
# Begin Source File

SOURCE=..\..\decoder\block.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\cabac.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\cabac_engine.h
# End Source File
# Begin Source File

SOURCE=..\..\decoder\dec_cavlc.h
# End Source File
# End Group
# End Target
# End Project
