/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
#include "Utils.h"

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

/*
 * Clients log file name.
 */

char *nxagentClientsLogName = NULL;

/*
 * User's home.
 */

static char *nxagentHomeDir = NULL;

/*
 * NX root directory.
 */

static char *nxagentRootDir = NULL;

/*
 * Session log Directory.
 */

static char *nxagentSessionDir = NULL;

void nxagentGetClientsPath(void);

static int nxagentPrintError(Display *dpy, XErrorEvent *event, FILE *fp);

/* declare an error handler that does not exit when an error
 * event is caught.
 */

int nxagentErrorHandler(Display *dpy, XErrorEvent *event)
{
  if (nxagentVerbose)
  {
    nxagentPrintError(dpy, event, stderr);
  }

  return 0;
}

/* copied from XlibInt.c:_XprintDefaultError
 * We cannot use the whole function because it requires XlibInt
 * internals. And we cannot call _XPrintDefaultError because it
 * is not exported.
 */
static int nxagentPrintError(dpy, event, fp)
    Display *dpy;
    XErrorEvent *event;
    FILE *fp;
{
    char buffer[BUFSIZ];
    char mesg[BUFSIZ];
    char number[32];
    const char *mtype = "XlibMessage";
#ifndef NXAGENT_SERVER
    register _XExtension *ext = (_XExtension *)NULL;
    _XExtension *bext = (_XExtension *)NULL;
#endif
    XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
    XGetErrorDatabaseText(dpy, mtype, "XError", "X Error", mesg, BUFSIZ);
    (void) fprintf(fp, "%s:  %s\n  ", mesg, buffer);
    XGetErrorDatabaseText(dpy, mtype, "MajorCode", "Request Major code %d",
        mesg, BUFSIZ);
    (void) fprintf(fp, mesg, event->request_code);
    if (event->request_code < 128) {
        snprintf(number, sizeof(number), "%d", event->request_code);
        XGetErrorDatabaseText(dpy, "XRequest", number, "", buffer, BUFSIZ);
    } else {
#ifndef NXAGENT_SERVER
        for (ext = dpy->ext_procs;
             ext && (ext->codes.major_opcode != event->request_code);
             ext = ext->next)
          ;
        if (ext) {
            strncpy(buffer, ext->name, BUFSIZ);
            buffer[BUFSIZ - 1] = '\0';
        } else
#endif
            buffer[0] = '\0';
    }
    (void) fprintf(fp, " (%s)\n", buffer);
    if (event->request_code >= 128) {
        XGetErrorDatabaseText(dpy, mtype, "MinorCode", "Request Minor code %d",
                              mesg, BUFSIZ);
        fputs("  ", fp);
        (void) fprintf(fp, mesg, event->minor_code);
#ifndef NXAGENT_SERVER
        if (ext) {
            snprintf(mesg, sizeof(mesg), "%s.%d", ext->name, event->minor_code);
            XGetErrorDatabaseText(dpy, "XRequest", mesg, "", buffer, BUFSIZ);
            (void) fprintf(fp, " (%s)", buffer);
        }
#endif
        fputs("\n", fp);
    }
    if (event->error_code >= 128) {
        /* kludge, try to find the extension that caused it */
        buffer[0] = '\0';
#ifndef NXAGENT_SERVER
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
            snprintf(buffer, sizeof(buffer), "%s.%d", bext->name,
                    event->error_code - bext->codes.first_error);
        else
#endif
            strcpy(buffer, "Value");
        XGetErrorDatabaseText(dpy, mtype, buffer, "", mesg, BUFSIZ);
        if (mesg[0]) {
            fputs("  ", fp);
            (void) fprintf(fp, mesg, event->resourceid);
            fputs("\n", fp);
        }
        /* let extensions try to print the values */
#ifndef NXAGENT_SERVER
        for (ext = dpy->ext_procs; ext; ext = ext->next) {
            if (ext->error_values)
                (*ext->error_values)(dpy, event, fp);
        }
#endif
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
#ifndef NXAGENT_SERVER
    XGetErrorDatabaseText(dpy, mtype, "CurrentSerial", "Current Serial #%d",
                          mesg, BUFSIZ);
    fputs("\n  ", fp);
    (void) fprintf(fp, mesg, (unsigned long long)(X_DPY_GET_REQUEST(dpy)));
#endif
    fputs("\n", fp);
    if (event->error_code == BadImplementation) return 0;
    return 1;
}

int nxagentExitHandler(const char *message)
{
  FatalError("%s", message);

  return 0;
}

void nxagentOpenClientsLogFile(void)
{
  if (!nxagentClientsLogName)
  {
    nxagentGetClientsPath();
  }

  if (nxagentClientsLogName && *nxagentClientsLogName != '\0')
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
      nxagentStderrBackup = dup(STDERR_FILENO);
    }

    if (nxagentStderrBackup != -1)
    {
      nxagentStderrDup = dup2(nxagentClientsLog, STDERR_FILENO);

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
    nxagentStderrDup = dup2(nxagentStderrBackup, STDERR_FILENO);

    if (nxagentStderrDup == -1)
    {
      fprintf(stderr, "Warning: Failed to restore stderr. Error is %d '%s'.\n",
                  errno, strerror(errno));
    }
  }

  nxagentCloseClientsLogFile();
}

/*
 * returns a pointer to the static nxagentHomeDir. The caller must not free
 * this pointer!
 */
