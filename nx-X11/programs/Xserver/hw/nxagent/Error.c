/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "os.h"

#include "scrnintstr.h"
#include "Agent.h"

#include "Xlib.h"
#include "Xproto.h"

#include "Error.h"
#include "Args.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG


/*
 * Duplicated stderr file descriptor.
 */

static int nxagentStderrDup = -1;

/*
 * Saved stderr file descriptor.
 */

static int nxagentStderrBackup = -1;

/*
 * Clients log file descriptor.
 */

static int nxagentClientsLog = -1;

#define DEFAULT_STRING_LENGTH 256


/*
 * Clients log file name.
 */

char nxagentClientsLogName[DEFAULT_STRING_LENGTH] = { 0 };

/*
 * User's home.
 */

static char nxagentHomeDir[DEFAULT_STRING_LENGTH] = { 0 };

/*
 * NX root directory.
 */

static char nxagentRootDir[DEFAULT_STRING_LENGTH] = { 0 };

/*
 * Session log Directory.
 */

static char nxagentSessionDir[DEFAULT_STRING_LENGTH] = { 0 };

char *nxagentGetClientsPath(void);

static int nxagentPrintError(Display *dpy, XErrorEvent *event, FILE *fp);

/* declare an error handler that does not exit when an error 
 * event is catched.
 */

int nxagentErrorHandler(Display *dpy, XErrorEvent *event)
{
  if (nxagentVerbose == 1)
  {
    nxagentPrintError(dpy, event, stderr);
  }

  return 0;
}

/* copied from XlibInt.c */
/* extension stuff roughly commented out */
static int nxagentPrintError(dpy, event, fp)
    Display *dpy;
    XErrorEvent *event;
    FILE *fp;
{
    char buffer[BUFSIZ];
    char mesg[BUFSIZ];
    char number[32];
    char *mtype = "XlibMessage";
    /*
    register _XExtension *ext = (_XExtension *)NULL;
    _XExtension *bext = (_XExtension *)NULL;
    */
    XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
    XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
    (void) fprintf(fp, "%s:  %s\n  ", mesg, buffer);
    XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d",
	mesg, BUFSIZ);
    (void) fprintf(fp, mesg, event->request_code);
    if (event->request_code < 128) {
	sprintf(number, "%d", event->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", number, "", buffer, BUFSIZ);
    } else {
      /*	for (ext = dpy->ext_procs;
	     ext && (ext->codes.major_opcode != event->request_code);
	     ext = ext->next)
	  ;
	if (ext)
	    strcpy(buffer, ext->name);
	else
      */
	    buffer[0] = '\0';
    }
    (void) fprintf(fp, " (%s)\n", buffer);
    if (event->request_code >= 128) {
	XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code %d",
			      mesg, BUFSIZ);
	fputs("  ", fp);
	(void) fprintf(fp, mesg, event->minor_code);
	/*
	if (ext) {
	    sprintf(mesg, "%s.%d", ext->name, event->minor_code);
	    XGetErrorDatabaseText(dpy, "XRequest", mesg, "", buffer, BUFSIZ);
	    (void) fprintf(fp, " (%s)", buffer);
	}
	*/
	fputs("\n", fp);
    }
    if (event->error_code >= 128) {
	/* kludge, try to find the extension that caused it */
	buffer[0] = '\0';
	/*
	for (ext = dpy->ext_procs; ext; ext = ext->next) {
	    if (ext->error_string) 
		(*ext->error_string)(dpy, event->error_code, &ext->codes,
				     buffer, BUFSIZ);
	    if (buffer[0]) {
		bext = ext;
		break;
	    }
	    if (ext->codes.first_error &&
		ext->codes.first_error < (int)event->error_code &&
		(!bext || ext->codes.first_error > bext->codes.first_error))
		bext = ext;
	}    
	if (bext)
	    sprintf(buffer, "%s.%d", bext->name,
		    event->error_code - bext->codes.first_error);
	else
	*/
	    strcpy(buffer, "Value");
	XGetErrorDatabaseText(dpy, mtype, buffer, "", mesg, BUFSIZ);
	if (mesg[0]) {
	    fputs("  ", fp);
	    (void) fprintf(fp, mesg, event->resourceid);
	    fputs("\n", fp);
	}
	/* let extensions try to print the values */
	/*
	for (ext = dpy->ext_procs; ext; ext = ext->next) {
	    if (ext->error_values)
		(*ext->error_values)(dpy, event, fp);
	}
	*/
    } else if ((event->error_code == BadWindow) ||
	       (event->error_code == BadPixmap) ||
	       (event->error_code == BadCursor) ||
	       (event->error_code == BadFont) ||
	       (event->error_code == BadDrawable) ||
	       (event->error_code == BadColor) ||
	       (event->error_code == BadGC) ||
	       (event->error_code == BadIDChoice) ||
	       (event->error_code == BadValue) ||
	       (event->error_code == BadAtom)) {
	if (event->error_code == BadValue)
	    XGetErrorDatabaseText(dpy, mtype, "Value", "Value 0x%x",
				  mesg, BUFSIZ);
	else if (event->error_code == BadAtom)
	    XGetErrorDatabaseText(dpy, mtype, "AtomID", "AtomID 0x%x",
				  mesg, BUFSIZ);
	else
	    XGetErrorDatabaseText(dpy, mtype, "ResourceID", "ResourceID 0x%x",
				  mesg, BUFSIZ);
	fputs("  ", fp);
	(void) fprintf(fp, mesg, event->resourceid);
	fputs("\n", fp);
    }
    XGetErrorDatabaseText(dpy, mtype, "ErrorSerial", "Error Serial #%d", 
			  mesg, BUFSIZ);
    fputs("  ", fp);
    (void) fprintf(fp, mesg, event->serial);
    XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%d",
			  mesg, BUFSIZ);
    fputs("\n  ", fp);
    /*    (void) fprintf(fp, mesg, dpy->request); */
    fputs("\n", fp);
    if (event->error_code == BadImplementation) return 0;
    return 1;
}

