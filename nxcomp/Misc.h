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

#ifndef Misc_H
#define Misc_H

#include <iostream>
#include <fstream>

#include <cerrno>
#include <cstring>

#ifdef __sun

#include <strings.h>

#endif

using namespace std;

//
// This is MD5 length.
//

#define MD5_LENGTH          16

//
// Error handling macros.
//

#define ESET(e)  (errno = (e))
#define EGET()   (errno)
#define ESTR()   strerror(errno)

//
// TCP port offset applied to NX port specification.
//

extern const int DEFAULT_NX_PROXY_PORT_OFFSET;

//
// Default TCP port used by client proxy to listen
// to X clients and by server proxy to connect to
// remote.
//

extern const int DEFAULT_NX_PROXY_PORT;

//
// Default X display number that client
// proxy imitates.
//

extern const int DEFAULT_NX_X_PORT;

//
// Establish the port offsets for the additional
// services.
//

extern const int DEFAULT_NX_CUPS_PORT_OFFSET;
extern const int DEFAULT_NX_SMB_PORT_OFFSET;
extern const int DEFAULT_NX_MEDIA_PORT_OFFSET;
extern const int DEFAULT_NX_AUX_PORT_OFFSET;
extern const int DEFAULT_NX_HTTP_PORT_OFFSET;
extern const int DEFAULT_NX_FONT_PORT_OFFSET;

//
// Slave channels can be originated by both sides
// so they need to have different port offsets
// in the case the user runs both proxies on the
// same host.
//

extern const int DEFAULT_NX_SLAVE_PORT_CLIENT_OFFSET;
extern const int DEFAULT_NX_SLAVE_PORT_SERVER_OFFSET;

//
// NX proxy binds to all network interfaces by default
// With the -loopback parameter, you can switch
// over to binding to the loopback device only.
//

extern const int DEFAULT_LOOPBACK_BIND;

//
// Return strings containing various info.
//

const char *GetUsageInfo();
const char *GetCopyrightInfo();
const char *GetOtherCopyrightInfo();

//
// Define this if you want immediate flush of
// the log output.
//

#define FLUSH_LOGOFS

//
// Global objects providing shared functions.
//

class Auth;
class Control;
class Statistics;

extern Auth       *auth;
extern Control    *control;
extern Statistics *statistics;

//
// Log file.
//

extern ostream *logofs;

//
// Cleanup code.
//

void HandleAbort() __attribute__((noreturn));
void HandleShutdown() __attribute__((noreturn));

extern "C"
{
  void HandleCleanup(int code = 0) __attribute__((noreturn));
  void HandleCleanupForReconnect();
}

//
// Manage signal handlers.
//

void DisableSignals();
void EnableSignals();

//
// Manage timers.
//

void SetTimer(int value);
void ResetTimer();

//
// Show a dialog asking the user if he/she
// wants to close the current session. Look
// in the alerts file for the known critical
// events.
//

void HandleAlert(int code, int local);

//
// Run the callback registered by the proxy
// or the agent.
//

void KeeperCallback();
void FlushCallback(int length);

//
// Return the string literal corresponding
// the value.
//

const char *DumpSignal(int signal);
const char *DumpPolicy(int type);
const char *DumpControl(int code);
const char *DumpSession(int code);
const char *DumpAction(int type);
const char *DumpState(int type);
const char *DumpToken(int type);

//
// Print out content of buffer to log file.
// You need to define DUMP or OPCODES in
// the source to have these compiled.
//

const char *DumpOpcode(const int &opcode);
const char *DumpChecksum(const void *checksum);

void DumpData(const unsigned char *data, unsigned int length);
void DumpHexData(const unsigned char *data, unsigned int length);
void DumpChecksum(const unsigned char *data, unsigned int length);
void DumpBlockChecksums(const unsigned char *data, unsigned int length,
                            unsigned int block);

//
// Defines logofs_flush as an empty string to
// avoid calling the corresponding ostream's
// flush() function.
//

#ifdef FLUSH_LOGOFS

#define logofs_flush "" ; logofs -> flush()

#else

#define logofs_flush ""

#endif

//
// Is the host where local proxy is running
// big-endian?
//

extern int _hostBigEndian;
extern int _storeBigEndian;

inline void setHostBigEndian(int flag)
{
  _hostBigEndian = flag;
}

inline int hostBigEndian()
{
  return _hostBigEndian;
}

inline int storeBigEndian()
{
  return _storeBigEndian;
}

extern const unsigned int IntMask[33];

unsigned int GetUINT(unsigned const char *buffer, int bigEndian);
unsigned int GetULONG(unsigned const char *buffer, int bigEndian);
void PutUINT(unsigned int value, unsigned char *buffer, int bigEndian);
void PutULONG(unsigned int value, unsigned char *buffer, int bigEndian);

inline void CleanData(unsigned char *buffer, int size)
{
  unsigned char *end = buffer + size;

  while (buffer < end)
  {
    *buffer++ = 0x00;
  }
}
  
int CheckData(istream *fs);
int CheckData(ostream *fs);
int PutData(ostream *fs, const unsigned char *buffer, int size);
int GetData(istream *fs, unsigned char *buffer, int size);
int FlushData(ostream *fs);

unsigned int RoundUp2(unsigned int x);
unsigned int RoundUp4(unsigned int x);
unsigned int RoundUp8(unsigned int x);

#endif /* Misc_H */
