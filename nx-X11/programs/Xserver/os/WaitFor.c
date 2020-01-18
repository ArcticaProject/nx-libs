/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* nx-X11, NX protocol compression and NX extensions to this software     */
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

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
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

/*****************************************************************
 * OS Dependent input routines:
 *
 *  WaitForSomething
 *  TimerForce, TimerSet, TimerCheck, TimerFree
 *
 *****************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <nx-X11/Xos.h>			/* for strings, fcntl, time */
#include <errno.h>
#include <stdio.h>
#include <nx-X11/X.h>
#include "misc.h"

#include "osdep.h"
#include <nx-X11/Xpoll.h>
#include "dixstruct.h"
#include "opaque.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP)

static unsigned long startTimeInMillis;

#endif

/* This is just a fallback to errno to hide the differences between unix and
   Windows in the code */
#define GetErrno() errno

/* modifications by raphael */
int
mffs(fd_mask mask)
{
    int i;

    if (!mask) return 0;
    i = 1;
    while (!(mask & 1))
    {
	i++;
	mask >>= 1;
    }
    return i;
}

#ifdef DPMSExtension
#define DPMS_SERVER
#include <nx-X11/extensions/dpms.h>
#endif

struct _OsTimerRec {
    OsTimerPtr		next;
    CARD32		expires;
    CARD32              delta;
    OsTimerCallback	callback;
    void *		arg;
};

static void DoTimer(OsTimerPtr timer, CARD32 now, OsTimerPtr *prev);
static void CheckAllTimers(void);
static OsTimerPtr timers = NULL;

/*****************
 * WaitForSomething:
 *     Make the server suspend until there is
 *	1. data from clients or
 *	2. input events available or
 *	3. ddx notices something of interest (graphics
 *	   queue ready, etc.) or
 *	4. clients that have buffered replies/events are ready
 *
 *     If the time between INPUT events is
 *     greater than ScreenSaverTime, the display is turned off (or
 *     saved, depending on the hardware).  So, WaitForSomething()
 *     has to handle this also (that's why the select() has a timeout.
 *     For more info on ClientsWithInput, see ReadRequestFromClient().
 *     pClientsReady is an array to store ready client->index values into.
 *****************/

int
WaitForSomething(int *pClientsReady)
{
    int i;
    struct timeval waittime, *wt;
    INT32 timeout = 0;
    fd_set clientsReadable;
    fd_set clientsWritable;
    int curclient;
    int selecterr;
    int nready;
    CARD32 now = 0;
    Bool    someReady = FALSE;
    Bool    someNotifyWriteReady = FALSE;

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
    fprintf(stderr, "WaitForSomething: Got called.\n");
#endif

    FD_ZERO(&clientsReadable);

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP)

    startTimeInMillis = GetTimeInMillis();

#endif

    /* We need a while loop here to handle 
       crashed connections and the screen saver timeout */
    while (1)
    {
	/* deal with any blocked jobs */
	if (workQueue)
	    ProcessWorkQueue();
	if (XFD_ANYSET (&ClientsWithInput))
	{
	    someReady = TRUE;
	    waittime.tv_sec = 0;
	    waittime.tv_usec = 0;
	    wt = &waittime;
	}
	if (someReady)
	{
	    XFD_COPYSET(&AllSockets, &LastSelectMask);
	    XFD_UNSET(&LastSelectMask, &ClientsWithInput);
	}
	else
	{
        wt = NULL;
	if (timers)
        {
            now = GetTimeInMillis();
	    timeout = timers->expires - now;
            if (timeout > 0 && timeout > timers->delta + 250) {
                /* time has rewound.  reset the timers. */
                CheckAllTimers();
            }

	    if (timers) {
		timeout = timers->expires - now;
		if (timeout < 0)
		    timeout = 0;
		waittime.tv_sec = timeout / MILLI_PER_SECOND;
		waittime.tv_usec = (timeout % MILLI_PER_SECOND) *
				   (1000000 / MILLI_PER_SECOND);
		wt = &waittime;
	    }
	}
	XFD_COPYSET(&AllSockets, &LastSelectMask);
	}
	SmartScheduleStopTimer ();

	BlockHandler((void *)&wt, (void *)&LastSelectMask);
	if (NewOutputPending)
	    FlushAllOutput();

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP)

        /*
         * If caller has marked the first element of pClientsReady[],
         * bail out of select after a short timeout. We need this to
         * let the NX agent remove the splash screen when the timeout
         * is expired. A better option would be to use the existing
         * screen-saver timeout but it can be modified by clients, so
         * we would need a special handling. This hack is trivial and
         * keeps WaitForSomething() backward compatible with the exis-
         * ting servers.
         */

        if (pClientsReady[0] == -1)
        {
            unsigned long timeoutInMillis;

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP) && defined(NX_TRANS_DEBUG)
            fprintf(stderr, "WaitForSomething: pClientsReady[0] is [%d], pClientsReady[1] is [%d].\n",
                        pClientsReady[0], pClientsReady[1]);
