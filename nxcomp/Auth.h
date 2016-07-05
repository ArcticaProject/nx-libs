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

#ifndef Auth_H
#define Auth_H

#include "Timestamp.h"

//
// Handle the forwarding of authorization credentials
// to the X server by replacing the fake cookie with
// the real cookie as it is read from the auth file.
// At the moment only the MIT-MAGIC-COOKIE-1 cookies
// are recognized. The implementation is based on the
// corresponding code found in the SSH client.
//

class Auth
{
  public:

  //
  // Must be created by passing the fake cookie that
  // will be forwarded by the remote end and with the
  // real X display that is going to be used for the
  // session.
  //

  Auth(char *display, char *cookie);

  ~Auth();

  int isValid()
  {
    return (isTimestamp(last_) == 1 && fakeCookie_ != NULL &&
                *fakeCookie_ != '\0' && realCookie_ != NULL &&
                    *realCookie_ != '\0' && fakeData_ != NULL &&
                        realData_ != NULL && dataSize_ != 0);
  }

  int isFake() const
  {
    return generatedCookie_;
  }

  //
  // Method called in the channel class to find if the
  // provided cookie matches the fake one. If the data
  // matches, the fake cookie is replaced with the real
  // one.
  //

  int checkCookie(unsigned char *buffer);

  protected:

  //
  // Update the real cookie for the display. If called
  // a further time, check if the auth file is changed
  // and get the new cookie.
  //

  int updateCookie();

  //
  // Find out which authorization file is to be used
  // and query the cookie for the current display.
  //

  int getCookie();

  //
  // Extract the binary data from the cookies so that
  // data can be directly compared at the time it is
  // taken from the X request.
  //

  int validateCookie();

  //
  // Generate a fake random cookie and copy it to the
  // provided string.
  //

  void generateCookie(char *cookie);

  private:

  char *display_;
  char *file_;

  T_timestamp last_;

  char *fakeCookie_;
  char *realCookie_;

  char *fakeData_;
  char *realData_;

  int dataSize_;

  int generatedCookie_;
};

#endif /* Auth_H */