int nxagentExitHandler(const char *message)
{
  FatalError(message);

  return 0;
}

void nxagentOpenClientsLogFile()
{
  char * clientsLogName;

  if (*nxagentClientsLogName == '\0')
  {
    clientsLogName = nxagentGetClientsPath();

    if (clientsLogName != NULL)
    {
      free(clientsLogName);
    }
  }

  if (nxagentClientsLogName != NULL && *nxagentClientsLogName !='\0')
  {
    nxagentClientsLog = open(nxagentClientsLogName, O_RDWR | O_CREAT | O_APPEND, 0600);

    if (nxagentClientsLog == -1)
    {
      fprintf(stderr, "Warning: Failed to open clients log. Error is %d '%s'.\n",
                  errno, strerror(errno));
    }
  }
  else
  {
    #ifdef TEST
    fprintf(stderr, "nxagentOpenClientsLogFile: Cannot open clients log file. The path does not exist.\n");
    #endif
  }
}

void nxagentCloseClientsLogFile(void)
{
  close(nxagentClientsLog);
}

void nxagentStartRedirectToClientsLog(void)
{
  nxagentOpenClientsLogFile();

  if (nxagentClientsLog != -1)
  {
    if (nxagentStderrBackup == -1)
    {
      nxagentStderrBackup = dup(2);
    }

    if (nxagentStderrBackup != -1)
    {
      nxagentStderrDup = dup2(nxagentClientsLog, 2);

      if (nxagentStderrDup == -1)
      {
        fprintf(stderr, "Warning: Failed to redirect stderr. Error is %d '%s'.\n",
                    errno, strerror(errno));
      }
    }
    else
    {
      fprintf(stderr, "Warning: Failed to backup stderr. Error is %d '%s'.\n",
                  errno, strerror(errno));
    }
  }
}