#endif

            timeoutInMillis = GetTimeInMillis();

            if (timeoutInMillis - startTimeInMillis >= NX_TRANS_WAKEUP)
            {
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP) && defined(NX_TRANS_DEBUG)
                fprintf(stderr, "WaitForSomething: Returning 0 because of wakeup timeout.\n");
#endif
                return 0;
            }

            timeoutInMillis = NX_TRANS_WAKEUP - (timeoutInMillis - startTimeInMillis);

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP) && defined(NX_TRANS_DEBUG)
            fprintf(stderr, "WaitForSomething: Milliseconds to next wakeup are %ld.\n",
                        timeoutInMillis);
#endif
            if (wt == NULL || (wt -> tv_sec * MILLI_PER_SECOND +
                    wt -> tv_usec / MILLI_PER_SECOND) > timeoutInMillis)
            {
                if ((waittime.tv_sec * MILLI_PER_SECOND +
                        waittime.tv_usec / MILLI_PER_SECOND) > timeoutInMillis)
                {
                    waittime.tv_sec = timeoutInMillis / MILLI_PER_SECOND;
                    waittime.tv_usec = (timeoutInMillis * MILLI_PER_SECOND) %
                                            (MILLI_PER_SECOND * 1000);
                    wt = &waittime;
                }

#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP) && defined(NX_TRANS_DEBUG)
                fprintf(stderr, "WaitForSomething: Next wakeup timeout set to %ld milliseconds.\n",
                            (waittime.tv_sec * MILLI_PER_SECOND) +
                                (waittime.tv_usec / MILLI_PER_SECOND));
#endif
            }
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_WAKEUP) && defined(NX_TRANS_DEBUG)
            else
            {
                fprintf(stderr, "WaitForSomething: Using existing timeout of %ld milliseconds.\n",
                            (waittime.tv_sec * MILLI_PER_SECOND) +
                                (waittime.tv_usec / MILLI_PER_SECOND));
            }
#endif
        }
#endif

	/* keep this check close to select() call to minimize race */
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
	if (dispatchException)
	{
	    i = -1;

            fprintf(stderr, "WaitForSomething: Value of dispatchException is true. Set i = -1.\n");
	}
#else
        if (dispatchException)
            i = -1;
#endif
	else if (AnyWritesPending)
	{
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
            if (wt == NULL)
            {
                fprintf(stderr, "WaitForSomething: Executing select with LastSelectMask and "
                            "clientsWritable and null timeout.\n");
            }
            else
            {
                fprintf(stderr, "WaitForSomething: Executing select with LastSelectMask, "
                            "clientsWritable, %ld secs and %ld usecs.\n",
                                wt -> tv_sec, wt -> tv_usec);
            }
#endif
	     XFD_COPYSET(&ClientsWriteBlocked, &LastSelectWriteMask);
	     XFD_ORSET(&LastSelectWriteMask, &NotifyWriteFds, &LastSelectWriteMask);
	     i = Select(MaxClients, &LastSelectMask, &LastSelectWriteMask, NULL, wt);
	}
	else 
	{
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
            if (wt == NULL)
            {
                fprintf(stderr, "WaitForSomething: Executing select with LastSelectMask and null timeout.\n");
            }
            else
            {
                fprintf(stderr, "WaitForSomething: Executing select with LastSelectMask, %ld secs and %ld usecs.\n",
                            wt -> tv_sec, wt -> tv_usec);
            }
#endif
	    i = Select (MaxClients, &LastSelectMask, NULL, NULL, wt);
	}
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
        fprintf(stderr, "WaitForSomething: Bailed out with i = [%d] and errno = [%d].\n", i, errno);

        if (i < 0)
        {
            fprintf(stderr, "WaitForSomething: Error is [%s].\n", strerror(errno));
        }
