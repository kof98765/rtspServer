# Microsoft Developer Studio Project File - Name="t264lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=t264lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "t264lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "t264lib.mak" CFG="t264lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "t264lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "t264lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "t264lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\bin\"
# PROP BASE Intermediate_Dir "Debug\lib"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bin\"
# PROP Intermediate_Dir "Debug\lib"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /I "../../common" /D "WIN32" /D "_DEBUG" /D "_LIB" /D "_MBCS" /TC /GZ /c
# ADD CPP /nologo /W3 /Gm /ZI /Od /I "../../common" /I "../../decoder" /I "../../encoder" /I "../../encoder/sse2" /I "../../encoder/plugins" /D "WIN32" /D "_DEBUG" /D "_LIB" /D "_MBCS" /FR /TC /GZ /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"$(OutDir)/t264libd.lib"
# ADD LIB32 /nologo /out:"../bin/t264libd.lib"

!ELSEIF  "$(CFG)" == "t264lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\bin\"
# PROP BASE Intermediate_Dir "Release\lib"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin\"
# PROP Intermediate_Dir "Release\lib"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /W3 /Zi /I "../../common" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_MBCS" /TC /c
# ADD CPP /nologo /W3 /Zi /O2 /Ob2 /I "../../common" /I "../../encoder/plugins" /I "../../encoder" /I "../../encoder/sse2" /I "../../decoder" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_MBCS" /TC /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"$(OutDir)/t264lib.lib"
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "t264lib - Win32 Debug"
# Name "t264lib - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx"
# Begin Group "asm"

# PROP Default_Filter "asm"
# Begin Source File

SOURCE=..\..\encoder\sse2\cpu.asm

!IF  "$(CFG)" == "t264lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug\lib
InputPath=..\..\encoder\sse2\cpu.asm
InputName=cpu

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "t264lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release\lib
InputPath=..\..\encoder\sse2\cpu.asm
InputName=cpu

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\encoder\sse2\dct_sse2.asm

!IF  "$(CFG)" == "t264lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug\lib
InputPath=..\..\encoder\sse2\dct_sse2.asm
InputName=dct_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "t264lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release\lib
InputPath=..\..\encoder\sse2\dct_sse2.asm
InputName=dct_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\encoder\sse2\interpolate_sse2.asm

!IF  "$(CFG)" == "t264lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug\lib
InputPath=..\..\encoder\sse2\interpolate_sse2.asm
InputName=interpolate_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "t264lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release\lib
InputPath=..\..\encoder\sse2\interpolate_sse2.asm
InputName=interpolate_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\encoder\sse2\sad.asm

!IF  "$(CFG)" == "t264lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug\lib
InputPath=..\..\encoder\sse2\sad.asm
InputName=sad

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "t264lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release\lib
InputPath=..\..\encoder\sse2\sad.asm
InputName=sad

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\encoder\sse2\utility_mmx.asm

!IF  "$(CFG)" == "t264lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Debug\lib
InputPath=..\..\encoder\sse2\utility_mmx.asm
InputName=utility_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "t264lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\Release\lib
InputPath=..\..\encoder\sse2\utility_mmx.asm
InputName=utility_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -Xvc -f win32 -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\..\decoder\block.c
DEP_CPP_BLOCK=\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\decoder\block.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\cabac.c
DEP_CPP_CABAC=\
	"..\..\common\bitstream.h"\
	"..\..\common\cabac_engine.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\inter.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\common\cabac_engine.c
DEP_CPP_CABAC_=\
	"..\..\common\bitstream.h"\
	"..\..\common\cabac_engine.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\cavlc.c
DEP_CPP_CAVLC=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\vlc.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\common\dct.c
DEP_CPP_DCT_C=\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\common\deblock.c
DEP_CPP_DEBLO=\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\decoder\dec_cabac.c
DEP_CPP_DEC_C=\
	"..\..\common\bitstream.h"\
	"..\..\common\cabac_engine.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\inter_b.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\decoder\dec_cavlc.c
