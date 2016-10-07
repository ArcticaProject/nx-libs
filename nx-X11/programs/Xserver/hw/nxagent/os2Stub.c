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

/*
 * (c) Copyright 1996 by Sebastien Marineau
 *			<marineau@genie.uottawa.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HOLGER VEIT  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Sebastien Marineau shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Sebastien Marineau.
 *
 */

/* This below implements select() for calls in nxagent. It has been         */
/* somewhat optimized for improved performance, but assumes a few */
/* things so it cannot be used as a general select.                             */

#define I_NEED_OS2_H
#include "Xpoll.h"
#include <stdio.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/time.h>
#define INCL_DOSSEMAPHORES
#define INCL_DOSNPIPES
#define INCL_DOSMISC
#define INCL_DOSMODULEMGR
#undef BOOL
#undef BYTE
#include <os2.h>

HEV hPipeSem;
HMODULE hmod_so32dll;
static int (*os2_tcp_select)(int*,int,int,int,long);
ULONG os2_get_sys_millis();
extern int _files[];

#define MAX_TCP 256
/* These lifted from sys/emx.h. Change if that changes there! */
#define F_SOCKET 0x10000000
#define F_PIPE 0x20000000

struct select_data
{
   fd_set read_copy;
   fd_set write_copy;
   BOOL have_read;
   BOOL have_write;
   int tcp_select_mask[MAX_TCP];
   int tcp_emx_handles[MAX_TCP];
   int tcp_select_copy[MAX_TCP];
   int socket_nread;
   int socket_nwrite;
   int socket_ntotal;
   int pipe_ntotal;
   int pipe_have_write;
   int max_fds;
};

