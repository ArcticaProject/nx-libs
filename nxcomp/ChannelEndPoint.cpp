/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2015 Qindel Formacion y Servicios SL.                    */
/*                                                                        */
/* This program is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License Version 2, as     */
/* published by the Free Software Foundation.                             */
/*                                                                        */
/* This program is distributed in the hope that it will be useful, but    */
/* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTA-  */
/* BILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General       */
/* Public License for more details.                                       */
/*                                                                        */
/* You should have received a copy of the GNU General Public License      */
/* along with this program; if not, you can request a copy to Qindel      */
/* or write to the Free Software Foundation, Inc., 59 Temple Place, Suite */
/* 330, Boston, MA  02111-1307 USA                                        */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "ChannelEndPoint.h"

#include "NXalert.h"

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
}

void
ChannelEndPoint::setSpec(const char *spec) {
  if (spec_) free(spec_);

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

  if (spec_) free(spec_);
  isUnix_ = false;
  isTCP_ = false;

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

  free(newSocketUri);
  free(unixPath);
  free(hostName);

  if (*socketUri != '\0')
    return true;

  return false;
}

void
ChannelEndPoint::setDefaultTCPPort(long port) {
  defaultTCPPort_ = port;
}

void
ChannelEndPoint::setDefaultTCPInterface(int publicInterface) {
  defaultTCPInterface_ = publicInterface;
}

void
ChannelEndPoint::setDefaultUnixPath(char *path) {
  if (defaultUnixPath_) free(defaultUnixPath_);

  if (path && strlen(path))
    defaultUnixPath_ = strdup(path);
  else
    defaultUnixPath_ = NULL;
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

  if (unixPath) *unixPath = 0;

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
  free(old);
  old = spec_;
  spec_ = (other.spec_ ? strdup(other.spec_) : NULL);
  free(old);
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
      free(endPointSpec);
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
