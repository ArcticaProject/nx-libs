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

#include "Misc.h"
#include "Unpack.h"
#include "Alpha.h"

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

int UnpackAlpha(unsigned char method, unsigned char *src_data, int src_size,
                    unsigned char *dst_data, int dst_size)
{
  if (*src_data == 0)
  {
    if (dst_size != src_size - 1)
    {
      #ifdef TEST
      *logofs << "UnpackAlpha: PANIC! Invalid destination size "
              << dst_size << " with source " << src_size
              << ".\n" << logofs_flush;
      #endif

      return -1;
    }

    #ifdef TEST
    *logofs << "UnpackAlpha: Expanding " << src_size - 1
            << " bytes of plain alpha data.\n" << logofs_flush;
    #endif

    memcpy(dst_data, src_data + 1, src_size - 1);

    return 1;
  }

  unsigned int check_size = dst_size;

  int result = ZDecompress(&unpackStream, dst_data, &check_size,
                               src_data + 1, src_size - 1);

  if (result != Z_OK)
  {
    #ifdef PANIC
    *logofs << "UnpackAlpha: PANIC! Failure decompressing alpha data. "
            << "Error is '" << zError(result) << "'.\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Failure decompressing alpha data. "
         << "Error is '" << zError(result) << "'.\n";

    return -1;
  }
  else if (check_size != (unsigned int) dst_size)
  {
    #ifdef PANIC
    *logofs << "UnpackAlpha: PANIC! Size mismatch in alpha data. "
            << "Resulting size is " << check_size << " with "
            << "expected size " << dst_size << ".\n"
            << logofs_flush;
    #endif

    cerr << "Error" << ": Size mismatch in alpha data. "
         << "Resulting size is " << check_size << " with "
         << "expected size " << dst_size << ".\n";

    return -1;
  }

  #ifdef TEST
  *logofs << "UnpackAlpha: Decompressed " << src_size - 1
          << " bytes to " << dst_size << " bytes of alpha data.\n"
          << logofs_flush;
  #endif

  return 1;
}

int UnpackAlpha(T_alpha *alpha, unsigned char *dst_data,
                    int dst_size, int big_endian)
{
  unsigned int count = dst_size >> 2;

  unsigned int i;

  int shift;

  if (count != alpha -> entries)
  {
    #ifdef WARNING
    *logofs << "UnpackAlpha: WARNING! Not applying the alpha with "
            << count << " elements needed and " << alpha -> entries
            << " available.\n" << logofs_flush;
    #endif

    return 0;
  }

  shift = (big_endian == 1 ? 0 : 3);
  
  for (i = 0; i < count; i++)
  {
    *(dst_data + shift) = *(alpha -> data + i);

    dst_data += 4;
  }

  return 1;
}
