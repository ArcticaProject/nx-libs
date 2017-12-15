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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "ChannelEndPoint.h"

#include "NXalert.h"

#include "Misc.h"

ChannelEndPoint::ChannelEndPoint(const char *spec)
  : defaultTCPPort_(0), defaultTCPInterface_(0),
    defaultUnixPath_(NULL), spec_(NULL) {
  setSpec(spec);
}

ChannelEndPoint::~ChannelEndPoint()
{
  char *unixPath = NULL;

  if (getUnixPath(&unixPath))
  {
    struct stat st;
    lstat(unixPath, &st);
    if(S_ISSOCK(st.st_mode))
      unlink(unixPath);
  }
  SAFE_FREE(unixPath);
  SAFE_FREE(defaultUnixPath_);
  SAFE_FREE(spec_);
}

void
ChannelEndPoint::setSpec(const char *spec) {
  SAFE_FREE(spec_);

  if (spec && strlen(spec))
  {
    spec_ = strdup(spec);
    isUnix_ = getUnixPath();
    isTCP_ = getTCPHostAndPort();
  }
  else
  {
    spec_ = NULL;
    isUnix_ = false;
    isTCP_ = false;
  }
}

void
ChannelEndPoint::setSpec(long port) {
  if (port >= 0) {
    char tmp[20];
    sprintf(tmp, "%ld", port);
    setSpec(tmp);
  }
  else {
    disable();
  }
}

void
ChannelEndPoint::setSpec(const char *hostName, long port) {
  int length;

  isUnix_ = false;
  isTCP_ = false;

  SAFE_FREE(spec_);

  if (hostName && strlen(hostName) && port >= 1)
  {
    length = snprintf(NULL, 0, "tcp:%s:%ld", hostName, port);
    spec_ = (char *)calloc(length + 1, sizeof(char));
    snprintf(spec_, length+1, "tcp:%s:%ld", hostName, port);
    isTCP_ = true;
  }
  else setSpec((char*)NULL);
}

bool
ChannelEndPoint::getSpec(char **socketUri) const {

  if (socketUri) *socketUri = NULL;

  char *unixPath = NULL;
  char *hostName = NULL;
  long port = -1;

  char *newSocketUri = NULL;
  int length = -1;

  if (getUnixPath(&unixPath))
  {
    length = snprintf(NULL, 0, "unix:%s", unixPath);
  }
  else if (getTCPHostAndPort(&hostName, &port))
  {
    length = snprintf(NULL, 0, "tcp:%s:%ld", hostName, port);
  }

  if (length > 0)
  {
    newSocketUri = (char *)calloc(length + 1, sizeof(char));
    if (isUnixSocket())
      snprintf(newSocketUri, length+1, "unix:%s", unixPath);
    else
      snprintf(newSocketUri, length+1, "tcp:%s:%ld", hostName, port);

    if (socketUri)
      *socketUri = strdup(newSocketUri);
  }

  SAFE_FREE(newSocketUri);
  SAFE_FREE(unixPath);
  SAFE_FREE(hostName);

  if (NULL != *socketUri)
    return true;

  return false;
}

void
ChannelEndPoint::setDefaultTCPPort(long port) {
  defaultTCPPort_ = port;
  isTCP_ = getTCPHostAndPort();
}

void
ChannelEndPoint::setDefaultTCPInterface(int publicInterface) {
  defaultTCPInterface_ = publicInterface;
}

void
ChannelEndPoint::setDefaultUnixPath(char *path) {
  SAFE_FREE(defaultUnixPath_);

  if (path && strlen(path))
    defaultUnixPath_ = strdup(path);
  else
    defaultUnixPath_ = NULL;

  isUnix_ = getUnixPath();
}

void
ChannelEndPoint::disable() {
  setSpec("0");
}

bool
ChannelEndPoint::getPort(long *port) const {
  if (port) *port = 0;
  long p = -1;
  if (spec_) {
    char *end;
    p = strtol(spec_, &end, 10);
    if ((end == spec_) || (*end != '\0'))
      return false;
  }

  if (port) *port = p;
  return true;
}