char *nxagentGetHomePath(void)
{
  if (!nxagentHomeDir)
  {
    /*
     * Check the NX_HOME environment.
     */

    char *homeEnv = getenv("NX_HOME");

    if (!homeEnv || *homeEnv == '\0')
    {
      #ifdef TEST
      fprintf(stderr, "%s: No environment for NX_HOME.\n", __func__);
      #endif

      homeEnv = getenv("HOME");

      if (!homeEnv || *homeEnv == '\0')
      {
        #ifdef PANIC
        fprintf(stderr, "%s: PANIC! No environment for HOME.\n", __func__);
        #endif

        return NULL;
      }
    }

    /* FIXME: this is currently never freed as it is thought to last
       over the complete runtime. We should add a free call at shutdown
       eventually... */
    nxagentHomeDir = strdup(homeEnv);
    if (!nxagentHomeDir)
    {
      #ifdef PANIC
      fprintf(stderr, "%s: PANIC! Can't allocate memory for the home path.\n", __func__);
      #endif
      return NULL;
    }

    #ifdef TEST
    fprintf(stderr, "%s: Assuming NX user's home directory '%s'.\n", __func__, nxagentHomeDir);
    #endif
  }

  return nxagentHomeDir;
}

/*
 * returns a pointer to the static nxagentRootDir. The caller must not free
 * this pointer!
 */
char *nxagentGetRootPath(void)
{
  if (!nxagentRootDir)
  {
    /*
     * Check the NX_ROOT environment.
     */

    char *rootEnv = getenv("NX_ROOT");

    if (!rootEnv || *rootEnv == '\0')
    {
      #ifdef TEST
      fprintf(stderr, "%s: WARNING! No environment for NX_ROOT.\n", __func__);
      #endif

      /*
       * We will determine the root NX directory based on the NX_HOME
       * or HOME directory settings.
       */

      char *homeEnv = nxagentGetHomePath();

      if (!homeEnv)
      {
        return NULL;
      }

      /* FIXME: this is currently never freed as it is thought to last
         over the complete runtime. We should add a free call at shutdown
         eventually... */
      int len = asprintf(&nxagentRootDir, "%s/.nx", homeEnv);
      if (len == -1)
      {
        #ifdef PANIC
        fprintf(stderr, "%s: could not build NX Root Dir string\n", __func__);
        #endif

        return NULL;
      }

      #ifdef TEST
      fprintf(stderr, "%s: Assuming NX root directory in '%s'.\n", __func__, homeEnv);
      #endif

      /*
       * Create the NX root directory.
       */

      struct stat dirStat;
      if ((stat(nxagentRootDir, &dirStat) == -1) && (errno == ENOENT))
      {
        if (mkdir(nxagentRootDir, 0777) < 0 && (errno != EEXIST))
        {
          #ifdef PANIC
          fprintf(stderr, "%s: PANIC! Can't create directory '%s'. Error is %d '%s'.\n", __func__,
                      nxagentRootDir, errno, strerror(errno));
          #endif

          return NULL;
        }
      }
    }
    else
    {
      /* FIXME: this is currently never freed as it is thought to last
         over the complete runtime. We should add a free call
         eventually... */
      nxagentRootDir = strdup(rootEnv);
    }

    #ifdef TEST
    fprintf(stderr, "%s: Assuming NX root directory '%s'.\n", __func__,
                nxagentRootDir);
    #endif

  }

  return nxagentRootDir;
}

/*
 * returns a pointer to the static nxagentSessionDir. The caller must not free
 * this pointer!
 */
char *nxagentGetSessionPath(void)
{
  if (!nxagentSessionDir)
  {
    /*
     * If nxagentSessionId does not exist we assume that the
     * sessionPath cannot be realized and do not use the clients log
     * file.
     */

    if (*nxagentSessionId == '\0')
    {
      #ifdef TEST
      fprintf(stderr, "%s: Session id does not exist. Assuming session path NULL.\n", __func__);
      #endif

      return NULL;
    }

    char *rootPath = nxagentGetRootPath();

    if (!rootPath)
    {
      return NULL;
    }

    /* FIXME: this is currently only freed if the dir cannot be created
       and will last over the runtime otherwise.  We should add a free call
       eventually... */
    int len = asprintf(&nxagentSessionDir, "%s/C-%s", rootPath, nxagentSessionId);

    if (len == -1)
    {
      #ifdef PANIC
      fprintf(stderr, "%s: PANIC!: Could not alloc sessiondir string'.\n", __func__);
      #endif

      return NULL;
    }

    struct stat dirStat;
    if ((stat(nxagentSessionDir, &dirStat) == -1) && (errno == ENOENT))
    {
      if (mkdir(nxagentSessionDir, 0777) < 0 && (errno != EEXIST))
      {
        #ifdef PANIC
        fprintf(stderr, "%s: PANIC! Can't create directory '%s'. Error is %d '%s'.\n", __func__,
                    nxagentSessionDir, errno, strerror(errno));
        #endif

        SAFE_free(nxagentSessionDir);
        return NULL;
      }
    }

    #ifdef TEST
    fprintf(stderr, "%s: NX session is '%s'.\n", __func__, nxagentSessionDir);
    #endif
  }

  return nxagentSessionDir;
}

void nxagentGetClientsPath(void)
{
  if (!nxagentClientsLogName)
  {
    char *sessionPath = nxagentGetSessionPath();

    if (!sessionPath)
    {
      return;
    }

    /* FIXME: this is currently never freed as it is thought to last
       over the complete runtime. We should add a free call at shutdown
       eventually... */
    int len = asprintf(&nxagentClientsLogName, "%s/clients", sessionPath);
    if (len == -1)
    {
      #ifdef PANIC
      fprintf(stderr, "%s: PANIC! Could not alloc NX clients Log File Path.\n", __func__);
      #endif
      return;
    }
  }

  return;
}

