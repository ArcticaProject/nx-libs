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

#ifndef NX_H
#define NX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/uio.h>

#define NX_FD_ANY                      -1

#define NX_MODE_ANY                    -1
#define NX_MODE_CLIENT                  1
#define NX_MODE_SERVER                  2

#define NX_DISPLAY_ANY                  NULL

#define NX_SIGNAL_ANY                  -1
#define NX_SIGNAL_ENABLE                1
#define NX_SIGNAL_DISABLE               2
#define NX_SIGNAL_RAISE                 3
#define NX_SIGNAL_FORWARD               4

#define NX_POLICY_IMMEDIATE             1
#define NX_POLICY_DEFERRED              2

#define NX_ALERT_REMOTE                 0
#define NX_ALERT_LOCAL                  1

#define NX_HANDLER_FLUSH                0
#define NX_HANDLER_STATISTICS           1

#define NX_STATISTICS_PARTIAL           0
#define NX_STATISTICS_TOTAL             1

#define NX_CHANNEL_X11                  0
#define NX_CHANNEL_CUPS                 1
#define NX_CHANNEL_SMB                  2
#define NX_CHANNEL_MEDIA                3
#define NX_CHANNEL_HTTP                 4
#define NX_CHANNEL_FONT                 5
#define NX_CHANNEL_SLAVE                6

#define NX_FILE_SESSION                 0
#define NX_FILE_ERRORS                  1
#define NX_FILE_OPTIONS                 2
#define NX_FILE_STATS                   3

/*
 * The following are the new interfaces to the NX transport. The
 * NX proxy software is now intended to be run as a library of a
 * higher level communication manager (nxssh, nxhttp, nxrtp, etc,
 * not only nxproxy). This is a work-in-progress, so expect these
 * interfaces to change in future. At the present moment, as an
 * example, there is no provision for creating and managing mul-
 * tiple proxy connections.
 */

/*
 * Attach a NX transport to the provided descriptor. This should be
 * done after having created a pair of connected sockets.
 */

extern int NXTransCreate(int fd, int mode, const char *options);

/*
 * Tell the proxy to use the second descriptor as its own end of
 * the internal connection to the NX agent. The NX agent will use
 * the first descriptor. Setting an agent connection will have the
 * effect of disabling further X client connections and, if it is
 * possible, will trigger the use of the memory-to-memory transport.
 */

extern int NXTransAgent(int fd[2]);

/*
 * Prepare the file sets and the timeout for a later execution of
 * the select(). The masks and the timeout must persist across all
 * the calls, so if you don't need any of the values, it is requi-
 * red that you create empty masks and a default timeout. To save
 * a check at each run, all the functions below assume that valid
 * pointers are passed.
 */

extern int NXTransPrepare(int *maxfds, fd_set *readfds,
                              fd_set *writefds, struct timeval *timeout);

/*
 * Call select() to find out the descriptors in the sets having
 * pending data.
 */

extern int NXTransSelect(int *result, int *error, int *maxfds, fd_set *readfds,
                             fd_set *writefds, struct timeval *timeout);

/*
 * Perform the required I/O on all the NX descriptors having pen-
 * ding data. This can include reading and writing to the NX chan-
 * nels, encoding and decoding the proxy data or managing any of
 * the other NX resources.
 */

extern int NXTransExecute(int *result, int *error, int *maxfds, fd_set *readfds,
                              fd_set *writefds, struct timeval *timeout);

/*
 * Run an empty loop, giving to the NX transport a chance to check
 * its descriptors.
 */

extern int NXTransContinue(struct timeval *timeout);

/*
 * Perform I/O on the given descriptors. If memory-to-memory trans-
 * port has been activated and the descriptor is recognized as a
 * valid agent connection, then the functions will read and write
 * the data directly to the proxy buffer, otherwise the correspond-
 * ing network operation will be performed.
 */

extern int NXTransRead(int fd, char *data, int size);
extern int NXTransWrite(int fd, char *data, int size);
extern int NXTransReadable(int fd, int *readable);

extern int NXTransReadVector(int fd, struct iovec *iovdata, int iovsize);
extern int NXTransWriteVector(int fd, struct iovec *iovdata, int iovsize);

extern int NXTransClose(int fd);

/*
 * Return true if the NX transport is running. The fd parameter can
 * be either the local descriptor attached to the NX transport or
 * NX_FD_ANY.
 */

extern int NXTransRunning(int fd);

/*
 * Close down the NX transport and free all the allocated resources.
 * The fd parameter can be either the local descriptor or NX_FD_ANY.
 * This must be explicitly called by the agent before the proxy can
 * start the tear down procedure.
 */

extern int NXTransDestroy(int fd);

