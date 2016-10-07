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

#ifndef RenderChangePicture_H
#define RenderChangePicture_H

//
// Define the characteristics
// of this message class here.
//

#undef  MESSAGE_NAME
#define MESSAGE_NAME          "RenderChangePicture"

#undef  MESSAGE_STORE
#define MESSAGE_STORE         RenderChangePictureStore

#undef  MESSAGE_CLASS
#define MESSAGE_CLASS         RenderMinorExtensionStore

#undef  MESSAGE_METHODS
#define MESSAGE_METHODS       "RenderMinorExtensionMethods.h"

#undef  MESSAGE_HEADERS
#define MESSAGE_HEADERS       "RenderMinorExtensionHeaders.h"

#undef  MESSAGE_TAGS
#define MESSAGE_TAGS          "RenderMinorExtensionTags.h"

#undef  MESSAGE_OFFSET
#define MESSAGE_OFFSET        8

#undef  MESSAGE_HAS_SIZE
#define MESSAGE_HAS_SIZE      1

#undef  MESSAGE_HAS_DATA
#define MESSAGE_HAS_DATA      1

#undef  MESSAGE_HAS_FILTER
#define MESSAGE_HAS_FILTER    0

//
// Declare the message class.
//

#include MESSAGE_HEADERS

class MESSAGE_STORE : public MESSAGE_CLASS
{
  public:

  virtual const char *name() const
  {
    return MESSAGE_NAME;
  }

  virtual int identitySize(const unsigned char *buffer,
                               unsigned int size)
  {
    return MESSAGE_OFFSET;
  }

  #include MESSAGE_METHODS
};

#endif /* RenderChangePicture_H */
