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

#include "Auth.h"

#include "Misc.h"
#include "Control.h"
#include "Timestamp.h"
#include "Pipe.h"

#define DEFAULT_STRING_LIMIT  512

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Store the provided cookie as our 'fake' cookie, then
// read the 'real' cookie from the current X authority
// file. 
//

Auth::Auth(char *display, char *cookie)
{
  display_ = NULL;

  file_ = NULL;

  last_ = nullTimestamp();

  fakeCookie_ = NULL;
  realCookie_ = NULL;

  fakeData_ = NULL;
  realData_ = NULL;

  dataSize_ = 0;

  generatedCookie_ = 0;

  if (display == NULL || *display == '\0' || cookie == NULL ||
          *cookie == '\0' || strlen(cookie) != 32)
  {
    #ifdef PANIC
    *logofs << "Auth: PANIC! Can't create the X authorization data "
            << "with cookie '" << cookie << "' and display '"
            << display << "'.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Can't create the X authorization data "
         << "with cookie '" << cookie << "' and display '"
         << display << "'.\n";

    return;
  }

  #ifdef TEST
  *logofs << "Auth: Creating X authorization data with cookie '"
          << cookie << "' and display '" << display << "'.\n"
          << logofs_flush;
  #endif

  //
  // Get a local copy of all parameters.
  //

  display_ = new char[strlen(display) + 1];
  file_    = new char[DEFAULT_STRING_LIMIT];

  fakeCookie_ = new char[strlen(cookie) + 1];
  realCookie_ = new char[DEFAULT_STRING_LIMIT];

  if (display_ == NULL || file_ == NULL ||
          fakeCookie_ == NULL || realCookie_ == NULL)
  {
    #ifdef PANIC
    *logofs << "Auth: PANIC! Cannot allocate memory for the X "
            << "authorization data.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot allocate memory for the X "
         << "authorization data.\n";

    return;
  }

  strcpy(display_, display);

  *file_ = '\0';

  strcpy(fakeCookie_, cookie);

  *realCookie_ = '\0';

  //
  // Get the real cookie from the authorization file.
  //

  updateCookie();
}

Auth::~Auth()
{
  delete [] display_;
  delete [] file_;

  delete [] fakeCookie_;
  delete [] realCookie_;

  delete [] fakeData_;
  delete [] realData_;
}

//
// At the present moment the cookie is read only once,
// at the time the instance is initialized. If the auth
// file changes along the life of the session, the old
// cookie will be used. This works with X servers beca-
// use of an undocumented "feature". See nx-X11.
//

