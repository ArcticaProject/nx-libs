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

#ifndef ServerProxy_H
#define ServerProxy_H

#include <sys/types.h>
#include <sys/socket.h>

#include "Proxy.h"

#include "Misc.h"

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

  virtual void handlePortConfiguration(int cupsServerPort, int smbServerPort, int mediaServerPort,
                                           int httpServerPort, const char *fontServerPort);

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
    if (control -> isProtoStep7() == 1)
    {
      return ((channelId & control -> ChannelMask) == 0);
    }
    else
    {
      return 0;
    }
  }

  private:

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

  int cupsServerPort_;
  int smbServerPort_;
  int mediaServerPort_;
  int httpServerPort_;

  //
  // It will have to be passed to the channel
  // so that it can set the port where the
  // font server connections are tunneled.
  //

  char *fontServerPort_;
};

#endif /* ServerProxy_H */
