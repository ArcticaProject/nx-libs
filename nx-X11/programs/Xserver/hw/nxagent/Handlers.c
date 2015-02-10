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

#include "dixstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "osdep.h"

#include "Agent.h"
#include "Handlers.h"
#include "Display.h"
#include "Events.h"
#include "Client.h"
#include "Reconnect.h"
#include "Dialog.h"
#include "Drawable.h"
#include "Splash.h"
#include "Screen.h"
#include "Millis.h"

#include "NXlib.h"
#include "Shadow.h"

/*
 * Set here the required log level.
 */

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG
#undef  DUMP

/*
 * Log begin and end of the important
 * handlers.
 */

#undef  BLOCKS

/*
 * If not defined, flush immediately
 * upon entering the block handler.
 */

#define FLUSH_AFTER_MULTIPLE_READS

/*
 * Introduce a small delay after each
 * loop if the session is down. The
 * value is in milliseconds.
 */

#define LOOP_DELAY_IF_DOWN       50

/*
 * The soft limit should roughly match
 * the size of the Xlib I/O buffer.
 */

#define BYTES_BEFORE_SOFT_TOKEN  2048
#define BYTES_BEFORE_HARD_TOKEN  65536

/*
 * Maximum number of synchronization
 * requests before waiting for the
 * remote.
 */

#define TOKENS_PENDING_LIMIT     8

/*
 * Limits are very unobtrusive. We don't
 * want to interfere with the normal
 * dispatching.
 */

#define BYTES_BEFORE_YIELD       1048576
#define TIME_BEFORE_YIELD        500

/*
 * Dinamically reduce the display buffer
 * size after a congestion.
 */

#undef DYNAMIC_DISPLAY_BUFFER

/*
 * Minimum display buffer size.
 */

#define MINIMUM_DISPLAY_BUFFER   512

#ifdef NX_DEBUG_INPUT
extern int nxagentDebugInputDevices;
extern unsigned long nxagentLastInputDevicesDumpTime;

extern void nxagentDumpInputDevicesState(void);
#endif

/*
 * Used in the handling of the X desktop
 * manager protocol.
 */

int nxagentXdmcpUp = 0;
int nxagentXdmcpAlertUp = 0;

/*
 * Also used in the block, wakeup and
 * sync handlers.
 */

int nxagentBuffer;
int nxagentBlocking;
int nxagentCongestion;

double nxagentBytesIn;
double nxagentBytesOut;

/*
 * Total number of descriptors ready
 * as reported by the wakeup handler.
 */

int nxagentReady;

/*
 * Timestamp of the last write to the
 * remote display.
 */

int nxagentFlush;

/*
 * Arbitrate the bandwidth among our
 * clients.
 */

struct _TokensRec   nxagentTokens   = { 0, 0, 0 };
struct _DispatchRec nxagentDispatch = { UNDEFINED, 0, 0, 0 };

/*
 * Called just before blocking, waiting
 * for our clients or the X server.
 */

extern int nxagentSkipImage;