/*
 * Tell to the proxy how to handle the standard POSIX signals. For
 * example, given the SIGINT signal, the caller can specify any of
 * the following actions:
 *
 * NX_SIGNAL_ENABLE:  A signal handler will have to be installed by
 *                    the library, so that it can be intercepted by
 *                    the proxy.
 *
 * NX_SIGNAL_DISABLE: The signal will be handled by the caller and,
 *                    eventually, forwarded to the proxy by calling
 *                    NXTransSignal() explicitly.
 *
 * NX_SIGNAL_RAISE:   The signal must be handled now, as if it had
 *                    been delivered by the operating system. This
 *                    function can be called by the agent with the
 *                    purpose of propagating a signal to the proxy.
 *
 * NX_SIGNAL_FORWARD: A signal handler will have to be installed by
 *                    the library but the library will have to call
 *                    the original signal handler when the signal
 *                    is received.
 *
 * As a rule of thumb, agents should let the proxy handle SIGUSR1
 * and SIGUSR2, used for producing the NX protocol statistics, and
 * SIGHUP, used for disconnecting the NX transport.
 *
 * The following signals are blocked by default upon creation of the
 * NX transport:
 *
 * SIGCHLD    These signals should be always put under the control
 * SIGUSR1    of the proxy. If agents are intercepting them, agents
 * SIGUSR2    should later call NXTransSignal(..., NX_SIGNAL_RAISE)
 * SIGHUP     to forward the signal to the proxy. As an alternative
 *            they can specify a NX_SIGNAL_FORWARD action, so they,
 *            in turn, can be notified about the signal. This can
 *            be especially useful for SIGCHLD.
 *
 * SIGINT     These signals should be intercepted by agents. Agents
 * SIGTERM    should ensure that NXTransDestroy() is called before
 *            exiting, to give the proxy a chance to shut down the
 *            NX transport.
 *
 * SIGPIPE    This signal is blocked by the proxy, but not used to
 *            implement any functionality. It can be handled by the
 *            NX agent without affecting the proxy.
 *
 * SIGALRM    This is now used by the proxy and agents should not
 *            redefine it. Agents can use the signal to implement
 *            their own timers but should not interleave calls to
 *            the NX transport and should restore the old handler
 *            when the timeout is raised.
 *
 * SIGVTALRM  These signals are not used but may be used in future
 * SIGWINCH   versions of the library.
 * SIGIO
 * SIGTSTP
 * SIGTTIN
 * SIGTTOU
 *
 * By calling NXTransSignal(..., NX_SIGNAL_DISABLE) nxcomp will res-
 * tore the signal handler that was saved at the time the proxy hand-
 * ler was installed. This means that you should call the function
 * just after the XOpenDisplay() or any other function used to init-
 * ialize the NX transport.
 */
 
extern int NXTransSignal(int signal, int action);

/*
 * Return a value between 0 and 9 indicating the congestion level
 * based on the tokens still available. A value of 9 means that
 * the link is congested and no further data can be sent.
 */

extern int NXTransCongestion(int fd);

/*
 * Let the application to be notified by the proxy when an event oc-
 * curs. The parameter, as set at the time the handler is installed,
 * is passed each time to the callback function. The parameter is
 * presumably the display pointer, given that at the present moment
 * the NX transport doesn't have access to the display structure and
 * so wouldn't be able to determine the display to pass to the call-
 * back function.
 *
 * NX_HANDLER_FLUSH:      The handler function is called when some
 *                        more data has been written to the proxy
 *                        link.
 *
 * The data is the number of bytes written.
 *
 * NX_HANDLER_STATISTICS: This handler is called to let the agent
 *                        include arbitrary data in the transport
 *                        statistics. The parameter, in this case,
 *                        is a pointer to a pointer to a null term-
 *                        inated string. The pointer is set at the
 *                        time the handler is registered. The point-
 *                        ed string will have to be filled by the
 *                        agent with its statistics data.
 *
 * The data can be NX_STATISTICS_PARTIAL or NX_STATISTICS_TOTAL. The
 * agent can refer to the value by using the  NXStatisticsPartial and
 * NXStatisticsTotal constants defined in NXvars.h.
 *
 * Note that these interfaces are used by Xlib and nxcompext. Agents
 * should never call these interfaces directly, but use the nxcompext
 * wrapper.
 */

extern int NXTransHandler(int fd, int type, void (*handler)(void *parameter,
                              int reason), void *parameter);

/*
 * Set the policy to be used by the NX transport to write data to the
 * proxy link:
 *
 * NX_POLICY_IMMEDIATE: When set to immediate, the proxy will try to
 *                      write the data just after having encoded it.
 *
 * NX_POLICY_DEFERRED:  When policy is set to deferred, data will be
 *                      accumulated in a buffer and written to the
 *                      remote proxy when NXTransFlush() is called by
 *                      the agent.
 */

extern int NXTransPolicy(int fd, int type);

/*
 * Query the number of bytes that have been accumulated for a deferred
 * flush.
 */

extern int NXTransFlushable(int fd);

