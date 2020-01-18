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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef HAVE_XKB_CONFIG_H
#include <xkb-config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <ctype.h>

/* stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <nx-X11/X.h>
#include <nx-X11/Xos.h>
#include <nx-X11/Xproto.h>
#include <nx-X11/keysym.h>
#include <nx-X11/extensions/XKM.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include <xkbsrv.h>
#include <nx-X11/extensions/XI.h>
#include "xkb.h"

#if defined(CSRG_BASED) || defined(linux) || defined(__GNU__)
#include <paths.h>
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

#if defined(WIN32)
#define PATHSEPARATOR "\\"
#else
#define PATHSEPARATOR "/"
#endif

#ifdef WIN32

#include <nx-X11/Xwindows.h>
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

	free(cmd);
	return -1;
    }
    /* Wait until child process exits. */
    WaitForSingleObject( pi.hProcess, INFINITE );

    GetExitCodeProcess( pi.hProcess, &dwExitCode);
    
    /* Close process and thread handles. */
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    free(cmd);

    return dwExitCode;
}
#undef System
#define System(x) Win32System(x)
#endif

static void
OutputDirectory(
    char* outdir,
    size_t size)
{
#ifndef WIN32
    if (getuid() == 0 && (strlen(XKM_OUTPUT_DIR) < size))
    {
	/* if server running as root it *may* be able to write */
	/* FIXME: check whether directory is writable at all */
	(void) strcpy (outdir, XKM_OUTPUT_DIR);
    } else
#else
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

static Bool
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
	 outFile= Xstrdup(&outFile[1]);
    else outFile= Xstrdup(file);
    XkbEnsureSafeMapName(outFile);
    OutputDirectory(xkm_output_dir, sizeof(xkm_output_dir));

    if (XkbBaseDirectory!=NULL) {
        char *xkbbasedir = XkbBaseDirectory;
        char *xkbbindir = XkbBinDirectory;

	if (asprintf(&cmd,"\"%s" PATHSEPARATOR "xkbcomp\" -w %d \"-R%s\" -xkm %s%s -em1 %s -emp %s -eml %s keymap/%s \"%s%s.xkm\"",
		xkbbindir,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
		xkbbasedir,(map?"-m ":""),(map?map:""),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,file,
		xkm_output_dir,outFile) == -1)
            cmd = NULL;
    }
    else {
	if (asprintf(&cmd, "xkbcomp -w %d -xkm %s%s -em1 %s -emp %s -eml %s keymap/%s \"%s%s.xkm\"",
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
		(map?"-m ":""),(map?map:""),
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,file,
		xkm_output_dir,outFile) == -1)
            cmd = NULL;
    }
    if (xkbDebugFlags) {
	DebugF("XkbDDXCompileNamedKeymap compiling keymap using:\n");
	DebugF("    \"cmd\"\n");
    }
    if (System(cmd)==0) {
	if (nameRtrn) {
	    strncpy(nameRtrn,outFile,nameRtrnLen);
	    nameRtrn[nameRtrnLen-1]= '\0';
	}
	if (outFile!=NULL)
	    _XkbFree(outFile);
        if (cmd!=NULL)
            free(cmd);
	return True;
    } 
    DebugF("Error compiling keymap (%s)\n",names->keymap);
    if (outFile!=NULL)
	_XkbFree(outFile);
    if (cmd!=NULL)
        free(cmd);
    return False;
}