void nxagentEndRedirectToClientsLog(void)
{
  if (nxagentStderrBackup != -1)
  {
    nxagentStderrDup = dup2(nxagentStderrBackup, 2);

    if (nxagentStderrDup == -1)
    {
      fprintf(stderr, "Warning: Failed to restore stderr. Error is %d '%s'.\n",
                  errno, strerror(errno));
    }
  }

  nxagentCloseClientsLogFile();
}

char *nxagentGetHomePath(void)
{
  char *homeEnv;
  char *homePath;

  if (*nxagentHomeDir == '\0')
  {
    /*
     * Check the NX_HOME environment.
     */

    homeEnv = getenv("NX_HOME");

    if (homeEnv == NULL || *homeEnv == '\0')
    {
      #ifdef TEST
      fprintf(stderr, "nxagentGetHomePath: No environment for NX_HOME.\n");
      #endif

      homeEnv = getenv("HOME");

      if (homeEnv == NULL || *homeEnv == '\0')
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentGetHomePath: PANIC! No environment for HOME.\n");
        #endif

        return NULL;
      }
    }

    if (strlen(homeEnv) > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentGetHomePath: PANIC! Invalid value for the NX "
                  "home directory '%s'.\n", homeEnv);
      #endif
    }

    strncpy(nxagentHomeDir, homeEnv, DEFAULT_STRING_LENGTH - 1);

    #ifdef TEST
    fprintf(stderr, "nxagentGetHomePath: Assuming NX user's home directory '%s'.\n", nxagentHomeDir);
    #endif
  }

  homePath = (char*) malloc(strlen(nxagentHomeDir) + 1);

  if (homePath == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentGetHomePath: PANIC! Can't allocate memory for the home path.\n");
    #endif

    return NULL;
  }

  strcpy(homePath, nxagentHomeDir);

  return homePath;
}

char *nxagentGetRootPath(void)
{
  char *rootEnv;
  char *homeEnv;
  char *rootPath;

  struct stat dirStat;

  if (*nxagentRootDir == '\0')
  {
    /*
     * Check the NX_ROOT environment.
     */

    rootEnv = getenv("NX_ROOT");

    if (rootEnv == NULL || *rootEnv == '\0')
    {
      #ifdef TEST
      fprintf(stderr, "nxagentGetRootPath: WARNING! No environment for NX_ROOT.\n");
      #endif

      /*
       * We will determine the root NX directory
       * based on the NX_HOME or HOME directory
       * settings.
       */

      homeEnv = nxagentGetHomePath();

      if (homeEnv == NULL)
      {

        #ifdef PANIC
        #endif

        return NULL;
      }

      if (strlen(homeEnv) > DEFAULT_STRING_LENGTH -
              strlen("/.nx") - 1)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentGetRootPath: PANIC! Invalid value for the NX "
                    "home directory '%s'.\n", homeEnv);
        #endif

        free(homeEnv);

        return NULL;
      }

      #ifdef TEST
      fprintf(stderr, "nxagentGetRootPath: Assuming NX root directory in '%s'.\n", homeEnv);
      #endif

      strcpy(nxagentRootDir, homeEnv);
      strcat(nxagentRootDir, "/.nx");

      free(homeEnv);

      /*
       * Create the NX root directory.
       */

      if ((stat(nxagentRootDir, &dirStat) == -1) && (errno == ENOENT))
      {
        if (mkdir(nxagentRootDir, 0777) < 0 && (errno != EEXIST))
        {
          #ifdef PANIC
          fprintf(stderr, "nxagentGetRootPath: PANIC! Can't create directory '%s'. Error is %d '%s'.\n",
                      nxagentRootDir, errno, strerror(errno));
          #endif

          return NULL;
        }
      }
    }
    else
    {
      if (strlen(rootEnv) > DEFAULT_STRING_LENGTH - 1)
      {
        #ifdef PANIC
         fprintf(stderr, "nxagentGetRootPath: PANIC! Invalid value for the NX root directory '%s'.\n",
                     rootEnv);
        #endif

        return NULL;
      }

      strcpy(nxagentRootDir, rootEnv);
    }

    #ifdef TEST
    fprintf(stderr, "nxagentGetRootPath: Assuming NX root directory '%s'.\n",
                nxagentRootDir);
    #endif
        
  }

  rootPath = malloc(strlen(nxagentRootDir) + 1);

  if (rootPath == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentGetRootPath: Can't allocate memory for the root path.\n");
    #endif

    return NULL;
  }

  strcpy(rootPath, nxagentRootDir);

  return rootPath;
}

