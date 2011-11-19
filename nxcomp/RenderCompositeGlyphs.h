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

#ifndef RenderCompositeGlyphs_H
#define RenderCompositeGlyphs_H

//
// Define the characteristics
// of this message class here.
//

#undef  MESSAGE_NAME
#define MESSAGE_NAME          "RenderCompositeGlyphs"

#undef  MESSAGE_STORE
#define MESSAGE_STORE         RenderCompositeGlyphsStore

#undef  MESSAGE_CLASS
#define MESSAGE_CLASS         RenderMinorExtensionStore

#undef  MESSAGE_METHODS
#define MESSAGE_METHODS       "RenderMinorExtensionMethods.h"

#undef  MESSAGE_HEADERS
#define MESSAGE_HEADERS       "RenderMinorExtensionHeaders.h"

#undef  MESSAGE_TAGS
#define MESSAGE_TAGS          "RenderMinorExtensionTags.h"

#undef  MESSAGE_OFFSET
#define MESSAGE_OFFSET        28

#undef  MESSAGE_HAS_SIZE
#define MESSAGE_HAS_SIZE      1

#undef  MESSAGE_HAS_DATA
#define MESSAGE_HAS_DATA      1

#undef  MESSAGE_HAS_FILTER
#define MESSAGE_HAS_FILTER    0

//
// Encode the first 8 bytes of the
// data differentially in newer
// protocol versions.
//

#undef  MESSAGE_OFFSET_IF_PROTO_STEP_8
#define MESSAGE_OFFSET_IF_PROTO_STEP_8  36

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
    unsigned int offset = (control -> isProtoStep8() == 1 ?
                               MESSAGE_OFFSET_IF_PROTO_STEP_8 :
                                   MESSAGE_OFFSET);

    return (size >= offset ? offset : size);
  }

  #include MESSAGE_METHODS
};

#endif /* RenderCompositeGlyphs_H */
