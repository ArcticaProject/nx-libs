# Microsoft Developer Studio Project File - Name="gdi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=gdi - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gdi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gdi.mak" CFG="gdi - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gdi - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gdi - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gdi - Win32 Release x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gdi - Win32 Debug x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gdi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../main" /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/main" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/swrast" /I "../../../../src/mesa/shader" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 mesa.lib winmm.lib msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib /out:"Release/OPENGL32.DLL" /libpath:"../mesa/Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Release\OPENGL32.LIB ..\..\..\..\lib	copy Release\OPENGL32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Release\OPENGL32.DLL ..\..\..\..\progs\demos
# End Special Build Tool

!ELSEIF  "$(CFG)" == "gdi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/main" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/swrast" /I "../../../../src/mesa/shader" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mesa.lib winmm.lib msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"Debug/OPENGL32.DLL" /pdbtype:sept /libpath:"../mesa/Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Debug\OPENGL32.LIB ..\..\..\..\lib	copy Debug\OPENGL32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Debug\OPENGL32.DLL ..\..\..\..\progs\demos
# End Special Build Tool

!ELSEIF  "$(CFG)" == "gdi - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "gdi___Win32_Release_x86"
# PROP BASE Intermediate_Dir "gdi___Win32_Release_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "../../main" /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/main" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/swrast" /I "../../../../src/mesa/shader" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../main" /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/main" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/swrast" /I "../../../../src/mesa/shader" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mesa.lib winmm.lib msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib /out:"Release/OPENGL32.DLL" /libpath:"../mesa/Release"
# ADD LINK32 mesa.lib winmm.lib msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib /out:"Release_x86/OPENGL32.DLL" /libpath:"../mesa/Release_x86"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Release_x86\OPENGL32.LIB ..\..\..\..\lib	copy Release_x86\OPENGL32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Release_x86\OPENGL32.DLL ..\..\..\..\progs\demos	copy Release_x86\OPENGL32.DLL "C:\Documents and Settings\mjk\Pulpit\pen\noise-demo"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "gdi - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gdi___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "gdi___Win32_Debug_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/main" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/swrast" /I "../../../../src/mesa/shader" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /FR /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/main" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/swrast" /I "../../../../src/mesa/shader" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GDI_EXPORTS" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mesa.lib winmm.lib msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"Debug/OPENGL32.DLL" /pdbtype:sept /libpath:"../mesa/Debug"
# ADD LINK32 mesa.lib winmm.lib msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"Debug_x86/OPENGL32.DLL" /pdbtype:sept /libpath:"../mesa/Debug_x86"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Debug_x86\OPENGL32.LIB ..\..\..\..\lib	copy Debug_x86\OPENGL32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Debug_x86\OPENGL32.DLL ..\..\..\..\progs\demos	copy Debug_x86\OPENGL32.DLL "C:\Documents and Settings\mjk\Pulpit\pen\noise-demo"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "gdi - Win32 Release"
# Name "gdi - Win32 Debug"
# Name "gdi - Win32 Release x86"
# Name "gdi - Win32 Debug x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\common\driverfuncs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\windows\gdi\mesa.def
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\windows\gdi\wgl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\windows\gdi\wmesa.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\windows\gdi\colors.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\common\driverfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\drivers\windows\gdi\wmesadef.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
