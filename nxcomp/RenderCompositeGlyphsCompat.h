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

#ifndef RenderCompositeGlyphsCompat_H
#define RenderCompositeGlyphsCompat_H

//
// Define the characteristics
// of this message class here.
//

#undef  MESSAGE_NAME
#define MESSAGE_NAME          "RenderCompositeGlyphsCompat"

#undef  MESSAGE_STORE
#define MESSAGE_STORE         RenderCompositeGlyphsCompatStore

#undef  MESSAGE_CLASS
#define MESSAGE_CLASS         RenderMinorExtensionStore

#undef  MESSAGE_METHODS
#define MESSAGE_METHODS       "RenderMinorExtensionMethods.h"

#undef  MESSAGE_HEADERS
#define MESSAGE_HEADERS       "RenderMinorExtensionHeaders.h"

#undef  MESSAGE_TAGS
#define MESSAGE_TAGS          "RenderMinorExtensionTags.h"

#undef  MESSAGE_OFFSET
#define MESSAGE_OFFSET        36

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
    return (size >= MESSAGE_OFFSET ? MESSAGE_OFFSET : size);
  }

  #include MESSAGE_METHODS
};

#endif /* RenderCompositeGlyphsCompat_H */