void nxagentBlockHandler(pointer data, struct timeval **timeout, pointer mask)
{
  /*
   * Zero timeout.
   */

  static struct timeval zero;

  /*
   * Current timestamp.
   */

  static int now;

  /*
   * Pending bytes to write to the
   * network.
   */

  static int flushable;

  /*
   * Set if we need to synchronize
   * any drawable.
   */

  static int synchronize;

  #ifdef BLOCKS
  fprintf(stderr, "[Begin block]\n");
  #endif

  now = GetTimeInMillis();

  #ifdef NX_DEBUG_INPUT

  if (nxagentDebugInputDevices == 1 &&
        now - nxagentLastInputDevicesDumpTime > 5000)
  {
    nxagentLastInputDevicesDumpTime = now;

    nxagentDumpInputDevicesState();
  }

  #endif

  if (nxagentNeedConnectionChange() == 1)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentBlockHandler: Calling nxagentHandleConnectionChanges "
                "with ioError [%d] sigHup [%d].\n", nxagentException.ioError, nxagentException.sigHup);
    #endif

    nxagentHandleConnectionChanges();
  }

  if (nxagentOption(Rootless) &&
          nxagentLastWindowDestroyed && nxagentRootlessDialogPid == 0 &&
              now > nxagentLastWindowDestroyedTime + 30 * 1000 && !nxagentOption(NoRootlessExit))
  {
    #ifdef WARNING
    fprintf(stderr, "nxagentBlockHandler: No application running. Closing the session.\n");
    #endif

    nxagentTerminateSession();
  }

  #ifdef TEST

  if (nxagentLastWindowDestroyed == 1)
  {
    fprintf(stderr, "nxagentBlockHandler: Elapsed time [%lu].\n",
                now - nxagentLastWindowDestroyedTime);
  }

  #endif

  /*
   * Slow down the agent if the session is
   * not connected to a valid display.
   */

  if (NXDisplayError(nxagentDisplay) == 1 && nxagentShadowCounter == 0)
  {
    usleep(LOOP_DELAY_IF_DOWN * 1000);

    now = GetTimeInMillis();
  }

  /*
   * Update the shadow display. This is
   * only for test purposes.
   */

  #ifdef DUMP

  nxagentPixmapOnShadowDisplay(NULL);

  nxagentFbOnShadowDisplay();

  #endif

  /*
   * We need this here because some window
   * configuration changes can be generated
   * by the X server outside the control of
   * the DIX.
   */

  nxagentFlushConfigureWindow();

  /*
   * Check whether there is any drawable to
   * synchronize.
   */

  #ifdef TEST
  fprintf(stderr, "nxagentBlockHandler: Checking synchronization at %s with "
              "[%d][%d][%d].\n", GetTimeInMillisAsString(), nxagentCorruptedWindows,
                  nxagentCorruptedBackgrounds, nxagentCorruptedPixmaps);
  #endif

  synchronize = (nxagentCorruptedWindows > 0 ||
                     nxagentCorruptedBackgrounds > 0 ||
                         nxagentCorruptedPixmaps > 0);

  /*
   * The synchronization function requires a mask as
   * parameter:
   *
   * EVENT_BREAK       Breaks if an user input, like
   *                   a key press or a mouse move,
   *                   is detected.
   *
   * CONGESTION_BREAK  Breaks if the congestion beco-
   *                   mes greater than 4.
   *
   * BLOCKING_BREAK    Breaks if the display descript-
   *                   or becomes blocked for write
   *                   during the loop.
   *
   * ALWAYS_BREAK      Any of the previous conditions
   *                   is met.
   *
   * NEVER_BREAK       The loop continues until all
   *                   the drawables are synchronized.
   */

  if (synchronize == 1)
  {
    /*
     * We should not enter the synchronization
     * loop if there is any user input pending,
     * i.e. if we are in the middle of a scroll
     * operation.
     */

    if (nxagentForceSynchronization == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentBlockHandler: Going to force a synchronization at %s.\n",
                GetTimeInMillisAsString());
      #endif

      nxagentSynchronizationLoop(NEVER_BREAK);

      nxagentForceSynchronization = 0;
    }
    else if (nxagentUserInput(NULL) == 0 &&
                 nxagentBlocking == 0 &&
                     nxagentCongestion <= 4)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentBlockHandler: Going to synchronize at %s.\n",
                GetTimeInMillisAsString());
      #endif

      nxagentSynchronizationLoop(ALWAYS_BREAK);
    }
    #ifdef TEST
    else
    {
      fprintf(stderr, "nxagentBlockHandler: Not synchronizing with [%d][%d].\n",
                  nxagentBlocking, nxagentCongestion);
    }
    #endif

    /*
     * Check if we have more corrupted resources
     * and whether the conditions are satisfied
     * to continue with the synchronization.
     */

    synchronize = (nxagentCongestion <= 4 &&
                       (nxagentCorruptedWindows > 0 ||
                           nxagentCorruptedBackgrounds > 0 ||
                               nxagentCorruptedPixmaps > 0));

    if (nxagentSkipImage == 0 && synchronize == 1)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentBlockHandler: Setting a zero timeout with [%d][%d][%d] and "
                  "congestion [%d].\n", nxagentCorruptedWindows, nxagentCorruptedBackgrounds,
                      nxagentCorruptedPixmaps, nxagentCongestion);
      #endif

      zero.tv_sec  = 0;
      zero.tv_usec = 0;

      *timeout = &zero;
    }
    #ifdef TEST
    else
    {
      if (nxagentCorruptedWindows == 0 &&
              nxagentCorruptedBackgrounds == 0 &&
                  nxagentCorruptedPixmaps == 0)
      {
        fprintf(stderr, "nxagentBlockHandler: Nothing more to synchronize at %s.\n",
                    GetTimeInMillisAsString());
      }
      else
      {
        fprintf(stderr, "nxagentBlockHandler: Delaying synchronization with [%d][%d][%d] and "
                  "congestion [%d].\n", nxagentCorruptedWindows, nxagentCorruptedBackgrounds,
                      nxagentCorruptedPixmaps, nxagentCongestion);
      }
    }
    #endif
  }

  /*
   * If the remote X server is blocking, reduce the
   * amount of data sent in a single display update
   * by reducing the size of the display buffer.
   */

  #ifdef DYNAMIC_DISPLAY_BUFFER

  if (nxagentBlocking == 1 &&
          nxagentBuffer > MINIMUM_DISPLAY_BUFFER)
  {
    nxagentBuffer >>= 1;

    if (nxagentBuffer < MINIMUM_DISPLAY_BUFFER)
    {
      nxagentBuffer = MINIMUM_DISPLAY_BUFFER;
    }

    #ifdef TEST
    fprintf(stderr, "nxagentDispatchHandler: Reducing the display buffer to [%d] bytes.\n",
                nxagentBuffer);
    #endif

    NXSetDisplayBuffer(nxagentDisplay, nxagentBuffer);
  }
  else if (nxagentBuffer < nxagentOption(DisplayBuffer) &&
               nxagentCongestion == 0)
  {
    nxagentBuffer = nxagentOption(DisplayBuffer);

    #ifdef TEST
    fprintf(stderr, "nxagentDispatchHandler: Increasing the display buffer to [%d] bytes.\n",
                nxagentBuffer);
    #endif

    NXSetDisplayBuffer(nxagentDisplay, nxagentBuffer);
  }

  #endif /* #ifdef DYNAMIC_DISPLAY_BUFFER */
  
  /*
   * Dispatch to the clients the events that
   * may have become available.
   */

  if (nxagentPendingEvents(nxagentDisplay) > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentBlockHandler: Reading the events available.\n");
    #endif

    nxagentDispatchEvents(NULL);
  }

  /*
   * Check if there is any data remaining,
   * either in the display buffer or in
   * the NX transport.
   */

  flushable = NXDisplayFlushable(nxagentDisplay);

  if (flushable > 0)
  {
    #ifdef FLUSH_AFTER_MULTIPLE_READS

    /*
     * Flush all the outstanding data if
     * the wakeup handler didn't detect
     * any activity.
     */

    if (nxagentReady == 0 || now - nxagentFlush >=
            nxagentOption(DisplayCoalescence))
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentBlockHandler: Flushing the display with [%d] bytes to flush.\n",
                  flushable);
      #endif

      NXFlushDisplay(nxagentDisplay, NXFlushLink);

      /*
       * New events may have become available
       * after the flush.
       */

      if (nxagentPendingEvents(nxagentDisplay) > 0)
      {
        #ifdef TEST
        fprintf(stderr, "nxagentBlockHandler: Reading the events available.\n");
        #endif

        nxagentDispatchEvents(NULL);
      }
    }
    else
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentBlockHandler: Delaying flush with [%d][%d] and [%d] bytes.\n",
                  synchronize, nxagentReady, flushable);
      #endif

      zero.tv_sec  = 0;
      zero.tv_usec = 0;

      *timeout = &zero;
    }

    #else /* #ifdef FLUSH_AFTER_MULTIPLE_READS */

    /*
     * We are entering the select. Tell the NX
     * transport to write any produced data to
     * the remote end.
     */

    NXFlushDisplay(nxagentDisplay, NXFlushLink);

    if (nxagentPendingEvents(nxagentDisplay) > 0)
    {
      #ifdef TEST
      fprintf(stderr, "nxagentBlockHandler: Reading the events available.\n");
      #endif

      nxagentDispatchEvents(NULL);
    }

    #endif /* #ifdef FLUSH_AFTER_MULTIPLE_READS */
  }
  else
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentBlockHandler: Nothing to flush with [%d][%d].\n",
                synchronize, nxagentReady);
    #endif

    if (NXDisplayError(nxagentDisplay) == 0 &&
            nxagentQueuedEvents(nxagentDisplay) > 0)
    {
      #ifdef WARNING
      fprintf(stderr, "nxagentBlockHandler: WARNING! Forcing a null timeout with events queued.\n");
      #endif

      zero.tv_sec  = 0;
      zero.tv_usec = 0;

      *timeout = &zero;
    }
  }

  /*
   * WaitForSomething() sets a zero timeout if there
   * are clients with input, but doesn't stop the
   * timer. The select is then interrupted to update
   * the schedule time even if, what the dispatcher
   * cares, is only the number of ticks at the time
   * the client is scheduled in.
   */

  #ifdef SMART_SCHEDULE

  #ifdef DEBUG
  fprintf(stderr, "nxagentBlockHandler: Stopping the smart schedule timer.\n");
  #endif

  nxagentStopTimer();

  #endif

  nxagentPrintGeometry();

  #ifdef BLOCKS
  fprintf(stderr, "[End block]\n");
  #endif
}

