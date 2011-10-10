/* $Xorg: ddxLoad.c,v 1.3 2000/08/17 19:53:46 cpqbld Exp $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/
/* $XFree86: xc/programs/Xserver/xkb/ddxLoad.c,v 3.35 2003/10/02 13:30:12 eich Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef HAVE_XKB_CONFIG_H
#include <xkb-config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#define	NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/extensions/XKM.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include <X11/extensions/XKBsrv.h>
#include <X11/extensions/XI.h>
#include "xkb.h"

#if defined(CSRG_BASED) || defined(linux) || defined(__sgi) || defined(AIXV3) || defined(__osf__) || defined(__GNU__)
#include <paths.h>
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define	PATH_MAX MAXPATHLEN
#else
#define	PATH_MAX 1024
#endif
#endif

	/*
	 * If XKM_OUTPUT_DIR specifies a path without a leading slash, it is
	 * relative to the top-level XKB configuration directory.
	 * Making the server write to a subdirectory of that directory
	 * requires some work in the general case (install procedure
	 * has to create links to /var or somesuch on many machines),
	 * so we just compile into /usr/tmp for now.
	 */
#ifndef XKM_OUTPUT_DIR
#define	XKM_OUTPUT_DIR	"compiled/"
#endif

#define	PRE_ERROR_MSG "\"The XKEYBOARD keymap compiler (xkbcomp) reports:\""
#define	ERROR_PREFIX	"\"> \""
#define	POST_ERROR_MSG1 "\"Errors from xkbcomp are not fatal to the X server\""
#define	POST_ERROR_MSG2 "\"End of messages from xkbcomp\""

#if defined(__UNIXOS2__) || defined(WIN32)
#define PATHSEPARATOR "\\"
#else
#define PATHSEPARATOR "/"
#endif

#ifdef WIN32

#include <X11/Xwindows.h>
const char* 
Win32TempDir()
{
    static char buffer[PATH_MAX];
    if (GetTempPath(sizeof(buffer), buffer))
    {
        int len;
        buffer[sizeof(buffer)-1] = 0;
        len = strlen(buffer);
        if (len > 0)
            if (buffer[len-1] == '\\')
                buffer[len-1] = 0;
        return buffer;
    }
    if (getenv("TEMP") != NULL)
        return getenv("TEMP");
    else if (getenv("TMP") != NULL)
        return getenv("TEMP");
    else
        return "/tmp";
}

int 
Win32System(const char *cmdline)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwExitCode;
    char *cmd = xstrdup(cmdline);

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) 
    {
	LPVOID buffer;
	if (!FormatMessage( 
		    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		    FORMAT_MESSAGE_FROM_SYSTEM | 
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
		    GetLastError(),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		    (LPTSTR) &buffer,
		    0,
		    NULL ))
	{
	    ErrorF("Starting '%s' failed!\n", cmdline); 
	}
	else
	{
	    ErrorF("Starting '%s' failed: %s", cmdline, (char *)buffer); 
	    LocalFree(buffer);
	}

	xfree(cmd);
	return -1;
    }
    /* Wait until child process exits. */
    WaitForSingleObject( pi.hProcess, INFINITE );

    GetExitCodeProcess( pi.hProcess, &dwExitCode);
    
    /* Close process and thread handles. */
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    xfree(cmd);

    return dwExitCode;
}
#undef System
#define System(x) Win32System(x)
#endif

#ifdef MAKE_XKM_OUTPUT_DIR
/* Borrow trans_mkdir from Xtransutil.c to more safely make directories */
# undef X11_t
# define TRANS_SERVER
# define PRMSG(lvl,x,a,b,c) \
	if (lvl <= 1) { LogMessage(X_ERROR,x,a,b,c); } else ((void)0)
# include <X11/Xtrans/Xtransutil.c>
# ifndef XKM_OUTPUT_DIR_MODE
#  define XKM_OUTPUT_DIR_MODE 0755
# endif
#endif

#ifdef NXAGENT_SERVER

#define NX_XKB_BASE_DIRECTORY            "/usr/lib/X11/xkb"
#define NX_XKB_ALTERNATE_BASE_DIRECTORY  "/usr/share/X11/xkb"
#define NX_KEYMAP_DIR_FILE               "keymap.dir"
#define NX_ALT_XKBCOMP_PATH              "/usr/bin"

static char _NXXkbBasePath[PATH_MAX];
static char _NXXkbCompPath[PATH_MAX];

