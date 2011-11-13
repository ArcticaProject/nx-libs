/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
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

#include <unistd.h>

#include "NXalert.h"

#include "Socket.h"

#include "ServerProxy.h"

#include "ServerChannel.h"
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

ServerProxy::ServerProxy(int proxyFd) : Proxy(proxyFd)

{
  xServerAddrFamily_ = -1;
  xServerAddrLength_ = 0;

  xServerAddr_    = NULL;
  xServerDisplay_ = NULL;

  cupsServerPort_  = -1;
  smbServerPort_   = -1;
  mediaServerPort_ = -1;
  httpServerPort_  = -1;

  fontServerPort_ = NULL;

  #ifdef DEBUG
  *logofs << "ServerProxy: Created new object at " << this
          << ".\n" << logofs_flush;
  #endif
}

ServerProxy::~ServerProxy()
{
  delete xServerAddr_;

  delete [] xServerDisplay_;

  delete [] fontServerPort_;

  #ifdef DEBUG
  *logofs << "ServerProxy: Deleted object at " << this
          << ".\n" << logofs_flush;
  #endif
}

void ServerProxy::handleDisplayConfiguration(const char *xServerDisplay, int xServerAddrFamily,
                                                 sockaddr *xServerAddr, unsigned int xServerAddrLength)
{
  delete xServerAddr_;

  xServerAddr_ = xServerAddr;

  xServerAddrFamily_ = xServerAddrFamily;
  xServerAddrLength_ = xServerAddrLength;

  delete [] xServerDisplay_;

  xServerDisplay_ = new char[strlen(xServerDisplay) + 1];

  strcpy(xServerDisplay_, xServerDisplay);

  #ifdef DEBUG
  *logofs << "ServerProxy: Set display configuration to display '"
          << xServerDisplay_ << "'.\n"
          << logofs_flush;
  #endif
}

void ServerProxy::handlePortConfiguration(int cupsServerPort, int smbServerPort, int mediaServerPort,
                                              int httpServerPort, const char *fontServerPort)
{
  cupsServerPort_  = cupsServerPort;
  smbServerPort_   = smbServerPort;
  mediaServerPort_ = mediaServerPort;
  httpServerPort_  = httpServerPort;

  delete [] fontServerPort_;

  fontServerPort_ = new char[strlen(fontServerPort) + 1];

  strcpy(fontServerPort_, fontServerPort);

  #ifdef DEBUG
  *logofs << "ServerProxy: Set port configuration to CUPS "
          << cupsServerPort_ << ", SMB " << smbServerPort_
          << ", media " << mediaServerPort_ << ", HTTP "
          << httpServerPort_ << ".\n"
          << logofs_flush;
  #endif
}

int ServerProxy::handleNewConnection(T_channel_type type, int clientFd)
{
  switch (type)
  {
    case channel_font:
    {
      return handleNewGenericConnection(clientFd, channel_font, "font");
    }
    case channel_slave:
    {
      return handleNewSlaveConnection(clientFd);
    }
    default:
    {
      #ifdef PANIC
      *logofs << "ServerProxy: PANIC! Unsupported channel with type '"
              << getTypeName(type) << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unsupported channel with type '"
           << getTypeName(type) << "'.\n";

      return -1;
    }
  }
}

int ServerProxy::handleNewConnectionFromProxy(T_channel_type type, int channelId)
{
  switch (type)
  {
    case channel_x11:
    {
      return handleNewXConnectionFromProxy(channelId);
    }
    case channel_cups:
    {
      return handleNewGenericConnectionFromProxy(channelId, channel_cups, "localhost",
                                                     cupsServerPort_, "CUPS");
    }
    case channel_smb:
    {
      return handleNewGenericConnectionFromProxy(channelId, channel_smb, getComputerName(),
                                                     smbServerPort_, "SMB");
    }
    case channel_media:
    {
      return handleNewGenericConnectionFromProxy(channelId, channel_media, "localhost",
                                                     mediaServerPort_, "media");
    }
    case channel_http:
    {
      return handleNewGenericConnectionFromProxy(channelId, channel_http, getComputerName(),
                                                     httpServerPort_, "HTTP");
    }
    case channel_slave:
    {
      return handleNewSlaveConnectionFromProxy(channelId);
    }
    default:
    {
      #ifdef PANIC
      *logofs << "ServerProxy: PANIC! Unsupported channel with type '"
              << getTypeName(type) << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Unsupported channel with type '"
           << getTypeName(type) << "'.\n";

      return -1;
    }
  }
}