char *nxagentGetSessionPath(void)
{

  char *rootPath;
  char *sessionPath;

  struct stat dirStat;

  if (*nxagentSessionDir == '\0')
  {
    /*
     * If nxagentSessionId does not exist we
     * assume that the sessionPath cannot be
     * realized and do not use the clients
     * log file.
     */

    if (*nxagentSessionId == '\0')
    {
      #ifdef TEST
      fprintf(stderr, "nxagentGetSessionPath: Session id does not exist. Assuming session path NULL.\n");
      #endif

      return NULL;
    }

    rootPath = nxagentGetRootPath();

    if (rootPath == NULL)
    {
      return NULL;
    }

    strcpy(nxagentSessionDir, rootPath);

    free(rootPath);

    if (strlen(nxagentSessionDir) + strlen("/C-") + strlen(nxagentSessionId) > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentGetSessionPath: PANIC!: Invalid value for the NX session directory '%s'.\n",
                  nxagentSessionDir);
      #endif

      return NULL;
    }

    strcat(nxagentSessionDir, "/C-");

    strcat(nxagentSessionDir, nxagentSessionId);

    if ((stat(nxagentSessionDir, &dirStat) == -1) && (errno == ENOENT))
    {
      if (mkdir(nxagentSessionDir, 0777) < 0 && (errno != EEXIST))
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentGetSessionPath: PANIC! Can't create directory '%s'. Error is %d '%s'.\n",
                    nxagentSessionDir, errno, strerror(errno));
        #endif

        return NULL;
      }
    }

    #ifdef TEST
    fprintf(stderr, "nxagentGetSessionPath: NX session is '%s'.\n",
                nxagentSessionDir);
    #endif

  }

  sessionPath = malloc(strlen(nxagentSessionDir) + 1);

  if (sessionPath  == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentGetSessionPath:: PANIC! Can't allocate memory for the session path.\n");
    #endif

    return NULL;
  }


  strcpy(sessionPath, nxagentSessionDir);

  return sessionPath;
}

char *nxagentGetClientsPath()
{
  char *sessionPath;
  char *clientsPath;

  if (*nxagentClientsLogName == '\0')
  {
    sessionPath = nxagentGetSessionPath();

    if (sessionPath == NULL)
    {
      return NULL;
    }

    if (strlen(sessionPath) + strlen("/clients") > DEFAULT_STRING_LENGTH - 1)
    {
      #ifdef PANIC
      fprintf(stderr, "nxagentGetClientsPath: PANIC! Invalid value for the NX clients Log File Path '%s'.\n",
                  nxagentClientsLogName);
      #endif

      free(sessionPath);

      return NULL;
    }

    strcpy(nxagentClientsLogName, sessionPath);

    strcat(nxagentClientsLogName, "/clients");

    free(sessionPath);
  }

  clientsPath = malloc(strlen(nxagentClientsLogName) + 1);

  if (clientsPath == NULL)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentGetClientsPath: PANIC! Can't allocate memory for the clients Log File Path path.\n");
    #endif

    return NULL;
  }

  strcpy(clientsPath, nxagentClientsLogName);

  return clientsPath; 
}