static int NXVerifyXkbBaseDirectory(const char *dirPath)
{
  int size;
  char *keymapDirFilePath;
  struct stat keymapDirFileStat;

  /*
   * If keymap.dir file
   * is not present into
   * Xkb Base Directory,
   * we suppose that the
   * path is not valid.
   */

  size = strlen(dirPath) + strlen("/") +
             strlen(NX_KEYMAP_DIR_FILE) + 1;

  if ((keymapDirFilePath = malloc((size + 1) * sizeof(char))) == NULL)
  {
    FatalError("NXVerifyXkbBaseDirectory: malloc failed.\n");
  }

  strcpy(keymapDirFilePath, dirPath);
  strcat(keymapDirFilePath, "/");
  strcat(keymapDirFilePath, NX_KEYMAP_DIR_FILE);

  #ifdef TEST
  fprintf(stderr, "NXVerifyXkbBaseDirectory: Looking for [%s] file.\n",
              keymapDirFilePath);
  #endif

  if (stat(keymapDirFilePath, &keymapDirFileStat) != 0)
  {

    #ifdef TEST
    fprintf(stderr, "NXVerifyXkbBaseDirectory: Can't find the keymap.dir file [%s].\n",
                keymapDirFilePath);
    #endif

    free(keymapDirFilePath);

    return 0;
  }

  #ifdef TEST
  fprintf(stderr, "NXVerifyXkbBaseDirectory: Xkb Base Directory [%s] is valid.\n",
              dirPath);
  #endif

  free(keymapDirFilePath);

  return 1;
}

/*
 * This function returns the directory
 * containing the configuration files.
 * This directory is referred by Xkb-
 * BaseDirectory variable (generally
 * it contains the hardcoded path at
 * compile time). If the directory
 * does not exist, the function will
 * try a set of well known directories.
 */

char *_NXGetXkbBasePath(const char *path)
{
  /*
   * Check the xkb base directory only once.
   */

  if (*_NXXkbBasePath != '\0')
  {
    return _NXXkbBasePath;
  }
 
  if (NXVerifyXkbBaseDirectory(XkbBaseDirectory) == 1)
  {
    if (strlen(XkbBaseDirectory) + 1 > PATH_MAX)
    {
      #ifdef TEST
      fprintf(stderr, "_NXGetXkbBasePath: WARNING! Maximum length of xkb base path exceeded.\n");
      #endif
 
      goto _NXGetXkbBasePathError;
    }

    strcpy(_NXXkbBasePath, XkbBaseDirectory);

    #ifdef TEST
    fprintf(stderr, "_NXGetXkbBasePath: Using NX xkb base directory path [%s].\n",
                _NXXkbBasePath);
    #endif

    return _NXXkbBasePath;
  }
 
  if (NXVerifyXkbBaseDirectory(NX_XKB_BASE_DIRECTORY) == 1)
  {
    if (strlen(NX_XKB_BASE_DIRECTORY) + 1 > PATH_MAX)
    {
      #ifdef TEST
      fprintf(stderr, "_NXGetXkbBasePath: WARNING! Maximum length of xkb base path exceeded.\n");
      #endif

      goto _NXGetXkbBasePathError;
    }

    strcpy(_NXXkbBasePath, NX_XKB_BASE_DIRECTORY);

    #ifdef TEST
    fprintf(stderr, "_NXGetXkbBasePath: Using NX xkb base directory path [%s].\n",
                _NXXkbBasePath);
    #endif

    return _NXXkbBasePath;
  }

  if (NXVerifyXkbBaseDirectory(NX_XKB_ALTERNATE_BASE_DIRECTORY) == 1)
  {
    if (strlen(NX_XKB_ALTERNATE_BASE_DIRECTORY) + 1 > PATH_MAX)
    {
      #ifdef TEST
      fprintf(stderr, "_NXGetXkbBasePath: WARNING! Maximum length of xkb base path exceeded.\n");
      #endif

      goto _NXGetXkbBasePathError;
    }

    strcpy(_NXXkbBasePath, NX_XKB_ALTERNATE_BASE_DIRECTORY);

    #ifdef TEST
    fprintf(stderr, "_NXGetXkbBasePath: Using NX xkb base directory path [%s].\n",
                _NXXkbBasePath);
    #endif

    return _NXXkbBasePath;
  }

_NXGetXkbBasePathError:

  if (strlen(path) + 1 > PATH_MAX)
  {
    #ifdef TEST
    fprintf(stderr, "_NXGetXkbBasePath: WARNING! Maximum length of xkb base path exceeded.\n");
    #endif
  }

  strcpy(_NXXkbBasePath, path);

  #ifdef TEST
  fprintf(stderr, "_NXGetXkbBasePath: Using default xkb base path [%s].\n",
              _NXXkbBasePath);
  #endif

  return _NXXkbBasePath;
}