void nxagentWakeupHandler(pointer data, int count, pointer mask)
{
  #ifdef BLOCKS
  fprintf(stderr, "[Begin wakeup]\n");
  #endif

  if (nxagentException.sigHup || nxagentException.ioError)
  {
    #ifdef TEST
    fprintf(stderr,"nxagentWakeupHandler: Got SIGHUP or I/O error.\n");
    #endif

    #ifdef TEST
    fprintf(stderr, "nxagentBlockHandler: Calling nxagentHandleConnectionStates "
                "with ioError [%d] sigHup [%d].\n", nxagentException.ioError, nxagentException.sigHup);
    #endif

    nxagentHandleConnectionStates();
  }

  #ifdef SMART_SCHEDULE

  if (SmartScheduleDisable == 1)
  {

  #endif

    #ifdef DEBUG
    fprintf(stderr, "nxagentWakeupHandler: Resetting the dispatch state after wakeup.\n");
    #endif

    nxagentDispatch.start = GetTimeInMillis();

    nxagentDispatch.in  = nxagentBytesIn;
    nxagentDispatch.out = nxagentBytesOut;

  #ifdef SMART_SCHEDULE

  }

  #endif

  /*
   * Can become true during the dispatch loop.
   */

  nxagentBlocking = 0;

  /*
   * Check if we got new events.
   */

  if (count > 0 && FD_ISSET(nxagentXConnectionNumber, (fd_set *) mask))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentWakeupHandler: Reading the X events with count [%d].\n",
                count);
    #endif

    nxagentDispatchEvents(NULL);

    #ifdef TEST
    fprintf(stderr, "nxagentWakeupHandler: Removing the X descriptor from the count.\n");
    #endif

    FD_CLR(nxagentXConnectionNumber, (fd_set *) mask);

    count--;
  }
  else if (nxagentQueuedEvents(nxagentDisplay) == 1)
  {
    /*
     * We may have left some events in
     * the queue.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentWakeupHandler: Reading the queued X events with count [%d].\n",
                count);
    #endif

    nxagentDispatchEvents(NULL);
  }

  /*
   * Save the number of descriptors ready.
   */

  if (count <= 0)
  {
    count = (XFD_ANYSET(&ClientsWithInput) ? 1 : 0);
  }

  nxagentReady = count;

  #ifdef TEST

  if (nxagentReady == 0)
  {
    fprintf(stderr, "nxagentWakeupHandler: No X clients found to be processed.\n");
  }

  #endif

  /*
   * If the XDM connection can't be established
   * we'll need to create a dialog to notify the
   * user and give him/her a chance to terminate
   * the session.
   */

  if (nxagentOption(Xdmcp) == 1 && nxagentXdmcpUp == 0)
  {
    #ifdef DEBUG
    fprintf(stderr, "nxagentWakeupHandler: XdmcpState [%d].\n", XdmcpState);
    #endif

    if (XdmcpState == XDM_RUN_SESSION)
    {
      nxagentXdmcpUp = 1;
    }

    if (nxagentXdmcpUp == 0)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentWakeupHandler: XdmcpTime [%lu].\n",
                  GetTimeInMillis() - XdmcpStartTime);
      #endif

      #ifdef DEBUG
      fprintf(stderr, "nxagentWakeupHandler: XdmcpTimeOutRtx [%d].\n",
                  XdmcpTimeOutRtx);
      #endif

      if (nxagentXdmcpAlertUp == 0 &&
              GetTimeInMillis() - XdmcpStartTime >= XDM_TIMEOUT)
      {
        #ifdef WARNING
        fprintf(stderr, "nxagentWakeupHandler: WARNING! The XDM session seems to be "
                    "unable to start after [%ld] ms.\n",(long int)(GetTimeInMillis() - XdmcpStartTime));
        #endif

        NXTransAlert(FAILED_XDMCP_CONNECTION_ALERT, NX_ALERT_REMOTE);

        nxagentXdmcpAlertUp = 1;
      }
    }
  }

  #ifdef BLOCKS
  fprintf(stderr, "[End wakeup]\n");
  #endif
}