int ServerProxy::handleNewAgentConnection(Agent *agent)
{
  #ifdef PANIC
  *logofs << "ServerProxy: PANIC! Can't create an agent "
          << "connection at this side.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Can't create an agent "
       << "connection at this side.\n";

  return -1;
}

int ServerProxy::handleNewXConnection(int clientFd)
{
  #ifdef PANIC
  *logofs << "ServerProxy: PANIC! Can't create a new X channel "
          << "with FD#" << clientFd << " at this side.\n"
          << logofs_flush;
  #endif

  cerr << "Error" << ": Can't create a new X channel "
       << "with FD#" << clientFd << " at this side.\n";

  return -1;
}

int ServerProxy::handleNewXConnectionFromProxy(int channelId)
{
  //
  // Connect to the real X server.
  //

  int retryConnect = control -> OptionServerRetryConnect;

  int xServerFd;

  for (;;)
  {
    xServerFd = socket(xServerAddrFamily_, SOCK_STREAM, PF_UNSPEC);

    if (xServerFd < 0)
    {
      #ifdef PANIC
      *logofs << "ServerProxy: PANIC! Call to socket failed. "
              << "Error is " << EGET() << " '" << ESTR()
              << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Call to socket failed. "
           << "Error is " << EGET() << " '" << ESTR()
           << "'.\n";

      return -1;
    }

    #ifdef TEST
    *logofs << "ServerProxy: Trying to connect to X server '"
            << xServerDisplay_ << "'.\n" << logofs_flush;
    #endif

    int result = connect(xServerFd, xServerAddr_, xServerAddrLength_);

    getNewTimestamp();

    if (result < 0)
    {
      #ifdef WARNING
      *logofs << "ServerProxy: WARNING! Connection to '"
              << xServerDisplay_ << "' failed with error '"
              << ESTR() << "'. Retrying.\n" << logofs_flush;
      #endif

      close(xServerFd);

      if (--retryConnect == 0)
      {
        #ifdef PANIC
        *logofs << "ServerProxy: PANIC! Connection to '"
                << xServerDisplay_ << "' for channel ID#"
                << channelId << " failed. Error is "
                << EGET() << " '" << ESTR() << "'.\n"
                << logofs_flush;
        #endif

        cerr << "Error" << ": Connection to '"
             << xServerDisplay_ << "' failed. Error is "
             << EGET() << " '" << ESTR() << "'.\n";

        close(xServerFd);

        return -1;
      }

      if (activeChannels_.getSize() == 0)
      {
        sleep(2);
      }
      else
      {
        sleep(1);
      }
    }
    else
    {
      break;
    }
  }

  assignChannelMap(channelId, xServerFd);

  #ifdef TEST
  *logofs << "ServerProxy: X server descriptor FD#" << xServerFd 
          << " mapped to channel ID#" << channelId << ".\n"
          << logofs_flush;
  #endif

  //
  // Turn queuing off for path proxy-to-X-server.
  //

  if (control -> OptionServerNoDelay == 1)
  {
    SetNoDelay(xServerFd, control -> OptionServerNoDelay);
  }

  //
  // If requested, set the size of the TCP send
  // and receive buffers.
  //

  if (control -> OptionServerSendBuffer != -1)
  {
    SetSendBuffer(xServerFd, control -> OptionServerSendBuffer);
  }

  if (control -> OptionServerReceiveBuffer != -1)
  {
    SetReceiveBuffer(xServerFd, control -> OptionServerReceiveBuffer);
  }

  if (allocateTransport(xServerFd, channelId) < 0)
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

  channels_[channelId] = new ServerChannel(transports_[channelId], compressor_);

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

  if (port > 0)
  {
    channels_[channelId] -> setPorts(port);
  }

  //
  // Let channel configure itself according
  // to control parameters.
  //

  channels_[channelId] -> handleConfiguration();

  //
  // Check if we have successfully loaded the
  // selected cache and, if not, remove it
  // from disk.
  //

  handleCheckLoad();

  return 1;
}

//
// Check if we still need to drop a channel. We need
// to check this explicitly at the time we receive a
// request to load or save the cache because we could
// receive the control message before having entered
// the function handling the channel events.
//

