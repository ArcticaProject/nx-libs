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

#include "Socket.h"
#include "Agent.h"

#include "ClientProxy.h"

#include "ClientChannel.h"
#include "GenericChannel.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Log the operations related to sending
// and receiving the control tokens.
//

#undef  TOKEN

ClientProxy::ClientProxy(int proxyFd) : Proxy(proxyFd)
{
  fontServerPort_ = NULL;

  #ifdef DEBUG
  *logofs << "ClientProxy: Created new object at " << this
          << ".\n" << logofs_flush;
  #endif
}

ClientProxy::~ClientProxy()
{
  delete [] fontServerPort_;

  #ifdef DEBUG
  *logofs << "ClientProxy: Deleted object at " << this
          << ".\n" << logofs_flush;
  #endif
}

void ClientProxy::handleDisplayConfiguration(const char *xServerDisplay, int xServerAddrFamily,
                                                 sockaddr * xServerAddr, unsigned int xServerAddrLength)
{
  #ifdef DEBUG
  *logofs << "ClientProxy: No display configuration to set.\n"
          << logofs_flush;
  #endif
}

void ClientProxy::handlePortConfiguration(int cupsServerPort, int smbServerPort, int mediaServerPort,
                                              int httpServerPort, const char *fontServerPort)
{
  delete [] fontServerPort_;

  fontServerPort_ = new char[strlen(fontServerPort) + 1];

  strcpy(fontServerPort_, fontServerPort);

  #ifdef DEBUG
  *logofs << "ClientProxy: Set port configuration to font '"
          << fontServerPort_ << "'.\n"
          << logofs_flush;
  #endif
}

int ClientProxy::handleNewConnection(T_channel_type type, int clientFd)
{
  switch (type)
  {
    case channel_x11:
    {
      return handleNewXConnection(clientFd);
    }
    case channel_cups:
    {
      return handleNewGenericConnection(clientFd, channel_cups, "CUPS");
    }
    case channel_smb:
    {
      return handleNewGenericConnection(clientFd, channel_smb, "SMB");
    }
    case channel_media:
    {
      return handleNewGenericConnection(clientFd, channel_media, "media");
    }
    case channel_http:
    {
      return handleNewGenericConnection(clientFd, channel_http, "HTTP");
    }
    case channel_slave:
    {
      return handleNewSlaveConnection(clientFd);
    }
    default:
    {
      #ifdef PANIC
      *logofs << "ClientProxy: PANIC! Unsupported channel with type '"
              << getTypeName(type) << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unsupported channel with type '"
           << getTypeName(type) << "'.\n";

      return -1;
    }
  }
}

int ClientProxy::handleNewConnectionFromProxy(T_channel_type type, int channelId)
{
  switch (type)
  {
    case channel_font:
    {
      int port = atoi(fontServerPort_);

      if (port > 0)
      {
        //
        // Connect on the TCP port number.
        //

        return handleNewGenericConnectionFromProxy(channelId, channel_font, "localhost",
                                                       port, "font");
      }
      else
      {
        //
        // Connect to the Unix path.
        //

        return handleNewGenericConnectionFromProxy(channelId, channel_font, "localhost",
                                                       fontServerPort_, "font");
      }
    }
    case channel_slave:
    {
      return handleNewSlaveConnectionFromProxy(channelId);
    }
    default:
    {
      #ifdef PANIC
      *logofs << "ClientProxy: PANIC! Unsupported channel with type '"
              << getTypeName(type) << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unsupported channel with type '"
           << getTypeName(type) << "'.\n";

      return -1;
    }
  }
}

int ClientProxy::handleNewAgentConnection(Agent *agent)
{
  int clientFd = agent -> getLocalFd();

  int channelId = allocateChannelMap(clientFd);

  if (channelId == -1)
  {
    #ifdef PANIC
    *logofs << "ClientProxy: PANIC! Maximum mumber of available "
            << "channels exceeded.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Maximum mumber of available "
         << "channels exceeded.\n";

    return -1;
  }

  transports_[channelId] = agent -> getTransport();

  agent_ = channelId;

  return handleNewXConnection(clientFd);
}

int ClientProxy::handleNewXConnection(int clientFd)
{
  int channelId = getChannel(clientFd);

  //
  // Check if the channel has been
  // already mapped.
  //

  if (channelId == -1)
  {
    channelId = allocateChannelMap(clientFd);

    if (channelId == -1)
    {
      #ifdef PANIC
      *logofs << "ClientProxy: PANIC! Maximum mumber of available "
              << "channels exceeded.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Maximum mumber of available "
           << "channels exceeded.\n";

      return -1;
    }
  }

  #ifdef TEST
  *logofs << "ClientProxy: X client descriptor FD#" << clientFd
          << " mapped to channel ID#" << channelId << ".\n"
          << logofs_flush;
  #endif

  //
  // Turn queuing off for path proxy-to-X-client.
  //

  if (control -> OptionClientNoDelay == 1)
  {
    SetNoDelay(clientFd, control -> OptionClientNoDelay);
  }

  //
  // If requested, set the size of the TCP send
  // and receive buffers.
  //

  if (control -> OptionClientSendBuffer != -1)
  {
    SetSendBuffer(clientFd, control -> OptionClientSendBuffer);
  }

  if (control -> OptionClientReceiveBuffer != -1)
  {
    SetReceiveBuffer(clientFd, control -> OptionClientReceiveBuffer);
  }

  if (allocateTransport(clientFd, channelId) < 0)
  {
    return -1;
  }

  //
  // Starting from protocol level 3 client and server
  // caches are created in proxy and shared between all
  // channels. If remote proxy has older protocol level
  // pointers are NULL and channels must create their
  // own instances.
  //

  channels_[channelId] = new ClientChannel(transports_[channelId], compressor_);

  if (channels_[channelId] == NULL)
  {
    deallocateTransport(channelId);

    return -1;
  }

  increaseChannels(channelId);

  //
  // Propagate channel stores and caches to the new
  // channel.
  //

  channels_[channelId] -> setOpcodes(opcodeStore_);

  channels_[channelId] -> setStores(clientStore_, serverStore_);

  channels_[channelId] -> setCaches(clientCache_, serverCache_);

  int port = atoi(fontServerPort_);

  if (port > 0 || *fontServerPort_ != '\0')
  {
    channels_[channelId] -> setPorts(1);
  }

  if (handleControl(code_new_x_connection, channelId) < 0)
  {
    return -1;
  }

  //
  // Let channel configure itself according
  // to control parameters.
  //

  channels_[channelId] -> handleConfiguration();

  return 1;
}