void nxagentShadowBlockHandler(pointer data, struct timeval **timeout, pointer mask)
{
  static struct timeval zero;

  int changed;
  int suspended = 0;
  int result;
  int width_, height_;

  #ifdef BLOCKS
  fprintf(stderr, "[Begin block]\n");
  #endif

  if (nxagentNeedConnectionChange() == 1)
  {
    nxagentHandleConnectionChanges();
  }

  if (nxagentSessionState == SESSION_DOWN)
  {
    usleep(50 * 1000);
  }

  #ifndef __CYGWIN32__

  if (nxagentReadEvents(nxagentDisplay) > 0 ||
          nxagentReadEvents(nxagentShadowDisplay) > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentShadowBlockHandler: Reading X events queued.\n");
    #endif

    nxagentDispatchEvents(NULL);
  }

  if (nxagentShadowResize == 1)
  {
    nxagentShadowResize = 0;

    nxagentShadowAdaptToRatio();
  }

  #else

  if (nxagentReadEvents(nxagentDisplay) > 0)
  {
    #ifdef TEST
    fprintf(stderr, "nxagentShadowBlockHandler: Reading X events queued.\n");
    #endif

    nxagentDispatchEvents(NULL);
  }

  #endif

  changed = 0;

  NXShadowGetScreenSize(&width_, &height_);

  if (width_ != nxagentShadowWidth || height_ != nxagentShadowHeight)
  {
    /*
     * The master session has been resized.
     */

    NXShadowSetScreenSize(&nxagentShadowWidth, &nxagentShadowHeight);

    nxagentShadowAdaptToRatio();
  }

  nxagentShadowPoll(nxagentShadowPixmapPtr, nxagentShadowGCPtr, nxagentShadowDepth, nxagentShadowWidth,
                        nxagentShadowHeight, nxagentShadowBuffer, &changed, &suspended);

  result = nxagentShadowSendUpdates(&suspended);

  if (nxagentBlocking == 0)
  {
    nxagentSynchronizeDrawable((DrawablePtr) nxagentShadowPixmapPtr, DONT_WAIT,
                                   ALWAYS_BREAK, nxagentShadowWindowPtr);
  }

  /*
   * We are entering the select. Tell the NX
   * transport to write any produced data to
   * the remote end.
   */
/*
FIXME: Must queue multiple writes and handle
       the events by resembling the ordinary
       block handler.
*/

  NXFlushDisplay(nxagentDisplay, NXFlushLink);

  if (*timeout == NULL)
  {
    *timeout = &zero;
  }

  #ifdef __CYGWIN32__

  usleep(50 * 1000);

  (*timeout) -> tv_sec  = 0;
  (*timeout) -> tv_usec = 50 * 1000;

  #else

  if (changed == 0)
  {
    (*timeout) -> tv_sec  = 0;
    (*timeout) -> tv_usec = 50 * 1000;
  }
  else
  {
    (*timeout) -> tv_sec  = 0;
    (*timeout) -> tv_usec = 0;
  }

  #endif

  nxagentPrintGeometry();

  #ifdef BLOCKS
  fprintf(stderr, "[End block]\n");
  #endif
}