static int NXVerifyXkbCompPath(char *path)
{
  char *xkbCompPath;
  int xkbCompPathSize;
  struct stat xkbCompPathStat;

  if (path == NULL)
  {
    return 0;
  }

  xkbCompPathSize = strlen(path) + strlen("/") +
                        strlen("xkbcomp") + 1;

  if ((xkbCompPath = malloc((xkbCompPathSize + 1) * sizeof(char))) == NULL)
  {
    FatalError("NXVerifyXkbCompPath: WARNING! malloc failed.\n");

    return 0;
  }

  strcpy(xkbCompPath, path);
  strcat(xkbCompPath, "/");
  strcat(xkbCompPath, "xkbcomp");

  if (stat(xkbCompPath, &xkbCompPathStat) != 0)
  {
    #ifdef NX_TRANS_TEST
    fprintf(stderr, "NXVerifyXkbCompPath: WARNING! Failed to stat xkbcomp path [%s].\n",
                xkbCompPath);
    #endif

    free(xkbCompPath);

    return 0;
  }

  free(xkbCompPath);

  return 1;
}

/*
 * This function returns the directory
 * containing the xkbcomp executable.
 * The function will first try to locate
 * the executable in the hardcoded path
 * (the same path as the "base" xkb one)
 * and, if the xkbcomp file couldn't be
 * found, the function will not include
 * an explicit path and will rely on the
 * PATH environment to list the directory.
 */

char *_NXGetXkbCompPath(const char *path)
{

  char * xkbCompPath;

  /*
   * Check the xkbcomp executable
   * directory only once.
   */

  if (*_NXXkbCompPath != '\0')
  {
    return _NXXkbCompPath;
  }

  xkbCompPath = _NXGetXkbBasePath(path);

  if (NXVerifyXkbCompPath(xkbCompPath) == 1)
  {
    if (strlen(xkbCompPath) + 1 > PATH_MAX)
    {
      #ifdef TEST
      fprintf(stderr, "_NXGetXkbCompPath: WARNING! Maximum length of xkbcomp path exceeded.\n");
      #endif

      goto _NXGetXkbCompPathError;
    }

    strcpy(_NXXkbCompPath, xkbCompPath);

    #ifdef TEST
    fprintf(stderr, "_NXGetXkbCompPath: Using xkbcomp path [%s].\n",
                _NXXkbCompPath);
    #endif

    return _NXXkbCompPath;
  }

  xkbCompPath = NX_ALT_XKBCOMP_PATH;

  if (NXVerifyXkbCompPath(xkbCompPath) == 1)
  {
    if (strlen(xkbCompPath) + 1 > PATH_MAX)
    {
      #ifdef TEST
      fprintf(stderr, "_NXGetXkbCompPath: WARNING! Maximum length of xkbcomp path exceeded.\n");
      #endif

      goto _NXGetXkbCompPathError;
    }

    strcpy(_NXXkbCompPath, xkbCompPath);

    #ifdef TEST
    fprintf(stderr, "_NXGetXkbCompPath: Using NX xkbcomp path [%s].\n",
                _NXXkbCompPath);
    #endif

    return _NXXkbCompPath;
  }

_NXGetXkbCompPathError:

  if (strlen(path) + 1 > PATH_MAX)
  {
    #ifdef TEST
    fprintf(stderr, "_NXGetXkbCompPath: WARNING! Maximum length of xkbcomp path exceeded.\n");
    #endif
  }

  strcpy(_NXXkbCompPath, path);

  #ifdef TEST
  fprintf(stderr, "_NXGetXkbCompPath: Using default xkbcomp path [%s].\n",
              _NXXkbCompPath);
  #endif

  return _NXXkbCompPath;
}

#endif