DEP_CPP_DEC_CA=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\encoder\cavlc.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\inter_b.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\estimation.c
DEP_CPP_ESTIM=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\estimation.h"\
	"..\..\encoder\inter.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\inter.c
DEP_CPP_INTER=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\encoder\estimation.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\interpolate.h"\
	"..\..\encoder\intra.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\inter_b.c
DEP_CPP_INTER_=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\decoder\block.h"\
	"..\..\encoder\estimation.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\inter_b.h"\
	"..\..\encoder\interpolate.h"\
	"..\..\encoder\intra.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\interpolate.c
DEP_CPP_INTERP=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\interpolate.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\intra.c
DEP_CPP_INTRA=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\encoder\cavlc.h"\
	"..\..\encoder\intra.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\predict.c
DEP_CPP_PREDI=\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\encoder\predict.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\plugins\ratecontrol.c
DEP_CPP_RATEC=\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\plugins\ratecontrol.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\rbsp.c
DEP_CPP_RBSP_=\
	"..\..\common\bitstream.h"\
	"..\..\common\cabac_engine.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\rbsp.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\plugins\stat.c
DEP_CPP_STAT_=\
	"..\..\common\config.h"\
	"..\..\common\deblock.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\decoder\T264dec.c
DEP_CPP_T264D=\
	"..\..\common\bitstream.h"\
	"..\..\common\cabac_engine.h"\
	"..\..\common\config.h"\
	"..\..\common\dct.h"\
	"..\..\common\deblock.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\decoder\block.h"\
	"..\..\decoder\dec_cabac.h"\
	"..\..\decoder\dec_cavlc.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\interpolate.h"\
	"..\..\encoder\sse2\sse2.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\t264enc.c
DEP_CPP_T264E=\
	"..\..\common\bitstream.h"\
	"..\..\common\cabac_engine.h"\
	"..\..\common\config.h"\
	"..\..\common\dct.h"\
	"..\..\common\deblock.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\common\utility.h"\
	"..\..\encoder\cabac.h"\
	"..\..\encoder\cavlc.h"\
	"..\..\encoder\estimation.h"\
	"..\..\encoder\inter.h"\
	"..\..\encoder\inter_b.h"\
	"..\..\encoder\interpolate.h"\
	"..\..\encoder\intra.h"\
	"..\..\encoder\plugins\ratecontrol.h"\
	"..\..\encoder\predict.h"\
	"..\..\encoder\rbsp.h"\
	"..\..\encoder\sse2\sse2.h"\
	"..\..\encoder\typedecision.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\encoder\typedecision.c
DEP_CPP_TYPED=\
	"..\..\common\bitstream.h"\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\T264.h"\
	"..\..\encoder\estimation.h"\
	"..\..\encoder\typedecision.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\common\utility.c
DEP_CPP_UTILI=\
	"..\..\common\config.h"\
	"..\..\common\portab.h"\
	"..\..\common\utility.h"\
	
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc;xsd"
# Begin Source File

SOURCE=..\..\common\bitstream.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\cavlc.h
# End Source File
# Begin Source File

SOURCE=..\..\common\config.h
# End Source File
# Begin Source File

SOURCE=..\..\common\dct.h
# End Source File
# Begin Source File

SOURCE=..\..\common\deblock.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\estimation.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\inter.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\inter_b.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\interpolate.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\intra.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inttypes.h
# End Source File
# Begin Source File

SOURCE=..\..\common\portab.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\predict.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\plugins\ratecontrol.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\rbsp.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\plugins\stat.h
# End Source File
# Begin Source File

SOURCE=..\..\common\T264.h
# End Source File
# Begin Source File

SOURCE=..\..\common\timer.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\typedecision.h
# End Source File
# Begin Source File

SOURCE=..\..\common\utility.h
# End Source File
# Begin Source File

SOURCE=..\..\encoder\vlc.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx"
# End Group
# End Target
# End Project