int ClientProxy::handleNewXConnectionFromProxy(int channelId)
{
  #ifdef PANIC
  *logofs << "ClientProxy: PANIC! Can't create a new X channel "
          << "with ID#" << channelId << " at this side.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Can't create a new X channel "
       << "with ID#" << channelId << " at this side.\n";

  return -1;
}

int ClientProxy::handleLoad(T_load_type type)
{
  int channelCount = getChannels(channel_x11);

  if ((channelCount == 0 && type == load_if_first) ||
          (channelCount > 0 && type == load_if_any))
  {
    #ifdef TEST
    *logofs << "ClientProxy: Going to load content of client store.\n"
            << logofs_flush;
    #endif

    int result = handleLoadStores();

    if (result == 1)
    {
      if (handleControl(code_load_request) < 0)
      {
        return -1;
      }
 
      priority_ = 1;
    }
    else if (result < 0)
    {
      #ifdef WARNING
      *logofs << "ClientProxy: WARNING! Failed to load content "
              << "of persistent cache.\n" << logofs_flush;
      #endif

      //
      // Don't abort the proxy connection in the case
      // of a corrupted cache. By not sending the load
      // message to the remote peer, both sides will
      // start encoding messages using empty stores.
      // This behaviour is compatible with old proxy
      // versions.
      //

      if (channelCount == 0 && type == load_if_first)
      {
        if (handleResetStores() < 0)
        {
          #ifdef PANIC
          *logofs << "ClientProxy: PANIC! Failed to reset message stores.\n"
                  << logofs_flush;
          #endif

          return -1;
        }
      }
      else
      {
        return -1;
      }
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "ClientProxy: PANIC! Can't load the stores with "
            << channelCount << " remaining channels.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int ClientProxy::handleSave()
{
  //
  // If no more X channels are remaining
  // then save content of message stores.
  //

  int channelCount = getChannels(channel_x11);

  if (channelCount == 0)
  {
    int result = handleSaveStores();

    if (result == 1)
    {
      if (handleControl(code_save_request) < 0)
      {
        return -1;
      }

      priority_ = 1;

      return 1;
    }
    else if (result < 0)
    {
      #ifdef PANIC
      *logofs << "ClientProxy: PANIC! Failed to save stores "
              << "to persistent cache.\n" << logofs_flush;
      #endif

      return -1;
    }
  }
  else
  {
    #ifdef PANIC
    *logofs << "ClientProxy: PANIC! Can't save the stores with "
            << channelCount << " remaining channels.\n"
            << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int ClientProxy::handleAsyncEvents()
{
  if (canRead() == 1)
  {
    #if defined(TEST) || defined(INFO)
    *logofs << "Proxy: WARNING! Reading while writing "
            << "with data available on the proxy link.\n"
            << logofs_flush;
    #endif

    if (handleRead() < 0)
    {
      return -1;
    }

    return 1;
  }

  return 0;
}

int ClientProxy::handleLoadFromProxy()
{
  #ifdef PANIC
  *logofs << "ClientProxy: PANIC! Invalid load control message "
          << "received in proxy.\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Invalid load control message "
       << "received in proxy.\n";

  return -1;
}

int ClientProxy::handleSaveFromProxy()
{
  #ifdef PANIC
  *logofs << "ClientProxy: PANIC! Invalid save control message "
          << "received in proxy.\n" << logofs_flush;
  #endif

  cerr << "Error" << ": Invalid save control message "
       << "received in proxy.\n";

  return -1;
}

int ClientProxy::handleSaveAllStores(ostream *cachefs, md5_state_t *md5StateStream,
                                         md5_state_t *md5StateClient) const
{
  if (clientStore_ -> saveRequestStores(cachefs, md5StateStream, md5StateClient,
                                            use_checksum, discard_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> saveReplyStores(cachefs, md5StateStream, md5StateClient,
                                               discard_checksum, use_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> saveEventStores(cachefs, md5StateStream, md5StateClient,
                                               discard_checksum, use_data) < 0)
  {
    return -1;
  }

  return 1;
}

int ClientProxy::handleLoadAllStores(istream *cachefs, md5_state_t *md5StateStream) const
{
  if (clientStore_ -> loadRequestStores(cachefs, md5StateStream,
                                            use_checksum, discard_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> loadReplyStores(cachefs, md5StateStream,
                                               discard_checksum, use_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> loadEventStores(cachefs, md5StateStream,
                                               discard_checksum, use_data) < 0)
  {
    return -1;
  }

  return 1;
}