#endif
	selecterr = GetErrno();
	WakeupHandler(i, (void *)&LastSelectMask);
	SmartScheduleStartTimer ();

	if (i <= 0) /* An error or timeout occurred */
	{
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
            if (dispatchException)
            {
                fprintf(stderr, "WaitForSomething: Returning 0 because of (dispatchException).\n");
                return 0;
            }
#else
            if (dispatchException)
                return 0;
#endif
	    if (i < 0) 
	    {
		if (selecterr == EBADF)    /* Some client disconnected */
		{
		    CheckConnections ();
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
                    if (! XFD_ANYSET (&AllClients))
                    {
                        fprintf(stderr, "WaitForSomething: Returning 0 because of (! XFD_ANYSET (&AllClients)).\n");
                        return 0;
                    }
#else
                    if (! XFD_ANYSET (&AllClients))
                        return 0;
#endif
		}
		else if (selecterr == EINVAL)
		{
		    FatalError("WaitForSomething(): select: errno=%d\n",
			selecterr);
            }
		else if (selecterr != EINTR)
		{
		    ErrorF("WaitForSomething(): select: errno=%d\n",
			selecterr);
		}
	    }
	    else if (someReady)
	    {
		/*
		 * If no-one else is home, bail quickly
		 */
		XFD_COPYSET(&ClientsWithInput, &LastSelectMask);
		XFD_COPYSET(&ClientsWithInput, &clientsReadable);
		break;
	    }
#if defined(NX_TRANS_SOCKET)
            if (*checkForInput[0] != *checkForInput[1])
            {
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
                fprintf(stderr, "WaitForSomething: Returning 0 because of (*checkForInput[0] != *checkForInput[1]).\n");
#endif
		return 0;
            }
#else
	    if (*checkForInput[0] != *checkForInput[1])
		return 0;
#endif

	    if (timers)
	    {
                int expired = 0;
		now = GetTimeInMillis();
		if ((int) (timers->expires - now) <= 0)
		    expired = 1;

		while (timers && (int) (timers->expires - now) <= 0)
		    DoTimer(timers, now, &timers);

                if (expired)
                    return 0;
	    }
	}
	else
	{
	    fd_set tmp_set;

	    if (*checkForInput[0] == *checkForInput[1]) {
	        if (timers)
	        {
                    int expired = 0;
		    now = GetTimeInMillis();
		    if ((int) (timers->expires - now) <= 0)
		        expired = 1;

		    while (timers && (int) (timers->expires - now) <= 0)
		        DoTimer(timers, now, &timers);

                    if (expired)
                        return 0;
	        }
	    }
	    if (someReady)
		XFD_ORSET(&LastSelectMask, &ClientsWithInput, &LastSelectMask);
	    if (AnyWritesPending) {
		XFD_ANDSET(&clientsWritable, &LastSelectWriteMask, &ClientsWriteBlocked);
		    if (XFD_ANYSET(&clientsWritable)) {
		    NewOutputPending = TRUE;
		    XFD_ORSET(&OutputPending, &clientsWritable, &OutputPending);
		    XFD_UNSET(&ClientsWriteBlocked, &clientsWritable);
		    if (!XFD_ANYSET(&ClientsWriteBlocked) && NumNotifyWriteFd == 0)
			AnyWritesPending = FALSE;
		}
		if (NumNotifyWriteFd != 0) {
		    XFD_ANDSET(&tmp_set, &LastSelectWriteMask, &NotifyWriteFds);
		    if (XFD_ANYSET(&tmp_set))
			someNotifyWriteReady = TRUE;
		}
	    }

	    XFD_ANDSET(&clientsReadable, &LastSelectMask, &AllClients); 

	    XFD_ANDSET(&tmp_set, &LastSelectMask, &NotifyReadFds);
	    if (XFD_ANYSET(&tmp_set) || someNotifyWriteReady)
	         HandleNotifyFds();

	    if (XFD_ANYSET (&clientsReadable))
		break;
	}
    }

    nready = 0;
    if (XFD_ANYSET (&clientsReadable))
    {
	for (i=0; i<howmany(XFD_SETSIZE, NFDBITS); i++)
	{
	    while (clientsReadable.fds_bits[i])
	    {
		int client_index;

		curclient = ffs (clientsReadable.fds_bits[i]) - 1;
		client_index = /* raphael: modified */
			ConnectionTranslation[curclient + (i * (sizeof(fd_mask) * 8))];
	    pClientsReady[nready++] = client_index;
	    clientsReadable.fds_bits[i] &= ~(((fd_mask)1L) << curclient);
	}
	}
    }