int os2PseudoSelect(int nfds, fd_set *readfds, fd_set *writefds,
        fd_set *exceptfds, struct timeval *timeout)
{
static BOOL FirstTime=TRUE;
static haveTCPIP=TRUE;
ULONG timeout_ms;
ULONG postCount, start_millis,now_millis;
char faildata[16];
struct select_data sd;
BOOL any_ready;
int np,ns, i,ready_handles,n;
APIRET rc;

sd.have_read=FALSE; sd.have_write=FALSE; 
sd.socket_nread=0; sd.socket_nwrite=0; sd.socket_ntotal=0;
sd.max_fds=31; ready_handles=0; any_ready=FALSE;
sd.pipe_ntotal=0; sd.pipe_have_write=FALSE;

if(FirstTime){
   /* First load the so32dll.dll module and get a pointer to the SELECT function */

   if((rc=DosLoadModule(faildata,sizeof(faildata),"SO32DLL",&hmod_so32dll))!=0){
        fprintf(stderr, "Could not load module so32dll.dll, rc = %d. Error note %s\n",rc,faildata);
        haveTCPIP=FALSE;
        }
   if((rc = DosQueryProcAddr(hmod_so32dll, 0, "SELECT", (PPFN)&os2_tcp_select))!=0){
        fprintf(stderr, "Could not query address of SELECT, rc = %d.\n",rc);
        haveTCPIP=FALSE;
        }
   /* Call these a first time to set the semaphore */
    /* rc = DosCreateEventSem(NULL, &hPipeSem, DC_SEM_SHARED, FALSE);
    if(rc) { 
             fprintf(stderr, "Could not create event semaphore, rc=%d\n",rc);
             return(-1);
             }
    rc = DosResetEventSem(hPipeSem, &postCount); */ /* Done in xtrans code for servers*/

fprintf(stderr, "Client select() done first-time stuff, sem handle %d.\n",hPipeSem);

   FirstTime = FALSE;
}

/* Set up the time delay structs */

    if(timeout!=NULL) {
	timeout_ms=timeout->tv_sec*1000+timeout->tv_usec/1000;
	}
    else { timeout_ms=1000000; }  /* This should be large enough... */
    if(timeout_ms>0) start_millis=os2_get_sys_millis();

/* Copy the masks */
    {FD_ZERO(&sd.read_copy);}
    {FD_ZERO(&sd.write_copy);}
    if(readfds!=NULL){ XFD_COPYSET(readfds,&sd.read_copy); sd.have_read=TRUE;}
    if(writefds!=NULL) {XFD_COPYSET(writefds,&sd.write_copy);sd.have_write=TRUE;}

/* And zero the original masks */
    if(sd.have_read){ FD_ZERO(readfds);}
    if(sd.have_write) {FD_ZERO(writefds);}
    if(exceptfds != NULL) {FD_ZERO(exceptfds);}

/* Now we parse the fd_sets passed to select and separate pipe/sockets */
        n = os2_parse_select(&sd,nfds);
        if(n == -1) {
           errno = EBADF;
           return (-1);
           }

/* Now we have three cases: either we have sockets, pipes, or both */
/* We handle all three cases differently to optimize things */

/* Case 1: only pipes! */
        if((sd.pipe_ntotal >0) && (!sd.socket_ntotal)){
            np = os2_check_pipes(&sd,readfds,writefds);
            if(np > 0){
               return (np);
               }
            else if (np == -1) { return(-1); }
            while(!any_ready){
                 rc = DosWaitEventSem(hPipeSem, timeout_ms);
                 /* if(rc) fprintf(stderr,"Sem-wait timeout, rc = %d\n",rc); */
                 if(rc == 640)  {
                     return(0);
                     }
                 if((rc != 0) && (rc != 95)) {errno= EBADF; return(-1);}
                 np = os2_check_pipes(&sd,readfds,writefds);
                 if (np > 0){
                    return(np);
                    }
                 else if (np < 0){ return(-1); }
                 }
          }

/* Case 2: only sockets. Just let the os/2 tcp select do the work */
        if((sd.socket_ntotal > 0) && (!sd.pipe_ntotal)){
                ns = os2_check_sockets(&sd, readfds, writefds, timeout_ms);
                return (ns);
                }

/* Case 3: combination of both */
        if((sd.socket_ntotal > 0) && (sd.pipe_ntotal)){
           np = os2_check_pipes(&sd,readfds,writefds);
              if(np > 0){
                 any_ready=TRUE;
                 ready_handles += np;
                 }
              else if (np == -1) { return(-1); }

           ns = os2_check_sockets(&sd,readfds,writefds, 0);
           if(ns>0){
               ready_handles+=ns;
               any_ready = TRUE;
               }
           else if (ns == -1) {return(-1);}

           while (!any_ready && timeout_ms){

                rc = DosWaitEventSem(hPipeSem, 10L);
                if(rc == 0){
                        np = os2_check_pipes(&sd,readfds,writefds);
                        if(np > 0){
                        ready_handles+=np;
                        any_ready = TRUE;
                        }
                        else if (np == -1) { 
                                return(-1); }
                      }

                 ns = os2_check_sockets(&sd,readfds,writefds,exceptfds, 0);
                 if(ns>0){
                      ready_handles+=ns;
                      any_ready = TRUE;
                     }
                 else if (ns == -1) {return(-1);}

                  if (i%8 == 0) { 
                    now_millis = os2_get_sys_millis();
                    if((now_millis-start_millis) > timeout_ms) timeout_ms = 0;
                    }
                   i++;
                  }
        }

return(ready_handles);
}


ULONG os2_get_sys_millis()
{
   APIRET rc;
   ULONG milli;

   rc = DosQuerySysInfo(14, 14, &milli, sizeof(milli));
   if(rc) {
        fprintf(stderr,"Bad return code querying the millisecond counter! rc=%d\n",rc);
        return(0);
        }
   return(milli);
}

int os2_parse_select(sd,nfds)
struct select_data *sd;
int nfds;
{
   int i;
   APIRET rc;
/* First we determine up to which descriptor we need to check.              */
/* No need to check up to 256 if we don't have to (and usually we dont...)*/
/* Note: stuff here is hardcoded for fd_sets which are int[8] as in EMX!    */

  if(nfds > sd->max_fds){
     for(i=0;i<((FD_SETSIZE+31)/32);i++){
        if(sd->read_copy.fds_bits[i] ||
            sd->write_copy.fds_bits[i])
                        sd->max_fds=(i*32) +32;
        }
     }
   else { sd->max_fds = nfds; }
/* Check if result is greater than specified in select() call */
  if(sd->max_fds > nfds) sd->max_fds = nfds;

  if (sd->have_read)
    {
      for (i = 0; i < sd->max_fds; ++i) {
        if (FD_ISSET (i, &sd->read_copy)){
         if(_files[i] & F_SOCKET)
           {
            sd->tcp_select_mask[sd->socket_ntotal]=_getsockhandle(i);
            sd->tcp_emx_handles[sd->socket_ntotal]=i;
            sd->socket_ntotal++; sd->socket_nread++;
           }
         else if (_files[i] & F_PIPE)
          {
            sd -> pipe_ntotal++;
            /* rc = DosSetNPipeSem((HPIPE)i, (HSEM) hPipeSem, i);
            if(rc) { fprintf(stderr,"Error SETNPIPE rc = %d\n",rc); return -1;} */
          }
        }
      }
    }

  if (sd->have_write)
    {
      for (i = 0; i < sd->max_fds; ++i) {
        if (FD_ISSET (i, &sd->write_copy)){
         if(_files[i] & F_SOCKET)
         {
            sd->tcp_select_mask[sd->socket_ntotal]=_getsockhandle(i);
            sd->tcp_emx_handles[sd->socket_ntotal]=i;
            sd->socket_ntotal++; sd->socket_nwrite++;
         }
         else if (_files[i] & F_PIPE)
          {
            sd -> pipe_ntotal++;
            /* rc = DosSetNPipeSem((HPIPE)i, (HSEM) hPipeSem, i);
            if(rc) { fprintf(stderr,"Error SETNPIPE rc = %d\n",rc); return -1;} */
            sd -> pipe_have_write=TRUE;
          }
        }
      }
    }


return(sd->socket_ntotal);
}


