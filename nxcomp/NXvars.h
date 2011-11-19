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

#ifndef NXvars_H
#define NXvars_H

/*
 * This can be included by the proxy or another
 * layer that doesn't use Xlib.
 */

#if !defined(_XLIB_H_) && !defined(_XKBSRV_H_)

#define NeedFunctionPrototypes  1

#define Display  void

#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Display flush policies.
 */

#define NXPolicyImmediate    1
#define NXPolicyDeferred     2

/*
 * Type of flush.
 */

#define NXFlushBuffer        0
#define NXFlushLink          1

/*
 * Type of statistics.
 */

#define NXStatisticsPartial  0
#define NXStatisticsTotal    1

/*
 * Reason why the display is blocking.
 */

#define NXBlockRead          1
#define NXBlockWrite         2

/*
 * Set if the client is interested in ignoring
 * the display error and continue with the exe-
 * cution of the program. By default the usual
 * Xlib behaviour is gotten, and the library
 * will call an exit().
 */

extern int _NXHandleDisplayError;

/*
 * The function below is called whenever Xlib is
 * going to perform an I/O operation. The funct-
 * ion can be redefined to include additional
 * checks aimed at detecting if the display needs
 * to be closed, for example because of an event
 * or a signal mandating the end of the session.
 * In this way the client program can regain the
 * control before Xlib blocks waiting for input
 * from the network.
 */

typedef int (*NXDisplayErrorPredicate)(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* reason */
#endif
);

extern NXDisplayErrorPredicate _NXDisplayErrorFunction;

/*
 * This is called when Xlib is going to block
 * waiting for the display to become readable or
 * writable. The client can use the hook to run
 * any arbitrary operation that may require some
 * time to complete. The user should not try to
 * read or write to the display inside the call-
 * back routine.
 */

typedef void (*NXDisplayBlockHandler)(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* reason */
#endif
);

extern NXDisplayBlockHandler _NXDisplayBlockFunction;

/*
 * Used to notify the program when more data
 * is written to the socket.
 */

typedef void (*NXDisplayWriteHandler)(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* length */
#endif
);

extern NXDisplayWriteHandler _NXDisplayWriteFunction;

/*
 * This callback is used to notify the agent
 * that the proxy link has been flushed.
 */

typedef void (*NXDisplayFlushHandler)(
#if NeedFunctionPrototypes
    Display*            /* display */,
    int                 /* length */
#endif
);

extern NXDisplayFlushHandler _NXDisplayFlushFunction;

/*
 * Used by the NX transport to get an arbitrary
 * string to add to its protocol statistics.
 */

typedef void (*NXDisplayStatisticsHandler)(
#if NeedFunctionPrototypes
    Display*            /* display */,
    char*               /* buffer */,
    int                 /* size */
#endif
);

extern NXDisplayStatisticsHandler _NXDisplayStatisticsFunction;

/*
 * Let users redefine the function printing an
 * error message in the case of a out-of-order
 * sequence number.
 */

typedef void (*NXLostSequenceHandler)(
#if NeedFunctionPrototypes
    Display*            /* display */,
    unsigned long       /* newseq */,
    unsigned long       /* lastseq */,
    unsigned int        /* type */
#endif
);

extern NXLostSequenceHandler _NXLostSequenceFunction;

/*
 * Let the X server run the children processes
 * (as for example the keyboard initialization
 * utilities) by using the native system libra-
 * ries, instead of the libraries shipped with
 * the NX environment. If set, the Popen() in
 * the X server will remove the LD_LIBRARY_PATH
 * setting from the environment before calling
 * the execl() function in the child process.
 */

extern int _NXUnsetLibraryPath;

#ifdef __cplusplus
}
#endif

#endif /* NXvars_H */