static void
OutputDirectory(
    char* outdir,
    size_t size)
{
#ifndef WIN32
    if (getuid() == 0 && (strlen(XKM_OUTPUT_DIR) < size)
#ifdef MAKE_XKM_OUTPUT_DIR    
	&& (trans_mkdir(XKM_OUTPUT_DIR, XKM_OUTPUT_DIR_MODE) == 0)
#endif
	)
    {
	/* if server running as root it *may* be able to write */
	/* FIXME: check whether directory is writable at all */
	(void) strcpy (outdir, XKM_OUTPUT_DIR);
    } else
#endif
#ifdef _PATH_VARTMP
    if ((strlen(_PATH_VARTMP) + 1) < size) 
    {
	(void) strcpy (outdir, _PATH_VARTMP);
	if (outdir[strlen(outdir) - 1] != '/')	/* Hi IBM, Digital */
	    (void) strcat (outdir, "/");
    } else
#endif
#ifdef WIN32
    if (strlen(Win32TempDir()) + 1 < size)
    {
	(void) strcpy(outdir, Win32TempDir());
	(void) strcat(outdir, "\\");
    } else 
#endif
    if (strlen("/tmp/") < size)
    {
	(void) strcpy (outdir, "/tmp/");
    }
}

Bool
XkbDDXCompileNamedKeymap(	XkbDescPtr		xkb,
				XkbComponentNamesPtr	names,
				char *			nameRtrn,
				int			nameRtrnLen)
{
char 	*cmd = NULL,file[PATH_MAX],xkm_output_dir[PATH_MAX],*map,*outFile;

    if (names->keymap==NULL)
	return False;
    strncpy(file,names->keymap,PATH_MAX); file[PATH_MAX-1]= '\0';
    if ((map= strrchr(file,'('))!=NULL) {
	char *tmp;
	if ((tmp= strrchr(map,')'))!=NULL) {
	    *map++= '\0';
	    *tmp= '\0';
	}
	else {
	    map= NULL;
	}
    }
    if ((outFile= strrchr(file,'/'))!=NULL)
	 outFile= _XkbDupString(&outFile[1]);
    else outFile= _XkbDupString(file);
    XkbEnsureSafeMapName(outFile);
    OutputDirectory(xkm_output_dir, sizeof(xkm_output_dir));

#ifdef NXAGENT_SERVER

    if (_NXGetXkbCompPath(XkbBaseDirectory) != NULL)
    {

#else

    if (XkbBaseDirectory!=NULL) {

#endif

#ifndef __UNIXOS2__

#ifdef NXAGENT_SERVER
        char *xkbbasedir = _NXGetXkbBasePath(XkbBaseDirectory);
        char *xkbbindir = _NXGetXkbCompPath(XkbBinDirectory);
#else
        char *xkbbasedir = XkbBaseDirectory;
        char *xkbbindir = XkbBinDirectory;
#endif

#else
        /* relocate the basedir and replace the slashes with backslashes */
#ifdef NXAGENT_SERVER
        char *xkbbasedir = (char*)__XOS2RedirRoot(_NXGetXkbBasePath(XkbBaseDirectory));
        char *xkbbindir = (char*)__XOS2RedirRoot(_NXGetXkbCompPath(XkbBinDirectory));
#else
        char *xkbbasedir = (char*)__XOS2RedirRoot(XkbBaseDirectory);
        char *xkbbindir = (char*)__XOS2RedirRoot(XkbBinDirectory);
#endif
        int i;

	for (i=0; i<strlen(xkbbasedir); i++) 
            if (xkbbasedir[i]=='/') xkbbasedir[i]='\\';
	for (i=0; i<strlen(xkbbindir); i++) 
            if (xkbbindir[i]=='/') xkbbindir[i]='\\';
#endif

	cmd = Xprintf("\"%s" PATHSEPARATOR "xkbcomp\" -w %d \"-R%s\" -xkm %s%s -em1 %s -emp %s -eml %s keymap/%s \"%s%s.xkm\"",
		xkbbindir,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
		xkbbasedir,(map?"-m ":""),(map?map:""),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,file,
		xkm_output_dir,outFile);
    }
    else {
	cmd = Xprintf("xkbcomp -w %d -xkm %s%s -em1 %s -emp %s -eml %s keymap/%s \"%s%s.xkm\"",
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
		(map?"-m ":""),(map?map:""),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,file,
		xkm_output_dir,outFile);
    }
#ifdef DEBUG
    if (xkbDebugFlags) {
	ErrorF("XkbDDXCompileNamedKeymap compiling keymap using:\n");
	ErrorF("    \"cmd\"\n");
    }
#endif
#ifdef DEBUG_CMD
    ErrorF("xkb executes: %s\n",cmd);
#endif
    if (System(cmd)==0) {
	if (nameRtrn) {
	    strncpy(nameRtrn,outFile,nameRtrnLen);
	    nameRtrn[nameRtrnLen-1]= '\0';
	}
	if (outFile!=NULL)
	    _XkbFree(outFile);
        if (cmd!=NULL)
            xfree(cmd);
	return True;
    } 
#ifdef DEBUG
    ErrorF("Error compiling keymap (%s)\n",names->keymap);
#endif
    if (outFile!=NULL)
	_XkbFree(outFile);
    if (cmd!=NULL)
        xfree(cmd);
    return False;
}

Bool    	
XkbDDXCompileKeymapByNames(	XkbDescPtr		xkb,
				XkbComponentNamesPtr	names,
				unsigned		want,
				unsigned		need,
				char *			nameRtrn,
				int			nameRtrnLen)
{
FILE *	out;
char	*buf = NULL, keymap[PATH_MAX],xkm_output_dir[PATH_MAX];

#ifdef WIN32
char tmpname[PATH_MAX];
#endif    
    if ((names->keymap==NULL)||(names->keymap[0]=='\0')) {
	sprintf(keymap,"server-%s",display);
    }
    else {
	if (strlen(names->keymap) > PATH_MAX - 1) {
	    ErrorF("name of keymap (%s) exceeds max length\n", names->keymap);
	    return False;
	}
	strcpy(keymap,names->keymap);
    }

    XkbEnsureSafeMapName(keymap);
    OutputDirectory(xkm_output_dir, sizeof(xkm_output_dir));
#ifdef WIN32
    strcpy(tmpname, Win32TempDir());
    strcat(tmpname, "\\xkb_XXXXXX");
    (void) mktemp(tmpname);
#endif

#ifdef NXAGENT_SERVER
    if (_NXGetXkbCompPath(XkbBaseDirectory)!=NULL) {
#else
    if (XkbBaseDirectory!=NULL) {
#endif

#ifndef WIN32
        char *xkmfile = "-";
#else
        /* WIN32 has no popen. The input must be stored in a file which is used as input
           for xkbcomp. xkbcomp does not read from stdin. */
        char *xkmfile = tmpname;
#endif
#ifndef __UNIXOS2__
#ifdef NXAGENT_SERVER
        char *xkbbasedir = _NXGetXkbBasePath(XkbBaseDirectory);
        char *xkbbindir = _NXGetXkbCompPath(XkbBinDirectory);
#else
        char *xkbbasedir = XkbBaseDirectory;
        char *xkbbindir = XkbBinDirectory;
#endif
#else
        int i;
#ifdef NXAGENT_SERVER
        char *xkbbasedir = (char*)__XOS2RedirRoot(_NXGetXkbBasePath(XkbBaseDirectory));
        char *xkbbindir = (char*)__XOS2RedirRoot(_NXGetXkbCompPath(XkbBinDirectory));
#else
        char *xkbbasedir = (char*)__XOS2RedirRoot(XkbBaseDirectory);
        char *xkbbindir = (char*)__XOS2RedirRoot(XkbBinDirectory);
#endif
	for (i=0; i<strlen(xkbbasedir); i++) 
            if (xkbbasedir[i]=='/') xkbbasedir[i]='\\';
	for (i=0; i<strlen(xkbbindir); i++) 
            if (xkbbindir[i]=='/') xkbbindir[i]='\\';
#endif
        
	buf = Xprintf(
	   "\"%s" PATHSEPARATOR "xkbcomp\" -w %d \"-R%s\" -xkm \"%s\" -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		xkbbindir,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
		xkbbasedir, xkmfile,
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,
		xkm_output_dir,keymap);
    }
    else {
#ifndef WIN32
        char *xkmfile = "-";
#else
        char *xkmfile = tmpname;
#endif
	buf = Xprintf(
		"xkbcomp -w %d -xkm \"%s\" -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
                xkmfile,
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,
		xkm_output_dir,keymap);
    }
    
    #ifdef TEST
    if (buf != NULL)
        fprintf(stderr, "XkbDDXCompileKeymapByNames: "
                    "Executing command [%s].\n", buf);
    else
        fprintf(stderr, "XkbDDXCompileKeymapByNames: "
                    "Callin Popen() with null command.\n");
    #endif

#ifndef WIN32
    out= Popen(buf,"w");
#else
    out= fopen(tmpname, "w");
#endif
    
    if (out!=NULL) {
#ifdef DEBUG
    if (xkbDebugFlags) {
       ErrorF("XkbDDXCompileKeymapByNames compiling keymap:\n");
       XkbWriteXKBKeymapForNames(stderr,names,NULL,xkb,want,need);
    }
#endif
	XkbWriteXKBKeymapForNames(out,names,NULL,xkb,want,need);
#ifndef WIN32
#ifdef __sun
        if (Pclose(out) != 0)
        {
            ErrorF("Warning: Spurious failure reported in Pclose() runnning 'xkbcomp'.\n");
        }
        if (1)
#else
	if (Pclose(out)==0)
#endif
#else
	if (fclose(out)==0 && System(buf) >= 0)
#endif
	{
#ifdef DEBUG_CMD
	    ErrorF("xkb executes: %s\n",buf);
	    ErrorF("xkbcomp input:\n");
	    XkbWriteXKBKeymapForNames(stderr,names,NULL,xkb,want,need);
	    ErrorF("end xkbcomp input\n");
#endif
	    if (nameRtrn) {
		strncpy(nameRtrn,keymap,nameRtrnLen);
		nameRtrn[nameRtrnLen-1]= '\0';
	    }
#if defined(Lynx) && defined(__i386__) && defined(NEED_POPEN_WORKAROUND)
	/* somehow popen/pclose is broken on LynxOS AT 2.3.0/2.4.0!
	 * the problem usually shows up with XF86Setup
	 * this hack waits at max 5 seconds after pclose() returns
	 * for the output of the xkbcomp output file.
	 * I didn't manage to get a patch in time for the 3.2 release
	 */
            {
		int i;
		char name[PATH_MAX];
#ifdef NXAGENT_SERVER
                if (_NXGetXkbCompPath(XkbBaseDirectory)!=NULL)
		    sprintf(name,"%s/%s%s.xkm", _NXGetXkbCompPath(XkbBaseDirectory)
			,xkm_output_dir, keymap);
#else
                if (XkbBaseDirectory!=NULL)
		    sprintf(name,"%s/%s%s.xkm", XkbBaseDirectory
			,xkm_output_dir, keymap);
#endif
		else
                    sprintf(name,"%s%s.xkm", xkm_output_dir, keymap);
		for (i = 0; i < 10; i++) {
	            if (access(name, 0) == 0) break;
		    usleep(500000);
		}
#ifdef DEBUG
		if (i) ErrorF(">>>> Waited %d times for %s\n", i, name);
#endif
	    }
#endif
            if (buf != NULL)
                xfree (buf);
	    return True;
	}
#ifdef DEBUG
	else
	    ErrorF("Error compiling keymap (%s)\n",keymap);
#endif
#ifdef WIN32
        /* remove the temporary file */
        unlink(tmpname);
#endif
    }
#ifdef DEBUG
    else {
#ifndef WIN32
	ErrorF("Could not invoke keymap compiler\n");
#else
	ErrorF("Could not open file %s\n", tmpname);
#endif
    }
#endif
    if (nameRtrn)
	nameRtrn[0]= '\0';
    if (buf != NULL)
        xfree (buf);
    return False;
}

FILE *
XkbDDXOpenConfigFile(char *mapName,char *fileNameRtrn,int fileNameRtrnLen)
{
char	buf[PATH_MAX],xkm_output_dir[PATH_MAX];
FILE *	file;

    buf[0]= '\0';
    if (mapName!=NULL) {
	OutputDirectory(xkm_output_dir, sizeof(xkm_output_dir));
	if ((XkbBaseDirectory!=NULL)&&(xkm_output_dir[0]!='/')
#ifdef WIN32
                &&(!isalpha(xkm_output_dir[0]) || xkm_output_dir[1]!=':')
#endif
                ) {
	    if (strlen(XkbBaseDirectory)+strlen(xkm_output_dir)
		     +strlen(mapName)+6 <= PATH_MAX)
	    {
	        sprintf(buf,"%s/%s%s.xkm",XkbBaseDirectory,
					xkm_output_dir,mapName);
	    }
	}
	else if (strlen(xkm_output_dir)+strlen(mapName)+5 <= PATH_MAX)
	    sprintf(buf,"%s%s.xkm",xkm_output_dir,mapName);
	if (buf[0] != '\0')
	    file= fopen(buf,"rb");
	else file= NULL;
    }
    else file= NULL;
    if ((fileNameRtrn!=NULL)&&(fileNameRtrnLen>0)) {
	strncpy(fileNameRtrn,buf,fileNameRtrnLen);
	buf[fileNameRtrnLen-1]= '\0';
    }
    return file;
}

unsigned
XkbDDXLoadKeymapByNames(	DeviceIntPtr		keybd,
				XkbComponentNamesPtr	names,
				unsigned		want,
				unsigned		need,
				XkbFileInfo *		finfoRtrn,
				char *			nameRtrn,
				int 			nameRtrnLen)
{
XkbDescPtr	xkb;
FILE	*	file;
char		fileName[PATH_MAX];
unsigned	missing;

    bzero(finfoRtrn,sizeof(XkbFileInfo));
    if ((keybd==NULL)||(keybd->key==NULL)||(keybd->key->xkbInfo==NULL))
	 xkb= NULL;
    else xkb= keybd->key->xkbInfo->desc;
    if ((names->keycodes==NULL)&&(names->types==NULL)&&
	(names->compat==NULL)&&(names->symbols==NULL)&&
	(names->geometry==NULL)) {
	if (names->keymap==NULL) {
	    bzero(finfoRtrn,sizeof(XkbFileInfo));
	    if (xkb && XkbDetermineFileType(finfoRtrn,XkbXKMFile,NULL) &&
	   				((finfoRtrn->defined&need)==need) ) {
		finfoRtrn->xkb= xkb;
		nameRtrn[0]= '\0';
		return finfoRtrn->defined;
	    }
	    return 0;
	}
	else if (!XkbDDXCompileNamedKeymap(xkb,names,nameRtrn,nameRtrnLen)) {
#ifdef NOISY
	    ErrorF("Couldn't compile keymap file\n");
#endif
	    return 0;
	}
    }
    else if (!XkbDDXCompileKeymapByNames(xkb,names,want,need,
						nameRtrn,nameRtrnLen)){
#ifdef NOISY
	ErrorF("Couldn't compile keymap file\n");
#endif
	return 0;
    }
    file= XkbDDXOpenConfigFile(nameRtrn,fileName,PATH_MAX);
    if (file==NULL) {
	LogMessage(X_ERROR, "Couldn't open compiled keymap file %s\n",fileName);
	return 0;
    }
    missing= XkmReadFile(file,need,want,finfoRtrn);
    if (finfoRtrn->xkb==NULL) {
	LogMessage(X_ERROR, "Error loading keymap %s\n",fileName);
	fclose(file);
	(void) unlink (fileName);
	return 0;
    }
#ifdef DEBUG
    else if (xkbDebugFlags) {
	ErrorF("Loaded %s, defined=0x%x\n",fileName,finfoRtrn->defined);
    }
#endif
    fclose(file);
    (void) unlink (fileName);
    return (need|want)&(~missing);
}

Bool
XkbDDXNamesFromRules(	DeviceIntPtr		keybd,
			char *			rules_name,
			XkbRF_VarDefsPtr	defs,
			XkbComponentNamesPtr	names)
{
char 		buf[PATH_MAX];
FILE *		file;
Bool		complete;
XkbRF_RulesPtr	rules;

    if (!rules_name)
	return False;
    if (XkbBaseDirectory==NULL) {
	if (strlen(rules_name)+7 > PATH_MAX)
	    return False;
	sprintf(buf,"rules/%s",rules_name);
    }
    else {
	if (strlen(XkbBaseDirectory)+strlen(rules_name)+8 > PATH_MAX)
	    return False;
        sprintf(buf,"%s/rules/%s",XkbBaseDirectory,rules_name);
    }
    if ((file= fopen(buf,"r"))==NULL)
	return False;
    if ((rules= XkbRF_Create(0,0))==NULL) {
	fclose(file);
	return False;
    }
    if (!XkbRF_LoadRules(file,rules)) {
	fclose(file);
	XkbRF_Free(rules,True);
	return False;
    }
    bzero((char *)names,sizeof(XkbComponentNamesRec));
    complete= XkbRF_GetComponents(rules,defs,names);
    fclose(file);
    XkbRF_Free(rules,True);
    return complete;
}