int os2_check_sockets(sd,readfds,writefds)
struct select_data *sd;
fd_set *readfds,*writefds;
{
   int e,i;
   int j,n;
        memcpy(sd->tcp_select_copy,sd->tcp_select_mask,
                sd->socket_ntotal*sizeof(int));
 
        e = os2_tcp_select(sd->tcp_select_copy,sd->socket_nread,
                sd->socket_nwrite, 0, 0);

        if(e == 0) return(e);
/* We have something ready? */
        if(e>0){
            j = 0; n = 0;
            for (i = 0; i < sd->socket_nread; ++i, ++j)
                 if (sd->tcp_select_copy[j] != -1)
                    {
                    FD_SET (sd->tcp_emx_handles[j], readfds);
                    n ++;
                    }
             for (i = 0; i < sd->socket_nwrite; ++i, ++j)
                  if (sd->tcp_select_copy[j] != -1)
                     {
                     FD_SET (sd->tcp_emx_handles[j], writefds);
                     n ++;
                     }
               errno = 0;
               
               return n;
              }
        if(e<0){
           /*Error -- TODO. EBADF is a good choice for now. */
           fprintf(stderr,"Error in server select! e=%d\n",e);
           errno = EBADF;
           return (-1);
           }
 }

/* Check to see if anything is ready on pipes */

int os2_check_pipes(sd,readfds,writefds)
struct select_data *sd;
fd_set *readfds,*writefds;
{
int i,e;
ULONG ulPostCount;
PIPESEMSTATE pipeSemState[128];
APIRET rc;
        e = 0;
        rc = DosResetEventSem(hPipeSem,&ulPostCount);
        rc = DosQueryNPipeSemState((HSEM) hPipeSem, (PPIPESEMSTATE)&pipeSemState, 
                sizeof(pipeSemState));
        if(rc) fprintf(stderr,"SELECT: rc from QueryNPipeSem: %d\n",rc);
        i=0;
        while (pipeSemState[i].fStatus != 0) {
           /*fprintf(stderr,"SELECT: sem entry, stat=%d, flag=%d, key=%d,avail=%d\n",
                pipeSemState[i].fStatus,pipeSemState[i].fFlag,pipeSemState[i].usKey,
                pipeSemState[i].usAvail);  */
           if((pipeSemState[i].fStatus == 1) &&
                    (FD_ISSET(pipeSemState[i].usKey,&sd->read_copy))){
                FD_SET(pipeSemState[i].usKey,readfds);
                e++;
                }
           else if((pipeSemState[i].fStatus == 2)  &&
                    (FD_ISSET(pipeSemState[i].usKey,&sd->write_copy))){
                FD_SET(pipeSemState[i].usKey,writefds);
                e++;
                }
            else if( (pipeSemState[i].fStatus == 3) &&
                ( (FD_ISSET(pipeSemState[i].usKey,&sd->read_copy)) ||
                  (FD_ISSET(pipeSemState[i].usKey,&sd->write_copy)) )){
                errno = EBADF;
                /* fprintf(stderr,"Pipe has closed down, fd=%d\n",pipeSemState[i].usKey); */
                return (-1);
                }
            i++;
            } /* endwhile */
        /*fprintf(stderr,"Done listing pipe sem entries, total %d entries, total ready entries %d\n"i,e);*/
errno = 0;
return(e);
}

