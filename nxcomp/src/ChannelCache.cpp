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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