bool
ChannelEndPoint::getUnixPath(char **unixPath) const {

  if (unixPath) *unixPath = NULL;

  long p;
  char *path = NULL;

  if (getPort(&p)) {
    if (p != 1) return false;
  }
  else if (spec_ && (strncmp("unix:", spec_, 5) == 0)) {
    path = spec_ + 5;
  }
  else
    return false;

  if (!path || (*path == '\0')) {
    path = defaultUnixPath_;
    if (!path)
      return false;
  }

  if (unixPath)
    *unixPath = strdup(path);

  return true;
}

bool
ChannelEndPoint::isUnixSocket() const {
  return isUnix_;
}

// FIXME!!!
static const char *
getComputerName() {
  //
  // Strangely enough, under some Windows OSes SMB
  // service doesn't bind to localhost. Fall back
  // to localhost if can't find computer name in
  // the environment. In future we should try to
  // bind to localhost and then try the other IPs.
  //

  const char *hostname = NULL;

  #ifdef __CYGWIN32__

  hostname = getenv("COMPUTERNAME");

  #endif

  if (hostname == NULL)
  {
    hostname = "localhost";
  }

  return hostname;
}

bool
ChannelEndPoint::getTCPHostAndPort(char **host, long *port) const {
  long p;
  char *h = NULL;
  ssize_t h_len;

  if (host) *host = NULL;
  if (port) *port = 0;

  if (getPort(&p)) {
    h_len = 0;
  }
  else if (spec_ && (strncmp("tcp:", spec_, 4) == 0)) {
    h = spec_ + 4;
    char *colon = strrchr(h, ':');
    if (colon) {
      char *end;
      h_len = colon++ - h;
      p = strtol(colon, &end, 10);
      if ((end == colon) || (*end != '\0'))
        return false;
    }
    else {
      h_len = strlen(h);
      p = 1;
    }
  }
  else
    return false;

  if (p == 1) p = defaultTCPPort_;
  if (p < 1) return false;

  if (port)
    *port = p;

  if (host)
    *host = ( h_len
              ? strndup(h, h_len)
              : strdup(defaultTCPInterface_ ? getComputerName() : "localhost"));

  return true;
}

bool
ChannelEndPoint::isTCPSocket() const {
  return isTCP_;
}

long ChannelEndPoint::getTCPPort() const {
  long port;
  if (getTCPHostAndPort(NULL, &port)) return port;
  return -1;
}

bool
ChannelEndPoint::enabled() const {
  return (isUnixSocket() || isTCPSocket());
}

bool
ChannelEndPoint::configured() const {
  return ( spec_ && ( strcmp(spec_, "0") != 0) );
}

bool
ChannelEndPoint::validateSpec() {
  isTCP_ = getTCPHostAndPort();
  isUnix_ = getUnixPath();
  return ( getPort() || isUnix_ || isTCP_ );
}

ChannelEndPoint &ChannelEndPoint::operator=(const ChannelEndPoint &other) {
  char *old;
  defaultTCPPort_ = other.defaultTCPPort_;
  defaultTCPInterface_ = other.defaultTCPInterface_;
  old = defaultUnixPath_;
  defaultUnixPath_ = (other.defaultUnixPath_ ? strdup(other.defaultUnixPath_) : NULL);
  SAFE_FREE(old);
  old = spec_;
  spec_ = (other.spec_ ? strdup(other.spec_) : NULL);
  SAFE_FREE(old);
  isUnix_ = getUnixPath();
  isTCP_ = getTCPHostAndPort();
  return *this;
}

std::ostream& operator<<(std::ostream& os, const ChannelEndPoint& endPoint) {
  if (endPoint.enabled()) {
    char* endPointSpec = NULL;
    if (endPoint.getSpec(&endPointSpec))
    {
      os << endPointSpec;
      SAFE_FREE(endPointSpec);
    }
    else
      os << "(invalid)";
  }
  else
  {
    os << "(disabled)";
  }
  return os;
}
