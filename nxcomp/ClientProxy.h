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

#ifndef ClientProxy_H
#define ClientProxy_H

#include "Proxy.h"

//
// Set the verbosity level.
//

#undef  TEST
#undef  DEBUG

class ClientProxy : public Proxy
{
  public:

  ClientProxy(int proxyFD);

  virtual ~ClientProxy();

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

  virtual int handleLoad(T_load_type type);
  virtual int handleSave();

  virtual int handleAsyncEvents();

  virtual int handleLoadFromProxy();
  virtual int handleSaveFromProxy();

  virtual int handleSaveAllStores(ostream *cachefs, md5_state_t *md5StateStream,
                                      md5_state_t *md5StateClient) const;

  virtual int handleLoadAllStores(istream *cachefs, md5_state_t *md5StateStream) const;

  //
  // Utility function used to realize
  // a new connection.
  //

  protected:

  virtual int checkLocalChannelMap(int channelId)
  {
    if (control -> isProtoStep7() == 1)
    {
      return ((channelId & control -> ChannelMask) != 0);
    }
    else
    {
      return 1;
    }
  }

  //
  // Ports where to forward extended services'
  // TCP connections.
  //

  private:

  char *fontServerPort_;
};


#endif /* ClientProxy_H */
