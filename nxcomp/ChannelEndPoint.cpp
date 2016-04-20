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

#include "ChannelEndPoint.h"

#include "NXalert.h"

ChannelEndPoint::ChannelEndPoint(const char *spec)
  : defaultTCPPort_(0), defaultTCPInterface_(0),
    defaultUnixPath_(NULL) {
  spec_ = (spec ? strdup(spec) : NULL);
}

void
ChannelEndPoint::setSpec(const char *spec) {
  if (spec_) free(spec_);

  if (spec && strlen(spec))
    spec_ = strdup(spec);
  else
    spec_ = NULL;
}

void
ChannelEndPoint::setSpec(int port) {
  if (port >= 0) {
    char tmp[20];
    sprintf(tmp, "%d", port);
    setSpec(tmp);
  }
  else setSpec((char*)NULL);
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
ChannelEndPoint::disable() { setSpec("0"); }

bool
ChannelEndPoint::specIsPort(long *port) const {
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

  if (specIsPort(&p)) {
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

  if (specIsPort(&p)) {
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
ChannelEndPoint::enabled() const {
  return (getUnixPath() || getTCPHostAndPort());
}

long ChannelEndPoint::getTCPPort() const {
  long port;
  if (getTCPHostAndPort(NULL, &port)) return port;
  return -1;
}

bool
ChannelEndPoint::validateSpec() {
  return (specIsPort() || getUnixPath() || getTCPHostAndPort());
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
  return *this;
}

std::ostream& operator<<(std::ostream& os, const ChannelEndPoint& endPoint) {
  if (endPoint.enabled()) {
    char *unixPath, *host;
    long port;
    if (endPoint.getUnixPath(&unixPath)) {
      os << "unix:" << unixPath;
      free(unixPath);
    }
    else if (endPoint.getTCPHostAndPort(&host, &port)) {
      os << "tcp:" << host << ":" << port;
      free(host);
    }
    else {
      os << "(invalid)";
    }
  }
  else {
    os << "(disabled)";
  }
  return os;
}