/*
 * Tell to the NX transport to write all the accumulated data to the
 * remote proxy.
 */

extern int NXTransFlush(int fd);

/*
 * Create a new channel of the given type. It returns 1 on success,
 * 0 if the NX transport is not running, or -1 in the case of error.
 * On success, the descriptor provided by the caller can be later
 * used for the subsequent I/O. The type parameter not only tells to
 * the proxy the remote port where the channel has to be connected,
 * but also gives a hint about the type of data that will be carried
 * by the channel, so that the proxy can try to optimize the traffic
 * on the proxy link.
 *
 * NX_CHANNEL_X:     The channel will carry X traffic and it
 *                   will be connected to the remote X display.
 *
 * NX_CHANNEL_CUPS:  The channel will carry CUPS/IPP protocol.
 *
 * NX_CHANNEL_SMB:   The channel will carry SMB/CIFS protocol.
 *
 * NX_CHANNEL_MEDIA: The channel will transport audio or other
 *                   multimedia data.
 *
 * NX_CHANNEL_HTTP:  The channel will carry HTTP protocol.
 *
 * NX_CHANNEL_FONT:  The channel will forward a X font server
 *                   connection.
 *
 * Only a proxy running at the NX server/X client side will be able
 * to create a X, CUPS, SMB, MEDIA and HTTP channel. A proxy running
 * at the NX client/X server side can create font server connections.
 * The channel creation will also fail if the remote end has not been
 * set up to forward the connection.
 *
 * To create a new channel the agent will have to set up a socketpair
 * and pass to the proxy one of the socket descriptors.
 *
 * Example:
 *
 * #include <sys/types.h>
 * #include <sys/socket.h>
 *
 * int fds[2];
 *
 * if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) < 0)
 * {
 *   ...
 * }
 * else
 * {
 *   //
 *   // Use fds[0] locally and let the
 *   // proxy use fds[1].
 *   //
 *
 *   if (NXTransChannel(NX_FD_ANY, fds[1], NX_CHANNEL_X) <= 0)
 *   {
 *     ...
 *   }
 *
 *   //
 *   // The agent can now use fds[0] in
 *   // read(), write() and select()
 *   // system calls.
 *   //
 *
 *   ...
 * }
 *
 * Note that all the I/O on the descriptor should be non-blocking, to
 * give a chance to the NX transport to run in the background and handle
 * the data that will be fed to the agent's side of the socketpair. This
 * will happen automatically, as long as the agent uses the XSelect()
 * version of the select() function (as it is normal whenever performing
 * Xlib I/O). In all the other cases, like presumably in the agent's main
 * loop, the agent will have to loop through NXTransPrepare(), NXTrans-
 * Select() and NXTransExecute() functions explicitly, adding to the sets
 * the descriptors that are awaited by the agent. Please check the imple-
 * mentation of _XSelect() in nx-X11/lib/X11/XlibInt.c for an example.
 */

extern int NXTransChannel(int fd, int channelfd, int type);

/*
 * Return the name of the files used by the proxy for the current session.
 *
 * The type parameter can be:
 *
 * NX_FILE_SESSION:  Usually the file 'session' in the user's session
 *                   directory.
 *
 * NX_FILE_ERRORS:   The file used for the diagnostic output. Usually
 *                   the file 'errors' in the session directory.
 *
 * NX_FILE_OPTIONS:  The file containing the NX options, if any.
 *
 * NX_FILE_STATS:    The file used for the statistics output.
 *
 * The returned string is allocated in static memory. The caller should
 * copy the string upon returning from the function, without freeing the
 * pointer.
 */

extern const char *NXTransFile(int type);

/*
 * Return the time in milliseconds elapsed since the last call to this
 * same function.
 */

extern long NXTransTime(void);

/*
 * Other interfaces to the internal transport functions.
 */

extern int NXTransProxy(int fd, int mode, const char *display);

extern int NXTransClient(const char *display);

extern int NXTransDialog(const char *caption, const char *message,
                           const char *window, const char *type, int local,
                               const char *display);

extern int NXTransAlert(int code, int local);

extern int NXTransWatchdog(int timeout);

extern int NXTransKeeper(int caches, int images, const char *root);

extern void NXTransExit(int code) __attribute__((noreturn));

extern int NXTransParseCommandLine(int argc, const char **argv);
extern int NXTransParseEnvironment(const char *env, int force);

extern void NXTransCleanup(void) __attribute__((noreturn));

/*
 * Cleans up the global and local state
 * (the same way as NXTransCleanup does)
 * but does not exit the process
 * Needed for IOS platform
 */
extern void NXTransCleanupForReconnect(void);

extern const char* NXVersion();
extern int NXMajorVersion();
extern int NXMinorVersion();
extern int NXPatchVersion();
extern int NXMaintenancePatchVersion();

#ifdef __cplusplus
}
#endif

#endif /* NX_H */
