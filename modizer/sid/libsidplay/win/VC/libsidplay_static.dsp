# Microsoft Developer Studio Project File - Name="libsidplay_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libsidplay_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsidplay_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsidplay_static.mak" CFG="libsidplay_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsidplay_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libsidplay_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsidplay_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseStatic"
# PROP Intermediate_Dir "ReleaseStatic"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /I "../../include/sidplay" /D "NDEBUG" /D "WIN32" /D "__WINDOWS" /D "MBCS" /D "_LIB" /D "HAVE_MSWINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../../bin_vc6/Release/libsidplay_static.lib"

!ELSEIF  "$(CFG)" == "libsidplay_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugStatic"
# PROP Intermediate_Dir "DebugStatic"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../include" /I "../../include/sidplay" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_LIB" /D "HAVE_MSWINDOWS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../../../bin_vc6/Debug/libsidplay_static.lib"

!ENDIF 

# Begin Target

# Name "libsidplay_static - Win32 Release"
# Name "libsidplay_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\char.bin
# End Source File
# Begin Source File

SOURCE=..\..\src\config.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\event.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mixer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\player.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\prg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\psiddrv.a65
# End Source File
# Begin Source File

SOURCE=..\..\src\psiddrv.bin
# End Source File
# Begin Source File

SOURCE=..\..\src\psiddrv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\reloc65.c
# End Source File
# Begin Source File

SOURCE=..\..\src\sidplay2.cpp
# End Source File
# End Group
# Begin Group "Header Files (Public)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\sidplay\c64env.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\component.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\event.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sid2types.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidbuilder.h
# End Source File
# Begin Source File

SOURCE=.\sidconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidendian.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidenv.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidplay.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidplay2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidtypes.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\c64\c64cia.h
# End Source File
# Begin Source File

SOURCE=..\..\src\c64\c64vic.h
# End Source File
# Begin Source File

SOURCE=..\..\src\c64\c64xsid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\config.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=..\..\src\nullsid.h
# End Source File
# Begin Source File

SOURCE=..\..\src\player.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\sidconfig.h
# End Source File
# End Group
# Begin Group "Components"

# PROP Default_Filter ""
# Begin Group "MOS6510"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\mos6510\conf6510.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\mos6510.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\mos6510.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\mos6510c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\mos6510c.i
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\opcodes.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\sid6510c.h
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6510\cycle_based\sid6510c.i
# End Source File
# End Group
# Begin Group "MOS6526"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\mos6526\mos6526.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mos6526\mos6526.h
# End Source File
# End Group
# Begin Group "MOS656X"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\mos656x\mos656x.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\mos656x\mos656x.h
# End Source File
# End Group
# Begin Group "SID6526"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\sid6526\sid6526.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sid6526\sid6526.h
# End Source File
# End Group
# Begin Group "SidTune"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\include\sidplay\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\IconInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\InfoFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\MUS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\p00.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PP20.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PP20.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PP20_Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\PSID.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTune.cpp
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\SidTune.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTuneCfg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTuneTools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\sidtune\SidTuneTools.h
# End Source File
# Begin Source File

SOURCE=..\..\include\sidplay\SmartPtr.h
# End Source File
# End Group
# Begin Group "xSID"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\src\xsid\xsid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\xsid\xsid.h
# End Source File
# End Group
# End Group
# End Target
# End Project
