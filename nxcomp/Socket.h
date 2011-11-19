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

#ifndef Socket_H
#define Socket_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#ifdef __sun
#include <stropts.h>
#include <sys/filio.h>
#endif

//
// Set socket options.
//

int SetReuseAddress(int fd);
int SetNonBlocking(int fd, int value);
int SetLingerTimeout(int fd, int timeout);
int SetSendBuffer(int fd, int size);
int SetReceiveBuffer(int fd, int size);
int SetNoDelay(int fd, int value);
int SetKeepAlive(int fd);
int SetLowDelay(int fd);
int SetCloseOnExec(int fd);

//
// Get kernel support level.
//

int GetKernelStep();

//
// Get socket info.
//

int GetBytesReadable(int fd);
int GetBytesWritable(int fd);
int GetBytesQueued(int fd);

//
// Inline version, providing direct access
// to the interface.
//

#include "Misc.h"

inline int GetBytesReadable(int fd, int *readable)
{
  long t;

  int result = ioctl(fd, FIONREAD, &t);

  #ifdef DEBUG
  *logofs << "Socket: Bytes readable from FD#"
          << fd << " are " << t << " with result "
          << result << ".\n" << logofs_flush;
  #endif

  *readable = (int) t;

  return result;
}

//
// Query Internet address.
//

int GetHostAddress(const char *name);

#endif /* Socket_H */