void nxagentShadowWakeupHandler(pointer data, int count, pointer mask)
{
  #ifdef BLOCKS
  fprintf(stderr, "[Begin wakeup]\n");
  #endif

  if (nxagentException.sigHup || nxagentException.ioError)
  {
    #ifdef TEST
    fprintf(stderr,"nxagentShadowWakeupHandler: Got SIGHUP or I/O error.\n");
    #endif

    nxagentHandleConnectionStates();
  }

  #ifdef SMART_SCHEDULE

  if (SmartScheduleDisable == 1)
  {

  #endif

    #ifdef DEBUG
    fprintf(stderr, "nxagentShadowWakeupHandler: Resetting the dispatch state after wakeup.\n");
    #endif

    nxagentDispatch.start = GetTimeInMillis();

    nxagentDispatch.in  = nxagentBytesIn;
    nxagentDispatch.out = nxagentBytesOut;

  #ifdef SMART_SCHEDULE

  }

  #endif

  /*
   * Can become true during the dispatch loop.
   */

  nxagentBlocking = 0;

  /*
   * Check if we got new events.
   */

  if (count > 0 && FD_ISSET(nxagentXConnectionNumber, (fd_set *) mask))
  {
    #ifdef TEST
    fprintf(stderr, "nxagentShadowWakeupHandler: Reading the X events with count [%d].\n",
                count);
    #endif

    nxagentDispatchEvents(NULL);

    #ifdef TEST
    fprintf(stderr, "nxagentShadowWakeupHandler: Removing the X descriptor from the count.\n");
    #endif

    FD_CLR(nxagentXConnectionNumber, (fd_set *) mask);

    count--;
  }
  else if (nxagentQueuedEvents(nxagentDisplay) == 1)
  {
    /*
     * We may have left some events in
     * the queue.
     */

    #ifdef TEST
    fprintf(stderr, "nxagentShadowWakeupHandler: Reading the queued X events with count [%d].\n",
                count);
    #endif

    nxagentDispatchEvents(NULL);
  }

  /*
   * Save the number of descriptors ready.
   */

  if (count <= 0)
  {
    count = (XFD_ANYSET(&ClientsWithInput) ? 1 : 0);
  }

  nxagentReady = count;

  #ifdef TEST

  if (nxagentReady == 0)
  {
    fprintf(stderr, "nxagentShadowWakeupHandler: No X clients found to be processed.\n");
  }

  #endif

  #ifdef BLOCKS
  fprintf(stderr, "[End wakeup]\n");
  #endif
}

