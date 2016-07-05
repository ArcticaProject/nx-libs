/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
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
