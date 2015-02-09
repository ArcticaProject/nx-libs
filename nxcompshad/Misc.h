/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPSHAD, NX protocol compression and NX extensions to this software */
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

#include <cerrno>
#include <cstring>

using namespace std;

//
// Error handling macros.
//

#define ESET(e)  (errno = (e))
#define EGET()   (errno)
#define ESTR()   strerror(errno)

//
// Log file.
//

extern ostream *logofs;

#endif /* Misc_H */