#if defined(NX_TRANS_SOCKET) && defined(NX_TRANS_DEBUG)
    fprintf(stderr, "WaitForSomething: Returning nready.\n");
#endif
    return nready;
}

#if 0
/*
 * This is not always a macro.
 */
ANYSET(FdMask *src)
{
    int i;

    for (i=0; i<mskcnt; i++)
	if (src[ i ])
	    return (TRUE);
    return (FALSE);
}
#endif

/* If time has rewound, re-run every affected timer.
 * Timers might drop out of the list, so we have to restart every time. */
static void
CheckAllTimers(void)
{
    OsTimerPtr timer;
    CARD32 now;

start:
    now = GetTimeInMillis();

    for (timer = timers; timer; timer = timer->next) {
        if (timer->expires - now > timer->delta + 250) {
            TimerForce(timer);
            goto start;
        }
    }
}

static void
DoTimer(OsTimerPtr timer, CARD32 now, OsTimerPtr *prev)
{
    CARD32 newTime;

    *prev = timer->next;
    timer->next = NULL;
    newTime = (*timer->callback)(timer, now, timer->arg);
    if (newTime)
	TimerSet(timer, 0, newTime, timer->callback, timer->arg);
}

OsTimerPtr
TimerSet(OsTimerPtr timer, int flags, CARD32 millis, 
    OsTimerCallback func, void * arg)
{
    register OsTimerPtr *prev;
    CARD32 now = GetTimeInMillis();

    if (!timer)
    {
	timer = (OsTimerPtr)malloc(sizeof(struct _OsTimerRec));
	if (!timer)
	    return NULL;
    }
    else
    {
	for (prev = &timers; *prev; prev = &(*prev)->next)
	{
	    if (*prev == timer)
	    {
		*prev = timer->next;
		if (flags & TimerForceOld)
		    (void)(*timer->callback)(timer, now, timer->arg);
		break;
	    }
	}
    }
    if (!millis)
	return timer;
    if (flags & TimerAbsolute) {
        timer->delta = millis - now;
    }
    else {
        timer->delta = millis;
	millis += now;
    }
    timer->expires = millis;
    timer->callback = func;
    timer->arg = arg;
    if ((int) (millis - now) <= 0)
    {
	timer->next = NULL;
	millis = (*timer->callback)(timer, now, timer->arg);
	if (!millis)
	    return timer;
    }
    for (prev = &timers;
	 *prev && (int) ((*prev)->expires - millis) <= 0;
	 prev = &(*prev)->next)
        ;
    timer->next = *prev;
    *prev = timer;
    return timer;
}

Bool
TimerForce(OsTimerPtr timer)
{
    OsTimerPtr *prev;

    for (prev = &timers; *prev; prev = &(*prev)->next)
    {
	if (*prev == timer)
	{
	    DoTimer(timer, GetTimeInMillis(), prev);
	    return TRUE;
	}
    }
    return FALSE;
}


void
TimerCancel(OsTimerPtr timer)
{
    OsTimerPtr *prev;

    if (!timer)
	return;
    for (prev = &timers; *prev; prev = &(*prev)->next)
    {
	if (*prev == timer)
	{
	    *prev = timer->next;
	    break;
	}
    }
}

void
TimerFree(OsTimerPtr timer)
{
    if (!timer)
	return;
    TimerCancel(timer);
    free(timer);
}

void
TimerCheck(void)
{
    CARD32 now = GetTimeInMillis();

    while (timers && (int) (timers->expires - now) <= 0)
	DoTimer(timers, now, &timers);
}