void nxagentHandleCollectInputFocusEvent(int resource)
{
  Window window;
  int revert_to;

  if (NXGetCollectedInputFocus(nxagentDisplay, resource, &window, &revert_to) == 0)
  {
    #ifdef PANIC
    fprintf(stderr, "nxagentHandleCollectInputFocusEvent: PANIC! Failed to get the input focus "
                "reply for resource [%d].\n", resource);
    #endif
  }

  #ifdef DEBUG
  fprintf(stderr, "nxagentHandleCollectInputFocusEvent: Received a sync reply with [%d] pending.\n",
              nxagentTokens.pending);
  #endif

  nxagentTokens.pending--;

  nxagentCongestion = (nxagentTokens.pending >= TOKENS_PENDING_LIMIT / 2);

  #ifdef TEST
  fprintf(stderr, "nxagentHandleCollectInputFocusEvent: Current congestion level is [%d].\n",
              nxagentCongestion);
  #endif
}

Bool nxagentCollectInputFocusPredicate(Display *display, XEvent *X, XPointer ptr)
{
  return (X -> xclient.window == 0 &&
             X -> xclient.message_type == 0 &&
                 X -> xclient.format == 32 &&
                     X -> xclient.data.l[0] == NXCollectInputFocusNotify);
}

void nxagentDispatchHandler(ClientPtr client, int in, int out)
{
  /*
   * This function is called by the dispatcher (with 0
   * bytes out) after a new request has been processed.
   * It is also called by the write handler (with 0
   * bytes in) after more data has been written to the
   * display. It may be optionally called in the block
   * and wakeup handlers. In this case both in and out
   * must be 0.
   */

  if (out > 0)
  {
    /*
     * Called by the display write callback.
     */

    #ifdef DEBUG
    fprintf(stderr, "nxagentDispatchHandler: Called with [%d] bytes written.\n",
                out);
    #endif

    nxagentBytesOut += out;

    #ifdef DEBUG
    fprintf(stderr, "nxagentDispatchHandler: Total bytes are [%.0f] in [%.0f] out.\n",
                nxagentBytesIn, nxagentBytesOut);
    #endif

    /*
     * Don't take care of the synchronization if
     * the NX transport is running. The NX trans-
     * port has its own token-based control flow.
     *
     * We can't produce more output here because
     * we are in the middle of the flush. We will
     * take care of the sync requests when called
     * by the dispatcher.
     */

    if (nxagentOption(LinkType) == LINK_TYPE_NONE)
    {
      nxagentTokens.soft += out;

      if (out > BYTES_BEFORE_HARD_TOKEN)
      {
        nxagentTokens.hard += out;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentDispatchHandler: Sync bytes accumulated are [%d] and [%d].\n",
                  nxagentTokens.soft, nxagentTokens.hard);
      #endif
    }

    #ifdef SMART_SCHEDULE

    if (SmartScheduleDisable == 1)
    {

    #endif

      /*
       * Pay attention to the next client if this
       * client produced enough output.
       */

      if  (nxagentBytesOut - nxagentDispatch.out > BYTES_BEFORE_YIELD)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchHandler: Yielding with [%ld][%.0f][%.0f] for client [%d].\n",
                    GetTimeInMillis() - nxagentDispatch.start, nxagentBytesIn - nxagentDispatch.in,
                        nxagentBytesOut - nxagentDispatch.out, nxagentDispatch.client);
        #endif

        nxagentDispatch.start = GetTimeInMillis();

        nxagentDispatch.in  = nxagentBytesIn;
        nxagentDispatch.out = nxagentBytesOut;

        isItTimeToYield = 1;
      }
      #ifdef DEBUG
      else
      {
        fprintf(stderr, "nxagentDispatchHandler: Dispatching with [%ld][%.0f][%.0f] for client [%d].\n",
                    GetTimeInMillis() - nxagentDispatch.start, nxagentBytesIn - nxagentDispatch.in,
                        nxagentBytesOut - nxagentDispatch.out, nxagentDispatch.client);
      }
      #endif

    #ifdef SMART_SCHEDULE

    }

    #endif

    return;
  }
  else if (in > 0)
  {
    /*
     * Called by the dispatcher.
     */

    #ifdef DEBUG
    fprintf(stderr, "nxagentDispatchHandler: Called with [%d] bytes processed for client [%d].\n",
                in, client -> index);
    #endif

    /*
     * This is presently unused.
     *
     * nxagentClientAddBytes(client, in);
     *
     * #ifdef DEBUG
     * fprintf(stderr, "nxagentDispatchHandler: Bytes processed for client [%d] are [%ld].\n",
     *             client -> index, nxagentClientBytes(client));
     * #endif
     *
     */

    nxagentBytesIn += in;

    #ifdef DEBUG
    fprintf(stderr, "nxagentDispatchHandler: Total bytes are [%.0f] in [%.0f] out.\n",
                nxagentBytesIn, nxagentBytesOut);
    #endif

    /*
     * When using the dumb scheduler, before reading from
     * another client, the dispatcher tries to drain all
     * the input from the client being processed. This
     * means that, if isItTimeToYield is never set and the
     * client never produces any output, we'll stick into
     * the inner dispatch loop forever.
     */

    #ifdef SMART_SCHEDULE

    if (SmartScheduleDisable == 1)
    {

    #endif

      if  (client -> index != nxagentDispatch.client)
      {
        #ifdef DEBUG
        fprintf(stderr, "nxagentDispatchHandler: Resetting the dispatch state with [%d][%d].\n",
                    nxagentDispatch.client, client -> index);
        #endif

        nxagentDispatch.client = client -> index;
        nxagentDispatch.start  = GetTimeInMillis();

        nxagentDispatch.in  = nxagentBytesIn;
        nxagentDispatch.out = nxagentBytesOut;
      }
      else
      {
        static unsigned long int now;

        now = GetTimeInMillis();

        if (now - nxagentDispatch.start > TIME_BEFORE_YIELD ||
                nxagentBytesIn - nxagentDispatch.in > BYTES_BEFORE_YIELD)
        {
          #ifdef DEBUG
          fprintf(stderr, "nxagentDispatchHandler: Yielding with [%ld][%.0f][%.0f] for client [%d].\n",
                      now - nxagentDispatch.start, nxagentBytesIn - nxagentDispatch.in, nxagentBytesOut -
                          nxagentDispatch.out, nxagentDispatch.client);
          #endif

          nxagentDispatch.start = now;

          nxagentDispatch.in  = nxagentBytesIn;
          nxagentDispatch.out = nxagentBytesOut;

          isItTimeToYield = 1;
        }
        #ifdef DEBUG
        else
        {
          fprintf(stderr, "nxagentDispatchHandler: Dispatching with [%ld][%.0f][%.0f] for client [%d].\n",
                      now - nxagentDispatch.start, nxagentBytesIn - nxagentDispatch.in, nxagentBytesOut -
                          nxagentDispatch.out, nxagentDispatch.client);
        }
        #endif
      }

    #ifdef SMART_SCHEDULE

    }

    #endif

  }

  /*
   * Let's see if it's time to sync.
   */

  if (nxagentOption(LinkType) == LINK_TYPE_NONE)
  {
    if (nxagentTokens.hard > BYTES_BEFORE_HARD_TOKEN)
    {
      #ifdef DEBUG
      fprintf(stderr, "nxagentDispatchHandler: Requesting a hard sync reply with [%d] bytes.\n",
                  nxagentTokens.hard);
      #endif

      XSync(nxagentDisplay, 0);

      if (nxagentPendingEvents(nxagentDisplay) > 0)
      {
        nxagentDispatchEvents(NULL);
      }

      nxagentTokens.soft  = 0;
      nxagentTokens.hard  = 0;
    }
    else if (nxagentTokens.soft > BYTES_BEFORE_SOFT_TOKEN)
    {
      /*
       * Alternatively, the amounts of bytes
       * accounted for each sync request may
       * be decreased according to the number
       * of pending replies already awaited.
       *
       * else if (nxagentTokens.soft > (BYTES_BEFORE_SOFT_TOKEN / (nxagentTokens.pending + 1)))
       */

      int resource;

      /*
       * Wait eventually for the number of
       * synchronization requests to return
       * below the limit.
       */

      #ifdef TEST

      if (nxagentTokens.pending == TOKENS_PENDING_LIMIT)
      {
        fprintf(stderr, "nxagentDispatchHandler: WARNING! Waiting for the synchronization reply.\n");
      }

      #endif

      while (nxagentTokens.pending == TOKENS_PENDING_LIMIT)
      {
        if (nxagentWaitEvents(nxagentDisplay, NULL) == -1)
        {
          nxagentTokens.pending = 0;

          nxagentTokens.soft = 0;

          return;
        }

        nxagentDispatchEvents(NULL);

        nxagentBlocking = 1;
      }

      /*
       * Send a new synchronization request.
       */

      resource = nxagentWaitForResource(NXGetCollectInputFocusResource,
                                            nxagentCollectInputFocusPredicate);

      if (resource == -1)
      {
        #ifdef PANIC
        fprintf(stderr, "nxagentDispatchHandler: PANIC! Cannot allocate any valid resource.\n");
        #endif

        nxagentTokens.soft = 0;

        return;
      }

      #ifdef DEBUG
      fprintf(stderr, "nxagentDispatchHandler: Requesting a sync reply with [%d] bytes "
                  "and [%d] pending.\n", nxagentTokens.soft, nxagentTokens.pending);
      #endif

      NXCollectInputFocus(nxagentDisplay, resource);

      NXFlushDisplay(nxagentDisplay, NXFlushBuffer);

      if (nxagentPendingEvents(nxagentDisplay) > 0)
      {
        nxagentDispatchEvents(NULL);
      }

      nxagentTokens.pending++;

      nxagentCongestion = (nxagentTokens.pending >= TOKENS_PENDING_LIMIT / 2);

      #ifdef TEST
      fprintf(stderr, "nxagentDispatchHandler: Current congestion level is [%d].\n",
                  nxagentCongestion);
      #endif

      nxagentTokens.soft = 0;
    }
  }

  /*
   * Check if there are events to read.
   */

  if (nxagentPendingEvents(nxagentDisplay) > 0)
  {
    nxagentDispatchEvents(NULL);
  }
}
