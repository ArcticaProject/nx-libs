/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/***********************************************************
Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "Trap.h"

#include "misc.h"

extern int xorg_ProcXvDispatch(ClientPtr);
extern int xorg_SProcXvDispatch(ClientPtr);

#undef  TEST
#undef  DEBUG

/*
** ProcXvDispatch
**
**
**
*/

int
ProcXvDispatch(ClientPtr client)
{
  int result;

  /*
   * Report upstream that we are
   * dispatching a XVideo operation.
   */

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Going to dispatch XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  nxagentXvTrap = 1;

  result = xorg_ProcXvDispatch(client);

  nxagentXvTrap = 0;

  #ifdef TEST
  fprintf(stderr, "ProcXvDispatch: Dispatched XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  return result;
}

int
SProcXvDispatch(ClientPtr client)
{
  int result;

  /*
   * Report upstream that we are
   * dispatching a XVideo operation.
   */

  #ifdef TEST
  fprintf(stderr, "SProcXvDispatch: Going to dispatch XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  nxagentXvTrap = 1;

  result = xorg_SProcXvDispatch(client);

  nxagentXvTrap = 0;

  #ifdef TEST
  fprintf(stderr, "SProcXvDispatch: Dispatched XVideo operation [%d] for client [%d].\n",
              stuff->data, client -> index);
  #endif

  return result;
}
