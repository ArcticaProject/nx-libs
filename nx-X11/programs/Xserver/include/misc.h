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

Copyright 1992, 1993 Data General Corporation;
Copyright 1992, 1993 OMRON Corporation  

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that the
above copyright notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting documentation, and that
neither the name OMRON or DATA GENERAL be used in advertising or publicity
pertaining to distribution of the software without specific, written prior
permission of the party whose name is to be used.  Neither OMRON or 
DATA GENERAL make any representation about the suitability of this software
for any purpose.  It is provided "as is" without express or implied warranty.  

OMRON AND DATA GENERAL EACH DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OMRON OR DATA GENERAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.

******************************************************************/
#ifndef MISC_H
#define MISC_H 1
/*
 *  X internal definitions 
 *
 */

extern unsigned long globalSerialNumber;
extern unsigned long serverGeneration;

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>
#include <X11/X.h>
#include <stdint.h>

#ifndef _XTYPEDEF_POINTER
/* Don't let Xdefs.h define 'pointer' */
#define _XTYPEDEF_POINTER       1
#endif /* _XTYPEDEF_POINTER */

/* FIXME: for building this code against Xlib versions older than apprx. 04/2014
 * we still have to define the pointer type via Xdefs.h.
 *
 * The nx-libs code itself does not require the pointer definition.
 *
 */
#undef _XTYPEDEF_POINTER

#include <X11/Xdefs.h>

#ifndef IN_MODULE
#ifndef NULL
#include <stddef.h>
#endif
#endif

#ifndef MAXSCREENS
#define MAXSCREENS	16
#endif
#define MAXCLIENTS	256
#define MAXDITS		1
#define MAXEXTENSIONS	128
#define MAXFORMATS	8
#define MAXVISUALS_PER_SCREEN 50

typedef unsigned long PIXEL;
typedef unsigned long ATOM;


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef _XTYPEDEF_CALLBACKLISTPTR
typedef struct _CallbackList *CallbackListPtr; /* also in dix.h */
#define _XTYPEDEF_CALLBACKLISTPTR
#endif

typedef struct _xReq *xReqPtr;

#include "os.h" 	/* for ALLOCATE_LOCAL and DEALLOCATE_LOCAL */
#ifndef IN_MODULE
#include <nx-X11/Xfuncs.h> /* for bcopy, bzero, and bcmp */
#endif

#define NullBox ((BoxPtr)0)
#define MILLI_PER_MIN (1000 * 60)
#define MILLI_PER_SECOND (1000)

    /* this next is used with None and ParentRelative to tell
       PaintWin() what to use to paint the background. Also used
       in the macro IS_VALID_PIXMAP */

#define USE_BACKGROUND_PIXEL 3
#define USE_BORDER_PIXEL 3


/* byte swap a 32-bit literal */
#define lswapl(x) ((((x) & 0xff) << 24) |\
		   (((x) & 0xff00) << 8) |\
		   (((x) & 0xff0000) >> 8) |\
		   (((x) >> 24) & 0xff))

/* byte swap a short literal */
#define lswaps(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

#undef min
#undef max

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#ifndef IN_MODULE
/* abs() is a function, not a macro; include the file declaring
 * it in case we haven't done that yet.
 */  
#include <stdlib.h>
#endif /* IN_MODULE */
#ifndef Fabs
#define Fabs(a) ((a) > 0.0 ? (a) : -(a))	/* floating absolute value */
#endif
#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))
/* this assumes b > 0 */
#define modulus(a, b, d)    if (((d) = (a) % (b)) < 0) (d) += (b)
/*
 * return the least significant bit in x which is set
 *
 * This works on 1's complement and 2's complement machines.
 * If you care about the extra instruction on 2's complement
 * machines, change to ((x) & (-(x)))
 */
#define lowbit(x) ((x) & (~(x) + 1))

#ifndef IN_MODULE
/* XXX Not for modules */
#include <limits.h>
#if !defined(MAXSHORT) || !defined(MINSHORT) || \
    !defined(MAXINT) || !defined(MININT)
/*
 * Some implementations #define these through <math.h>, so preclude
 * #include'ing it later.
 */

#include <math.h>
#endif
#undef MAXSHORT
#define MAXSHORT SHRT_MAX
#undef MINSHORT
#define MINSHORT SHRT_MIN
#undef MAXINT
#define MAXINT INT_MAX
#undef MININT
#define MININT INT_MIN

#include <assert.h>
#include <ctype.h>
#include <stdio.h>	/* for fopen, etc... */

#endif

/**
 * Calculate the number of bytes needed to hold bits.
 * @param bits The minimum number of bits needed.
 * @return The number of bytes needed to hold bits.
 */
static __inline__ int
bits_to_bytes(const int bits) {
    return ((bits + 7) >> 3);
}
/**
 * Calculate the number of 4-byte units needed to hold the given number of
 * bytes.
 * @param bytes The minimum number of bytes needed.
 * @return The number of 4-byte units needed to hold bytes.
 */
static __inline__ int
bytes_to_int32(const int bytes) {
    return (((bytes) + 3) >> 2);
}

