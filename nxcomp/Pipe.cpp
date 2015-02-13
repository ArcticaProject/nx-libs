/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <grp.h>

#include "Pipe.h"
#include "Misc.h"
#include "Fork.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

extern void RegisterChild(int child);

static int Psplit(const char *command, char *parameters[], int limit);

//
// These are slightly modified versions of popen(3) and pclose(3)
// that don't rely on a shell to be available on the system, so
// that they can also work on Windows. As an additional benefit,
// these functions give up all privileges before running the com-
// mand. Code is taken from the X distribution and, in turn, is
// based on libc from FreeBSD 2.2.
//

static struct pid
{
  struct pid *next;
  FILE *fp;
  int self;

} *pidlist;

//
// A very unsofisticated attempt to parse the command line and
// split each parameter in distinct strings. This is not going
// to work when dealing with parameters containing spaces, even
// if they are enclosed in quotes.
// 

int Psplit(const char *command, char *parameters[], int limit)
{
  char *line;
  char *value;

  int number;

  //
  // Preapare the list of parameters.
  //

  for (number = 0; number < limit; number++)
  {
    parameters[number] = NULL;
  }

  //
  // Copy the command to get rid of the
  // const qualifier.
  //

  line = new char[strlen(command) + 1];

  if (line == NULL)
  {
    goto PsplitError;
  }

  strcpy(line, command);

  number = 0;

  value = strtok(line, " ");

  while (value != NULL && number < limit)
  {
    #ifdef DEBUG
    *logofs << "Psplit: Got parameter '" << value
            << "'.\n" << logofs_flush;
    #endif

    parameters[number] = new char[strlen(value) + 1];

    if (parameters[number] == NULL)
    {
      goto PsplitError;
    }

    strcpy(parameters[number], value);

    number++;

    //
    // If this is the first parameter, then
    // copy it in the second position and
    // use it as the name of the command.
    //

    if (number == 1)
    {
      parameters[number] = new char[strlen(value) + 1];

      if (parameters[number] == NULL)
      {
        goto PsplitError;
      }

      strcpy(parameters[number], value);

      number++;
    }

    value = strtok(NULL, " ");
  }

  //
  // Needs at least to have the command itself and
  // the first argument, being again the name of
  // the command.
  //

  if (number < 2)
  {
    goto PsplitError;
  }

  return number;

PsplitError:

  #ifdef PANIC
  *logofs << "Psplit: PANIC! Can't split command line '"
          << command << "'.\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Can't split command line '"
       << command << "'.\n";

  delete [] line;

  return -1;
}

FILE *Popen(char * const parameters[], const char *type)
{
  FILE *iop;

  struct pid *cur;
  int pdes[2], pid;

  if (parameters == NULL || type == NULL)
  {
    return NULL;
  }

  if ((*type != 'r' && *type != 'w') || type[1])
  {
    return NULL;
  }

  if ((cur = (struct pid *) malloc(sizeof(struct pid))) == NULL)
  {
    return NULL;
  }

  if (pipe(pdes) < 0)
  {
    free(cur);

    return NULL;
  }

  //
  // Block all signals until command is exited.
  // We need to gather information about the
  // child in Pclose().
  //

  DisableSignals();

  switch (pid = Fork())
  {
    case -1:
    {
      //
      // Error.
      //

      #ifdef PANIC
      *logofs << "Popen: PANIC! Function fork failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Function fork failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";

      close(pdes[0]);
      close(pdes[1]);

      free(cur);

      return NULL;
    }
    case 0:
    {
      //
      // Child.
      //

      struct passwd *pwent = getpwuid(getuid());
      if (pwent) initgroups(pwent->pw_name,getgid());
      setgid(getgid());
      setuid(getuid());

      if (*type == 'r')
      {
        if (pdes[1] != 1)
        {
          //
          // Set up stdout.
          //

          dup2(pdes[1], 1);
          close(pdes[1]);
        }

        close(pdes[0]);
      }
      else
      {
        if (pdes[0] != 0)
        {
          //
          // Set up stdin.
          //

          dup2(pdes[0], 0);
          close(pdes[0]);
        }

        close(pdes[1]);
      }

      execvp(parameters[0], parameters + 1);

      exit(127);
    }
  }

  //
  // Parent. Save data about the child.
  //

  RegisterChild(pid);

  if (*type == 'r')
  {
    iop = fdopen(pdes[0], type);

    close(pdes[1]);
  }
  else
  {
    iop = fdopen(pdes[1], type);

    close(pdes[0]);
  }

  cur -> fp = iop;
  cur -> self = pid;
  cur -> next = pidlist;

  pidlist = cur;

  #ifdef TEST
  *logofs << "Popen: Executing ";

  for (int i = 0; i < 256 && parameters[i] != NULL; i++)
  {
    *logofs << "[" << parameters[i] << "]";
  }

  *logofs << " with descriptor " << fileno(iop)
          << ".\n" << logofs_flush;
  #endif

  return iop;
}

FILE *Popen(const char *command, const char *type)
{
  char *parameters[256];

  if (Psplit(command, parameters, 256) > 0)
  {
    FILE *file = Popen(parameters, type);

    for (int i = 0; i < 256; i++)
    {
      delete [] parameters[i];
    }

    return file;
  }
  else
  {
    #ifdef PANIC
    *logofs << "Popen: PANIC! Failed to parse command '"
            << command << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to parse command '"
         << command << "'.\n";

    return NULL;
  }
}

int Pclose(FILE *iop)
{
  struct pid *cur, *last;

  int pstat;
  int pid;

  #ifdef TEST
  *logofs << "Pclose: Closing command with output "
          << "on descriptor " << fileno(iop) << ".\n"
          << logofs_flush;
  #endif

  fclose((FILE *) iop);

  for (last = NULL, cur = pidlist; cur; last = cur, cur = cur -> next)
  {
    if (cur -> fp == iop)
    {
      break;
    }
  }

  if (cur == NULL)
  {
    #ifdef PANIC
    *logofs << "Pclose: PANIC! Failed to find the process "
            << "for descriptor " << fileno(iop) << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to find the process "
         << "for descriptor " << fileno(iop) << ".\n";

    return -1;
  }

  do
  {
    #ifdef TEST
    *logofs << "Pclose: Going to wait for process "
            << "with pid '" << cur -> self << "'.\n"
            << logofs_flush;
    #endif

    pid = waitpid(cur -> self, &pstat, 0);
  }
  while (pid == -1 && errno == EINTR);

  if (last == NULL)
  {
    pidlist = cur -> next;
  }
  else
  {
    last -> next = cur -> next;
  }

  free(cur);

  //
  // Child has finished and we called the
  // waitpid(). We can enable signals again.
  //

  EnableSignals();

  return (pid == -1 ? -1 : pstat);
}
