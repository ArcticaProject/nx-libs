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

#include "ChannelCache.h"

const unsigned int CONFIGUREWINDOW_FIELD_WIDTH[7] =
{
  16,     // x
  16,     // y
  16,     // width
  16,     // height
  16,     // border width
  29,     // sibling window
  3       // stack mode
};

const unsigned int CREATEGC_FIELD_WIDTH[23] =
{
  4,      // function
  32,     // plane mask
  32,     // foreground
  32,     // background
  16,     // line width
  2,      // line style
  2,      // cap style
  2,      // join style
  2,      // fill style
  1,      // fill rule
  29,     // tile
  29,     // stipple
  16,     // tile/stipple x origin
  16,     // tile/stipple y origin
  29,     // font
  1,      // subwindow mode
  1,      // graphics exposures
  16,     // clip x origin
  16,     // clip y origin
  29,     // clip mask
  16,     // card offset
  8,      // dashes
  1       // arc mode
};