static Bool
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

    if (XkbBaseDirectory!=NULL) {
#ifndef WIN32
        char *xkmfile = "-";
#else
        /* WIN32 has no popen. The input must be stored in a file which is used as input
           for xkbcomp. xkbcomp does not read from stdin. */
        char *xkmfile = tmpname;
#endif
        char *xkbbasedir = XkbBaseDirectory;
        char *xkbbindir = XkbBinDirectory;
        
	if (asprintf(&buf,
		"\"%s" PATHSEPARATOR "xkbcomp\" -w %d \"-R%s\" -xkm \"%s\" -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		xkbbindir,
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
		xkbbasedir, xkmfile,
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,
		xkm_output_dir,keymap) == -1)
            buf = NULL;
    }
    else {
#ifndef WIN32
        char *xkmfile = "-";
#else
        char *xkmfile = tmpname;
#endif
	if (asprintf(&buf,
		"xkbcomp -w %d -xkm \"%s\" -em1 %s -emp %s -eml %s \"%s%s.xkm\"",
		((xkbDebugFlags<2)?1:((xkbDebugFlags>10)?10:(int)xkbDebugFlags)),
                xkmfile,
		PRE_ERROR_MSG,ERROR_PREFIX,POST_ERROR_MSG1,
		xkm_output_dir,keymap) == -1)
            buf = NULL;
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
            ErrorF("Warning: Spurious failure reported in Pclose() running 'xkbcomp'.\n");
        }
        if (1)
#else
	if (Pclose(out)==0)
#endif
#else
	if (fclose(out)==0 && System(buf) >= 0)
#endif
	{
	    if (xkbDebugFlags)
	        DebugF("xkb executes: %s\n",buf);
	    if (nameRtrn) {
		strncpy(nameRtrn,keymap,nameRtrnLen);
		nameRtrn[nameRtrnLen-1]= '\0';
	    }
            if (buf != NULL)
                free (buf);
	    return True;
	}
	else
	    LogMessage(X_ERROR, "Error compiling keymap (%s)\n", keymap);
#ifdef WIN32
        /* remove the temporary file */
        unlink(tmpname);
#endif
    }
    else {
#ifndef WIN32
	LogMessage(X_ERROR, "XKB: Could not invoke xkbcomp\n");
#else
	LogMessage(X_ERROR, "Could not open file %s\n", tmpname);
#endif
    }
    if (nameRtrn)
	nameRtrn[0]= '\0';
    if (buf != NULL)
        free (buf);
    return False;
}

static FILE *
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
	     if (snprintf(buf, PATH_MAX, "%s/%s%s.xkm", XkbBaseDirectory,
                           xkm_output_dir, mapName) >= PATH_MAX)
	          buf[0] = '\0';
	}
	else
	{
	      if (snprintf(buf, PATH_MAX, "%s%s.xkm", xkm_output_dir, mapName)
		  >= PATH_MAX)
                  buf[0] = '\0';
	}
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
            LogMessage(X_ERROR, "Couldn't compile keymap file %s\n",
                       names->keymap);
	    return 0;
	}
    }
    else if (!XkbDDXCompileKeymapByNames(xkb,names,want,need,
                                         nameRtrn,nameRtrnLen)){
	LogMessage(X_ERROR, "XKB: Couldn't compile keymap\n");
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
    else {
	DebugF("XKB: Loaded %s, defined=0x%x\n",fileName,finfoRtrn->defined);
    }
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

    if (strlen(XkbBaseDirectory) + strlen(rules_name) + 8 > PATH_MAX) {
        LogMessage(X_ERROR, "XKB: Rules name is too long\n");
        return False;
    }
    sprintf(buf,"%s/rules/%s", XkbBaseDirectory, rules_name);

    file = fopen(buf, "r");
    if (!file) {
        LogMessage(X_ERROR, "XKB: Couldn't open rules file %s\n", buf);
	return False;
    }

    rules = XkbRF_Create(0, 0);
    if (!rules) {
        LogMessage(X_ERROR, "XKB: Couldn't create rules struct\n");
	fclose(file);
	return False;
    }

    if (!XkbRF_LoadRules(file, rules)) {
        LogMessage(X_ERROR, "XKB: Couldn't parse rules file %s\n", rules_name);
	fclose(file);
	XkbRF_Free(rules,True);
	return False;
    }

    memset(names, 0, sizeof(*names));
    complete = XkbRF_GetComponents(rules,defs,names);
    fclose(file);
    XkbRF_Free(rules, True);

    if (!complete)
        LogMessage(X_ERROR, "XKB: Rules returned no components\n");

    return complete;
}