/**
 * Calculate the number of bytes (in multiples of 4) needed to hold bytes.
 * @param bytes The minimum number of bytes needed.
 * @return The closest multiple of 4 that is equal or higher than bytes.
 */
static __inline__ int
pad_to_int32(const int bytes) {
    return (((bytes) + 3) & ~3);
}

/**
 * Compare the two version numbers comprising of major.minor.
 *
 * @return A value less than 0 if a is less than b, 0 if a is equal to b,
 * or a value greater than 0
 */
static inline int
version_compare(uint32_t a_major, uint32_t a_minor,
                uint32_t b_major, uint32_t b_minor)
{
    if (a_major > b_major)
        return 1;
    if (a_major < b_major)
        return -1;
    if (a_minor > b_minor)
        return 1;
    if (a_minor < b_minor)
        return -1;

    return 0;
}

/* some macros to help swap requests, replies, and events */

#define LengthRestB(stuff) \
    ((client->req_len << 2) - sizeof(*stuff))

#define LengthRestS(stuff) \
    ((client->req_len << 1) - (sizeof(*stuff) >> 1))

#define LengthRestL(stuff) \
    (client->req_len - (sizeof(*stuff) >> 2))

#define SwapRestS(stuff) \
    SwapShorts((short *)(stuff + 1), LengthRestS(stuff))

#define SwapRestL(stuff) \
    SwapLongs((CARD32 *)(stuff + 1), LengthRestL(stuff))

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
void __attribute__ ((error("wrong sized variable passed to swap")))
wrong_size(void);
#else
static inline void
wrong_size(void)
{
}
#endif

#if !(defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)))
static inline int
__builtin_constant_p(int x)
{
    return 0;
}
#endif

/* byte swap a 64-bit value */
static inline void
swap_uint64(uint64_t *x)
{
    char n;

    n = ((char *) x)[0];
    ((char *) x)[0] = ((char *) x)[7];
    ((char *) x)[7] = n;

    n = ((char *) x)[1];
    ((char *) x)[1] = ((char *) x)[6];
    ((char *) x)[6] = n;

    n = ((char *) x)[2];
    ((char *) x)[2] = ((char *) x)[5];
    ((char *) x)[5] = n;

    n = ((char *) x)[3];
    ((char *) x)[3] = ((char *) x)[4];
    ((char *) x)[4] = n;
}

#define swapll(x) do { \
	if (sizeof(*(x)) != 8) \
	    wrong_size(); \
                swap_uint64((uint64_t *)(x));   \
    } while (0)

/* byte swap a 32-bit value */
static inline void
swap_uint32(uint32_t * x)
{
    char n = ((char *) x)[0];

    ((char *) x)[0] = ((char *) x)[3];
    ((char *) x)[3] = n;
    n = ((char *) x)[1];
    ((char *) x)[1] = ((char *) x)[2];
    ((char *) x)[2] = n;
}

#define swapl(x) do { \
	if (sizeof(*(x)) != 4) \
	    wrong_size(); \
	if (__builtin_constant_p((uintptr_t)(x) & 3) && ((uintptr_t)(x) & 3) == 0) \
	    *(x) = lswapl(*(x)); \
	else \
	    swap_uint32((uint32_t *)(x)); \
    } while (0)

/* byte swap a 16-bit value */
static inline void
swap_uint16(uint16_t * x)
{
    char n = ((char *) x)[0];

    ((char *) x)[0] = ((char *) x)[1];
    ((char *) x)[1] = n;
}

#define swaps(x) do { \
	if (sizeof(*(x)) != 2) \
	    wrong_size(); \
	if (__builtin_constant_p((uintptr_t)(x) & 1) && ((uintptr_t)(x) & 1) == 0) \
	    *(x) = lswaps(*(x)); \
	else \
	    swap_uint16((uint16_t *)(x)); \
    } while (0)

/* copy 32-bit value from src to dst byteswapping on the way */
#define cpswapl(src, dst) do { \
	if (sizeof((src)) != 4 || sizeof((dst)) != 4) \
	    wrong_size(); \
	(dst) = lswapl((src)); \
    } while (0)

/* copy short from src to dst byteswapping on the way */
#define cpswaps(src, dst) do { \
	if (sizeof((src)) != 2 || sizeof((dst)) != 2) \
	    wrong_size(); \
	(dst) = lswaps((src)); \
    } while (0)

extern void SwapLongs(
    CARD32 *list,
    unsigned long count);

extern void SwapShorts(
    short *list,
    unsigned long count);

extern void MakePredeclaredAtoms(void);

extern int Ones(
    unsigned long /*mask*/);

typedef struct _xPoint *DDXPointPtr;
typedef struct pixman_box16 *BoxPtr;
typedef struct _xEvent *xEventPtr;
typedef struct _xRectangle *xRectanglePtr;
typedef struct _GrabRec *GrabPtr;

/*  typedefs from other places - duplicated here to minimize the amount
 *  of unnecessary junk that one would normally have to include to get
 *  these symbols defined
 */

#ifndef _XTYPEDEF_CHARINFOPTR
typedef struct _CharInfo *CharInfoPtr; /* also in fonts/include/font.h */
#define _XTYPEDEF_CHARINFOPTR
#endif

#endif /* MISC_H */
