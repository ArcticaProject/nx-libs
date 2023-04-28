/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
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

#ifndef ServerProxy_H
#define ServerProxy_H

#include <sys/types.h>
#include <sys/socket.h>

#include "Proxy.h"

#include "Misc.h"
#include "ChannelEndPoint.h"

//
// Set the verbosity level.
//

#undef  TEST
#undef  DEBUG

class ServerProxy : public Proxy
{
  public:

  ServerProxy(int proxyFd);

  virtual ~ServerProxy();

  virtual void handleDisplayConfiguration(const char *xServerDisplay, int xServerAddrFamily,
                                              sockaddr *xServerAddr, unsigned int xServerAddrLength);

  virtual void handlePortConfiguration(ChannelEndPoint &cupsServerPort,
                                       ChannelEndPoint &smbServerPort,
                                       ChannelEndPoint &mediaServerPort,
                                       ChannelEndPoint &httpServerPort,
                                       const char *fontServerPort);

  protected:

  //
  // Create a new channel.
  //

  virtual int handleNewConnection(T_channel_type type, int clientFd);

  virtual int handleNewConnectionFromProxy(T_channel_type type, int channelId);

  virtual int handleNewAgentConnection(Agent *agent);

  virtual int handleNewXConnection(int clientFd);

  virtual int handleNewXConnectionFromProxy(int channelId);

  //
  // Implement persistence according
  // to our proxy mode.
  //

  virtual int handleLoad(T_load_type type)
  {
    return 0;
  }

  virtual int handleSave()
  {
    return 0;
  }

  virtual int handleAsyncEvents()
  {
    return 0;
  }

  virtual int handleLoadFromProxy();
  virtual int handleSaveFromProxy();

  virtual int handleSaveAllStores(ostream *cachefs, md5_state_t *md5StateStream,
                                      md5_state_t *md5StateClient) const;

  virtual int handleLoadAllStores(istream *cachefs, md5_state_t *md5StateStream) const;

  int handleCheckDrop();
  int handleCheckLoad();

  //
  // Utility function used to realize
  // a new connection.
  //

  protected:

  virtual int checkLocalChannelMap(int channelId)
  {
    // Since ProtoStep7 (#issue 108)
    return ((channelId & control -> ChannelMask) == 0);
  }

  private:

  // FIXME: Use a ChannelEndPoint object also for the X server!
  int xServerAddrFamily_;
  sockaddr *xServerAddr_;
  unsigned int xServerAddrLength_;

  //
  // This is the name of the X display where
  // we are going to forward connections.
  //

  char *xServerDisplay_;

  //
  // Ports where to forward extended services'
  // TCP connections.
  //

  ChannelEndPoint cupsServerPort_;
  ChannelEndPoint smbServerPort_;
  ChannelEndPoint mediaServerPort_;
  ChannelEndPoint httpServerPort_;

  //
  // It will have to be passed to the channel
  // so that it can set the port where the
  // font server connections are tunneled.
  //

  char *fontServerPort_;
};

#endif /* ServerProxy_H */
