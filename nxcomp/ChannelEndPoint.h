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

#ifndef ChannelEndPoint_H
#define ChannelEndPoint_H

#include <iostream>
#include <sys/un.h>

class ChannelEndPoint
{
 private:
  long defaultTCPPort_;
  int defaultTCPInterface_; // 0=localhost, otherwise IP of public interface.
  char *defaultUnixPath_;
  char *spec_;

  bool specIsPort(long *port = NULL) const;

 public:
  ChannelEndPoint(const char *spec = NULL);
  ChannelEndPoint &operator=(const ChannelEndPoint &other);

  bool enabled() const;
  bool disabled() { return !enabled(); }
  void disable();
  void setSpec(const char *spec);
  void setSpec(int port);
  void setDefaultTCPPort(long port);
  void setDefaultTCPInterface(int publicInterface);
  void setDefaultUnixPath(char *path);

  bool getUnixPath(char **path = NULL) const;
  bool getTCPHostAndPort(char **hostname = NULL, long *port = NULL) const;
  long getTCPPort() const;

  bool validateSpec();
};

std::ostream& operator<<(std::ostream& os, const ChannelEndPoint& endPoint);

#endif
