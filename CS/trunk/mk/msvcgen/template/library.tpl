# Microsoft Developer Studio Project File - Name="%project%" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=%project% - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "%makefile%".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "%makefile%" CFG="%project% - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "%project% - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "%project% - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "%project% - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "csrelease"
# PROP BASE Intermediate_Dir "csrelease"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "csrelease\bin\libs"
# PROP Intermediate_Dir "csrelease\temp\%project%"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /vmb /vms /Gy /GF /W4 /Gm /G5 /Ob2 /Og /Oi /Ot /Oy /D "WIN32" /D "NDEBUG" /D "_LIB" /FD /c
# ADD CPP /nologo /vmb /vms /Gy /GF /MD /W4 /Gm /G5 /Ob2 /Og /Oi /Ot /Oy /I "..\.." /I "..\..\include\cssys\win32" /I "..\..\include" /I "..\..\libs" /I "..\..\support" /I "..\..\apps" /I "..\..\plugins" /D "NDEBUG" /D "_LIB" /D "WIN32" /D "_WINDOWS" /D "WIN32_VOLATILE" /D "__CRYSTAL_SPACE__" /FD %cflags% /c
# ADD BASE RSC /l 0x409 /d "NDEBUG" /i "..\..\include" /i "..\..\include\cssys\win32"
# ADD RSC /l 0x409 /d "NDEBUG" /i "..\..\include" /i "..\..\include\cssys\win32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=echo File is a lib, Copy skipped.
# End Special Build Tool

!ELSEIF  "$(CFG)" == "%project% - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "csdebug"
# PROP BASE Intermediate_Dir "csdebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "csdebug\bin\libs"
# PROP Intermediate_Dir "csdebug\temp\%project%"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /vmb /vms /W4 /Gm /G5 /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /FD /c
# ADD CPP /nologo /vmb /vms /MDd /W4 /Gm /G5 /ZI /Od /I "..\.." /I "..\..\include\cssys\win32" /I "..\..\include" /I "..\..\libs" /I "..\..\support" /I "..\..\apps" /I "..\..\plugins" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_WINDOWS" /D "WIN32_VOLATILE" /D "__CRYSTAL_SPACE__" /D "CS_DEBUG" /D "CS_EXTENSIVE_MEMDEBUG" /FD %cflags% /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /i "..\..\include" /i "..\..\include\cssys\win32"
# ADD RSC /l 0x409 /d "_DEBUG" /i "..\..\include" /i "..\..\include\cssys\win32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"csdebug\bin\libs\%project%_d.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=echo File is a lib, Copy skipped.
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "%project% - Win32 Release"
# Name "%project% - Win32 Debug"
%groups%
# End Target
# End Project