void
TimerInit(void)
{
    OsTimerPtr timer;

    while ((timer = timers))
    {
	timers = timer->next;
	free(timer);
    }
}

#ifdef DPMSExtension

#define DPMS_CHECK_MODE(mode,time)\
    if (time > 0 && DPMSPowerLevel < mode && timeout >= time)\
       DPMSSet(mode);

#define DPMS_CHECK_TIMEOUT(time)\
    if (time > 0 && (time - timeout) > 0)\
       return time - timeout;

static CARD32
NextDPMSTimeout(INT32 timeout)
{
    /*
     * Return the amount of time remaining until we should set
     * the next power level. Fallthroughs are intentional.
     */
    switch (DPMSPowerLevel)
    {
       case DPMSModeOn:
           DPMS_CHECK_TIMEOUT(DPMSStandbyTime)

       case DPMSModeStandby:
           DPMS_CHECK_TIMEOUT(DPMSSuspendTime)

       case DPMSModeSuspend:
           DPMS_CHECK_TIMEOUT(DPMSOffTime)

       default: /* DPMSModeOff */
           return 0;
    }
}
#endif /* DPMSExtension */

static CARD32
ScreenSaverTimeoutExpire(OsTimerPtr timer,CARD32 now,void * arg)
{
    INT32 timeout      = now - lastDeviceEventTime.milliseconds;
    CARD32 nextTimeout = 0;

#ifdef DPMSExtension
    /*
     * Check each mode lowest to highest, since a lower mode can
     * have the same timeout as a higher one.
     */
    if (DPMSEnabled)
    {
       DPMS_CHECK_MODE(DPMSModeOff,     DPMSOffTime)
       DPMS_CHECK_MODE(DPMSModeSuspend, DPMSSuspendTime)
       DPMS_CHECK_MODE(DPMSModeStandby, DPMSStandbyTime)

       nextTimeout = NextDPMSTimeout(timeout);
    }

    /*
     * Only do the screensaver checks if we're not in a DPMS
     * power saving mode
     */
    if (DPMSPowerLevel != DPMSModeOn)
       return nextTimeout;
#endif /* DPMSExtension */

    if (!ScreenSaverTime)
       return nextTimeout;

    if (timeout < ScreenSaverTime)
    {
       return nextTimeout > 0 ?
               min(ScreenSaverTime - timeout, nextTimeout) :
               ScreenSaverTime - timeout;
    }

    ResetOsBuffers(); /* not ideal, but better than nothing */
    SaveScreens(SCREEN_SAVER_ON, ScreenSaverActive);

    if (ScreenSaverInterval > 0)
    {
       nextTimeout = nextTimeout > 0 ?
               min(ScreenSaverInterval, nextTimeout) :
               ScreenSaverInterval;
    }

    return nextTimeout;
}

static OsTimerPtr ScreenSaverTimer = NULL;

void
FreeScreenSaverTimer(void)
{
    if (ScreenSaverTimer) {
	TimerFree(ScreenSaverTimer);
	ScreenSaverTimer = NULL;
    }
}

void
SetScreenSaverTimer(void)
{
    CARD32 timeout = 0;

#ifdef DPMSExtension
    if (DPMSEnabled)
    {
       /*
        * A higher DPMS level has a timeout that's either less
        * than or equal to that of a lower DPMS level.
        */
       if (DPMSStandbyTime > 0)
           timeout = DPMSStandbyTime;

       else if (DPMSSuspendTime > 0)
           timeout = DPMSSuspendTime;

       else if (DPMSOffTime > 0)
           timeout = DPMSOffTime;
    }
#endif

    if (ScreenSaverTime > 0)
    {
       timeout = timeout > 0 ?
               min(ScreenSaverTime, timeout) :
               ScreenSaverTime;
    }

#ifdef SCREENSAVER
    if (timeout && !screenSaverSuspended) {
#else
    if (timeout) {
#endif
       ScreenSaverTimer = TimerSet(ScreenSaverTimer, 0, timeout,
                                   ScreenSaverTimeoutExpire, NULL);
    }
    else if (ScreenSaverTimer) {
       FreeScreenSaverTimer();
    }
}
