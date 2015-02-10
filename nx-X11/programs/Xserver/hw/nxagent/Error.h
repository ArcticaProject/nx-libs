/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXAGENT, NX protocol compression and NX extensions to this software    */
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

#ifndef __Error_H__
#define __Error_H__

/*
 * Clients log file name.
 */

extern char nxagentClientsLogName[];

extern char nxagentVerbose;

int nxagentErrorHandler(Display *dpy, XErrorEvent *event);

int nxagentExitHandler(const char *message);

void nxagentStartRedirectToClientsLog(void);

void nxagentEndRedirectToClientsLog(void);

char *nxagentGetSessionPath(void);

#endif /* __Error_H__ */
