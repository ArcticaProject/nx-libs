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
  bool isUnix_;
  bool isTCP_;

  bool getPort(long *port = NULL) const;

 public:
  ChannelEndPoint(const char *spec = NULL);
  ~ChannelEndPoint();
  ChannelEndPoint &operator=(const ChannelEndPoint &other);

  bool enabled() const;
  bool disabled() { return !enabled(); }
  void disable();
  void setSpec(const char *spec);
  void setSpec(long port);
  void setSpec(const char *hostName, long port);
  bool getSpec(char **socketUri) const;
  void setDefaultTCPPort(long port);
  void setDefaultTCPInterface(int publicInterface);
  void setDefaultUnixPath(char *path);

  bool getUnixPath(char **path = NULL) const;
  bool isUnixSocket() const;
  bool getTCPHostAndPort(char **hostname = NULL, long *port = NULL) const;
  long getTCPPort() const;
  bool isTCPSocket() const;

  bool validateSpec();
};

std::ostream& operator<<(std::ostream& os, const ChannelEndPoint& endPoint);

#endif