int Auth::updateCookie()
{
  if (isTimestamp(last_) == 0)
  {
    #ifdef TEST
    *logofs << "Auth: Reading the X authorization file "
            << "with last update at " << strMsTimestamp(last_)
            << ".\n" << logofs_flush;
    #endif

    if (getCookie() == 1 && validateCookie() == 1)
    {
      //
      // It should rather be the modification time
      // the auth file, so we can read it again if
      // the file is changed.
      //

      #ifdef TEST
      *logofs << "Auth: Setting last X authorization file "
              << "update at " << strMsTimestamp() << ".\n"
              << logofs_flush;
      #endif

      last_ = getTimestamp();

      return 1;
    }

    #ifdef PANIC
    *logofs << "Auth: PANIC! Cannot read the cookie from the X "
            << "authorization file.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot read the cookie from the X "
         << "authorization file.\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "Auth: WARNING! Skipping check on the X "
          << "authorization file.\n" << logofs_flush;
  #endif

  return 0;
}

int Auth::getCookie()
{
  //
  // Check the name of the auth file that we are going to use.
  // It can be either the value of the XAUTHORITY environment
  // or the default .Xauthority file in the user's home.
  //

  char *environment;

  environment = getenv("XAUTHORITY");

  if (environment != NULL && *environment != '\0')
  {
    strncpy(file_, environment, DEFAULT_STRING_LIMIT - 1);
  }
  else
  {
    snprintf(file_, DEFAULT_STRING_LIMIT - 1, "%s/.Xauthority",
                 control -> HomePath);
  }

  *(file_ + DEFAULT_STRING_LIMIT - 1) = '\0';

  #ifdef TEST
  *logofs << "Auth: Using X authorization file '" << file_
          << "'.\n" << logofs_flush;
  #endif

  //
  // Use the nxauth command on Windows and the Mac, xauth
  // on all the other platforms. On Windows we assume that
  // the nxauth command is located under bin in the client
  // installation directory. On Mac OS X we assume that the
  // command is located directly in the client installation
  // directory, to make bundle shipping easier. On all the
  // other platforms we use the default xauth command that
  // is in our path.
  //

  char command[DEFAULT_STRING_LIMIT];

  #if defined(__CYGWIN32__)

  snprintf(command, DEFAULT_STRING_LIMIT - 1,
               "%s/bin/nxauth", control -> SystemPath);

  *(command + DEFAULT_STRING_LIMIT - 1) = '\0';

  #elif defined(__APPLE__)

  snprintf(command, DEFAULT_STRING_LIMIT - 1,
               "%s/nxauth", control -> SystemPath);

  *(command + DEFAULT_STRING_LIMIT - 1) = '\0';

  #else

  strcpy(command, "xauth");

  #endif

  #ifdef TEST
  *logofs << "Auth: Using X auth command '" << command
          << "'.\n" << logofs_flush;
  #endif

  //
  // The SSH code forces using the unix:n port when passing localhost:n.
  // This is probably because localhost:n can fail to return a valid
  // entry on machines where the hostname for localhost doesn't match
  // exactly the 'localhost' string. For example, on a freshly installed
  // Fedora Core 3 I get a 'localhost.localdomain/unix:0' entry. Query-
  // ing 'xauth list localhost:0' results in an empty result, while the
  // query 'xauth list unix:0' works as expected. Note anyway that if
  // the cookie for the TCP connection on 'localhost' is set to a dif-
  // ferent cookie than the one for the Unix connections, both SSH and
  // NX will match the wrong cookie and session will fail.
  //
  
  char line[DEFAULT_STRING_LIMIT];

  if (strncmp(display_, "localhost:", 10) == 0)
  {
    snprintf(line, DEFAULT_STRING_LIMIT, "unix:%s", display_ + 10);
  }
  else
  {
    snprintf(line, DEFAULT_STRING_LIMIT, "%.200s", display_);
  }

  const char *parameters[256];

  parameters[0] = command;
  parameters[1] = command;
  parameters[2] = "-f";
  parameters[3] = file_;
  parameters[4] = "list";
  parameters[5] = line;
  parameters[6] = NULL;

  #ifdef TEST
  *logofs << "Auth: Executing command ";

  for (int i = 0; i < 256 && parameters[i] != NULL; i++)
  {
    *logofs << "[" << parameters[i] << "]";
  }

  *logofs << ".\n" << logofs_flush;
  #endif

  //
  // Use the popen() function to read the result
  // of the command. We would better use our own
  // implementation.
  //

  FILE *data = Popen((char *const *) parameters, "r");

  int result = -1;

  if (data == NULL)
  {
    #ifdef PANIC
    *logofs << "Auth: PANIC! Failed to execute the X auth command.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failed to execute the X auth command.\n";

    goto AuthGetCookieResult;
  }

  if (fgets(line, DEFAULT_STRING_LIMIT, data) == NULL)
  {
    #ifdef WARNING
    *logofs << "Auth: WARNING! Failed to read data from the X "
            << "auth command.\n" << logofs_flush;
    #endif

    #ifdef TEST
    cerr << "Warning" << ": Failed to read data from the X "
         << "auth command.\n";
    #endif

    #ifdef PANIC
    *logofs << "Auth: WARNING! Generating a fake cookie for "
            << "X authentication.\n" << logofs_flush;
    #endif

    #ifdef TEST
    cerr << "Warning" << ": Generating a fake cookie for "
         << "X authentication.\n";
    #endif

    generateCookie(realCookie_);
  }
  else
  {
    #ifdef TEST
    *logofs << "Auth: Checking cookie in string '" << line
            << "'.\n" << logofs_flush;
    #endif

    // 
    // Skip the hostname in the authority entry
    // just in case it includes some white spaces.
    //

    char *cookie = NULL;

    cookie = index(line, ':');

    if (cookie == NULL)
    {
      cookie = line;
    }

    if (sscanf(cookie, "%*s %*s %511s", realCookie_) != 1)
    {
      #ifdef PANIC
      *logofs << "Auth: PANIC! Failed to identify the cookie "
              << "in string '" << line << "'.\n"
              << logofs_flush;
      #endif

      cerr << "Error" << ": Failed to identify the cookie "
           << "in string '" << line << "'.\n";

      goto AuthGetCookieResult;
    }

    #ifdef TEST
    *logofs << "Auth: Got cookie '" << realCookie_
            << "' from file '" << file_ << "'.\n"
            << logofs_flush;
    #endif
  }

  result = 1;

AuthGetCookieResult:

  if (data != NULL)
  {
    Pclose(data);
  }

  return result;
}

int Auth::validateCookie()
{
  unsigned int length = strlen(realCookie_);

  if (length > DEFAULT_STRING_LIMIT / 2 - 1 ||
          strlen(fakeCookie_) != length)
  {
    #ifdef PANIC
    *logofs << "Auth: PANIC! Size mismatch between cookies '"
            << realCookie_ << "' and '" << fakeCookie_ << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Size mismatch between cookies '"
         << realCookie_ << "' and '" << fakeCookie_ << "'.\n";

    goto AuthValidateCookieError;
  }

  //
  // The length of the resulting data will be
  // half the size of the Hex cookie.
  //

  length = length / 2;

  fakeData_ = new char[length];
  realData_ = new char[length];

  if (fakeData_ == NULL || realData_ == NULL)
  {
    #ifdef PANIC
    *logofs << "Auth: PANIC! Cannot allocate memory for the binary X "
            << "authorization data.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Cannot allocate memory for the binary X "
         << "authorization data.\n";

    goto AuthValidateCookieError;
  }

  //
  // Translate the real cookie from Hex data
  // to its binary representation.
  //

  unsigned int value;

  for (unsigned int i = 0; i < length; i++)
  {
    if (sscanf(realCookie_ + 2 * i, "%2x", &value) != 1)
    {
      #ifdef PANIC
      *logofs << "Auth: PANIC! Bad X authorization data in real "
              << "cookie '" << realCookie_ << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Bad X authorization data in real cookie '"
           << realCookie_ << "'.\n";

      goto AuthValidateCookieError;
    }

    realData_[i] = value;

    if (sscanf(fakeCookie_ + 2 * i, "%2x", &value) != 1)
    {
      #ifdef PANIC
      *logofs << "Auth: PANIC! Bad X authorization data in fake "
              << "cookie '" << fakeCookie_ << "'.\n" << logofs_flush;
      #endif

      cerr << "Error" << ": Bad X authorization data in fake cookie '"
           << fakeCookie_ << "'.\n";

      goto AuthValidateCookieError;
    }

    fakeData_[i] = value;
  }

  dataSize_ = length;

  #ifdef TEST
  *logofs << "Auth: Validated real cookie '"
          << realCookie_ << "' and fake cookie '" << fakeCookie_
          << "' with data with size " << dataSize_ << ".\n"
          << logofs_flush;

  *logofs << "Auth: Ready to accept incoming connections.\n"
          << logofs_flush;
  #endif

  return 1;

AuthValidateCookieError:

  delete [] fakeData_;
  delete [] realData_;

  fakeData_ = NULL;
  realData_ = NULL;

  dataSize_ = 0;

  return -1;
}

int Auth::checkCookie(unsigned char *buffer)
{
  if (isValid() != 1)
  {
    #ifdef PANIC
    *logofs << "Auth: PANIC! Attempt to check the X cookie with "
            << "invalid authorization data.\n" << logofs_flush;
    #endif

    cerr << "Error" << ": Attempt to check the X cookie with "
         << "invalid authorization data.\n";

    return -1;
  }

  const char *protoName = "MIT-MAGIC-COOKIE-1";
  int protoSize = strlen(protoName);

  int matchedProtoSize;
  int matchedDataSize;

  if (buffer[0] == 0x42)
  {
    //
    // Byte order is MSB first.
    //

    matchedProtoSize = 256 * buffer[6] + buffer[7];
    matchedDataSize  = 256 * buffer[8] + buffer[9];
  }
  else if (buffer[0] == 0x6c)
  {
    //
    // Byte order is LSB first.
    //

    matchedProtoSize = buffer[6] + 256 * buffer[7];
    matchedDataSize  = buffer[8] + 256 * buffer[9];
  }
  else
  {
    #ifdef WARNING
    *logofs << "Auth: WARNING! Bad X connection data in the buffer.\n"
            << logofs_flush;
    #endif

    cerr << "Warning" << ": Bad X connection data in the buffer.\n";

    return -1;
  }

  //
  // Check if both the authentication protocol
  // and the fake cookie match our data.
  //

  int protoOffset = 12;

  #ifdef TEST
  *logofs << "Auth: Received a protocol size of "
          << matchedProtoSize << " bytes.\n"
          << logofs_flush;
  #endif

  if (matchedProtoSize != protoSize ||
          memcmp(buffer + protoOffset, protoName, protoSize) != 0)
  {
    #ifdef WARNING
    *logofs << "Auth: WARNING! Protocol mismatch or no X "
            << "authentication data.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Protocol mismatch or no X "
         << "authentication data.\n";

    return -1;
  }

  int dataOffset = protoOffset + ((matchedProtoSize + 3) & ~3);

  #ifdef TEST
  *logofs << "Auth: Received a data size of "
          << matchedDataSize << " bytes.\n"
          << logofs_flush;
  #endif

  if (matchedDataSize != dataSize_ ||
           memcmp(buffer + dataOffset, fakeData_, dataSize_) != 0)
  {
    #ifdef WARNING
    *logofs << "Auth: WARNING! Cookie mismatch in the X "
            << "authentication data.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Cookie mismatch in the X "
         << "authentication data.\n";

    return -1;
  }

  //
  // Everything is OK. Replace the fake data.
  //

  #ifdef TEST
  *logofs << "Auth: Replacing fake X authentication data "
          << "with the real data.\n" << logofs_flush;
  #endif

  memcpy(buffer + dataOffset, realData_, dataSize_);

  return 1;
}

void Auth::generateCookie(char *cookie)
{
  //
  // Code is from the SSH implementation, except that
  // we use a much weaker random number generator.
  // This is not critical, anyway, as this is just a
  // fake cookie. The X server doesn't have a cookie
  // for the display, so it will ignore the value we
  // feed to it.
  //

  T_timestamp timer = getTimestamp();

  srand((unsigned int) timer.tv_usec);

  unsigned int data = rand();

  for (int i = 0; i < 16; i++)
  {
    if (i % 4 == 0)
    {
      data = rand();
    }

    snprintf(cookie + 2 * i, 3, "%02x", data & 0xff);

    data >>= 8;
  }

  generatedCookie_ = 1;

  #ifdef TEST
  *logofs << "Auth: Generated X cookie string '"
          << cookie << "'.\n" << logofs_flush;
  #endif
}