int ServerProxy::handleCheckDrop()
{
  T_list channelList = activeChannels_.copyList();

  for (T_list::iterator j = channelList.begin();
           j != channelList.end(); j++)
  {
    int channelId = *j;

    if (channels_[channelId] != NULL &&
            (channels_[channelId] -> getDrop() == 1 ||
                channels_[channelId] -> getClosing() == 1))
    {
      #ifdef TEST
      *logofs << "ServerProxy: Dropping the descriptor FD#"
              << getFd(channelId) << " channel ID#"
              << channelId << ".\n" << logofs_flush;
      #endif

      handleDrop(channelId);
    }
  }

  return 1;
}

int ServerProxy::handleCheckLoad()
{
  //
  // Check if we just created the first X channel
  // but the client side didn't tell us to load
  // the cache selected at the session negotiation.
  // This is very likely because the load operation
  // failed at the remote side, for example because
  // the cache was invalid or corrupted.
  //

  int channelCount = getChannels(channel_x11);

  if (channelCount != 1)
  {
    return 0;
  }

  if (control -> PersistentCacheEnableLoad == 1 &&
          control -> PersistentCachePath != NULL &&
              control -> PersistentCacheName != NULL &&
                  isTimestamp(timeouts_.loadTs) == 0)
  {
    #ifdef WARNING
    *logofs << "ServerProxy: WARNING! Cache file '" << control -> PersistentCachePath
            << "/" << control -> PersistentCacheName << "' not loaded.\n"
            << logofs_flush;
    #endif

    //
    // Remove the cache file.
    //

    #ifdef WARNING
    *logofs << "ServerProxy: WARNING! Removing supposedly "
            << "incompatible cache '" << control -> PersistentCachePath
            << "/" << control -> PersistentCacheName
            << "'.\n" << logofs_flush;
    #endif

    handleResetPersistentCache();
  }

  return 1;
}

int ServerProxy::handleLoadFromProxy()
{
  //
  // Be sure we drop any confirmed channel.
  //

  handleCheckDrop();

  //
  // Check that either no X channel is
  // remaining or we are inside a reset.
  //

  int channelCount = getChannels(channel_x11);

  if (channelCount > 0)
  {
    #ifdef PANIC
    *logofs << "ServerProxy: PANIC! Protocol violation "
            << "in command load with " << channelCount
            << " channels.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Protocol violation "
         << "in command load from proxy.\n";

    return -1;
  } 
  else if (handleLoadStores() < 0)
  {
    #ifdef WARNING
    *logofs << "ServerProxy: WARNING! Failed to load content "
            << "of persistent cache.\n" << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int ServerProxy::handleSaveFromProxy()
{
  //
  // Be sure we drop any confirmed channel.
  //

  handleCheckDrop();

  //
  // Now verify that all channels are gone.
  //

  int channelCount = getChannels(channel_x11);

  if (channelCount > 0)
  {
    #ifdef PANIC
    *logofs << "ServerProxy: PANIC! Protocol violation "
            << "in command save with " << channelCount
            << " channels.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Protocol violation "
         << "in command save from proxy.\n";

    return -1;
  }
  else if (handleSaveStores() < 0)
  {
    #ifdef PANIC
    *logofs << "ServerProxy: PANIC! Failed to save stores "
            << "to persistent cache.\n" << logofs_flush;
    #endif

    return -1;
  }

  return 1;
}

int ServerProxy::handleSaveAllStores(ostream *cachefs, md5_state_t *md5StateStream,
                                         md5_state_t *md5StateClient) const
{
  if (clientStore_ -> saveRequestStores(cachefs, md5StateStream, md5StateClient,
                                            discard_checksum, use_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> saveReplyStores(cachefs, md5StateStream, md5StateClient,
                                               use_checksum, discard_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> saveEventStores(cachefs, md5StateStream, md5StateClient,
                                               use_checksum, discard_data) < 0)
  {
    return -1;
  }

  return 1;
}

int ServerProxy::handleLoadAllStores(istream *cachefs, md5_state_t *md5StateStream) const
{
  if (clientStore_ -> loadRequestStores(cachefs, md5StateStream,
                                            discard_checksum, use_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> loadReplyStores(cachefs, md5StateStream,
                                               use_checksum, discard_data) < 0)
  {
    return -1;
  }
  else if (serverStore_ -> loadEventStores(cachefs, md5StateStream,
                                               use_checksum, discard_data) < 0)
  {
    return -1;
  }

  return 1;
}
