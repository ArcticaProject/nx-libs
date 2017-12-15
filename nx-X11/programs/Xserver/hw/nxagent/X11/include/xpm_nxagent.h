/*
 * Copyright (C) 1989-95 GROUPE BULL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * GROUPE BULL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of GROUPE BULL shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from GROUPE BULL.
 */

/*****************************************************************************\
* xpm.h:                                                                      *
*                                                                             *
*  XPM library                                                                *
*  Include file                                                               *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

/*
 * This file is a reduced version of the header file of
 * <X11/xpm.h>
 *
 * This copy of code has been introduced to allow a clear namespace
 * separation between <X11/...> and <nx-X11/...> header files.
 *
 * This version of the Xpm library header file only contains symbols
 * required by nxagent and strictly avoids indirectly including
 * from an X11 library that is also shipped in nx-X11/lib/.
 *
 * When using <X11/xpm.h> instead for inclusion in nxagent, it will
 * attempt pulling in the <X11/extensions/Xlib.h> header file.
 * However, the headers of the same name from <nx-X11/...> should be
 * used instead.
 *
 * FIXME: Once the nxagent Xserver starts using libX11 from X.Org, this
 * hack can be removed.
 *
 * 2015/06/26, Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 */

#ifndef XPM_h
#define XPM_h

/*
 * first some identification numbers:
 * the version and revision numbers are determined with the following rule:
 * SO Major number = LIB minor version number.
 * SO Minor number = LIB sub-minor version number.
 * e.g: Xpm version 3.2f
 *      we forget the 3 which is the format number, 2 gives 2, and f gives 6.
 *      thus we have XpmVersion = 2 and XpmRevision = 6
 *      which gives  SOXPMLIBREV = 2.6
 *
 * Then the XpmIncludeVersion number is built from these numbers.
 */
#define XpmFormat 3
#define XpmVersion 4
#define XpmRevision 11
#define XpmIncludeVersion ((XpmFormat * 100 + XpmVersion) * 100 + XpmRevision)

#ifndef XPM_NUMBERS

/* let's define Pixel if it is not done yet */
#if ! defined(_XtIntrinsic_h) && ! defined(PIXEL_ALREADY_TYPEDEFED)
typedef unsigned long Pixel;	/* Index into colormap */
# define PIXEL_ALREADY_TYPEDEFED
#endif

/* include headers from <nx-X11/...> instead of <X11/...> */
#include <nx-X11/Xlib.h>
#include <nx-X11/Xutil.h>

/* Return ErrorStatus codes:
 * null     if full success
 * positive if partial success
 * negative if failure
 */

#define XpmColorError    1
#define XpmSuccess       0
#define XpmOpenFailed   -1
#define XpmFileInvalid  -2
#define XpmNoMemory     -3
#define XpmColorFailed  -4

typedef struct {
    char *name;			/* Symbolic color name */
    char *value;		/* Color value */
    Pixel pixel;		/* Color pixel */
}      XpmColorSymbol;

typedef struct {
    char *name;			/* name of the extension */
    unsigned int nlines;	/* number of lines in this extension */
    char **lines;		/* pointer to the extension array of strings */
}      XpmExtension;

typedef struct {
    char *string;		/* characters string */
    char *symbolic;		/* symbolic name */
    char *m_color;		/* monochrom default */
    char *g4_color;		/* 4 level grayscale default */
    char *g_color;		/* other level grayscale default */
    char *c_color;		/* color default */
}      XpmColor;

typedef struct {
    unsigned int width;		/* image width */
    unsigned int height;	/* image height */
    unsigned int cpp;		/* number of characters per pixel */
    unsigned int ncolors;	/* number of colors */
    XpmColor *colorTable;	/* list of related colors */
    unsigned int *data;		/* image data */
}      XpmImage;

typedef struct {
    unsigned long valuemask;	/* Specifies which attributes are defined */
    char *hints_cmt;		/* Comment of the hints section */
    char *colors_cmt;		/* Comment of the colors section */
    char *pixels_cmt;		/* Comment of the pixels section */
    unsigned int x_hotspot;	/* Returns the x hotspot's coordinate */
    unsigned int y_hotspot;	/* Returns the y hotspot's coordinate */
    unsigned int nextensions;	/* number of extensions */
    XpmExtension *extensions;	/* pointer to array of extensions */
}      XpmInfo;

typedef int (*XpmAllocColorFunc)(
    Display*			/* display */,
    Colormap			/* colormap */,
    char*			/* colorname */,
    XColor*			/* xcolor */,
    void*			/* closure */
);

typedef int (*XpmFreeColorsFunc)(
    Display*			/* display */,
    Colormap			/* colormap */,
    Pixel*			/* pixels */,
    int				/* npixels */,
    void*			/* closure */
);


/* required struct for hw/nxagent/Holder.c */
typedef struct {
    unsigned long valuemask;		/* Specifies which attributes are
					   defined */

    Visual *visual;			/* Specifies the visual to use */
    Colormap colormap;			/* Specifies the colormap to use */
    unsigned int depth;			/* Specifies the depth */
    unsigned int width;			/* Returns the width of the created
					   pixmap */
    unsigned int height;		/* Returns the height of the created
					   pixmap */
    unsigned int x_hotspot;		/* Returns the x hotspot's
					   coordinate */
    unsigned int y_hotspot;		/* Returns the y hotspot's
					   coordinate */
    unsigned int cpp;			/* Specifies the number of char per
					   pixel */
    Pixel *pixels;			/* List of used color pixels */
    unsigned int npixels;		/* Number of used pixels */
    XpmColorSymbol *colorsymbols;	/* List of color symbols to override */
    unsigned int numsymbols;		/* Number of symbols */
    char *rgb_fname;			/* RGB text file name */
    unsigned int nextensions;		/* Number of extensions */
    XpmExtension *extensions;		/* List of extensions */

    unsigned int ncolors;               /* Number of colors */
    XpmColor *colorTable;               /* List of colors */
/* 3.2 backward compatibility code */
    char *hints_cmt;                    /* Comment of the hints section */
    char *colors_cmt;                   /* Comment of the colors section */
    char *pixels_cmt;                   /* Comment of the pixels section */
/* end 3.2 bc */
    unsigned int mask_pixel;            /* Color table index of transparent
                                           color */

    /* Color Allocation Directives */
    Bool exactColors;			/* Only use exact colors for visual */
    unsigned int closeness;		/* Allowable RGB deviation */
    unsigned int red_closeness;		/* Allowable red deviation */
    unsigned int green_closeness;	/* Allowable green deviation */
    unsigned int blue_closeness;	/* Allowable blue deviation */
    int color_key;			/* Use colors from this color set */

    Pixel *alloc_pixels;		/* Returns the list of alloc'ed color
					   pixels */
    int nalloc_pixels;			/* Returns the number of alloc'ed
					   color pixels */

    Bool alloc_close_colors;    	/* Specify whether close colors should
					   be allocated using XAllocColor
					   or not */
    int bitmap_format;			/* Specify the format of 1bit depth
					   images: ZPixmap or XYBitmap */

    /* Color functions */
    XpmAllocColorFunc alloc_color;	/* Application color allocator */
    XpmFreeColorsFunc free_colors;	/* Application color de-allocator */
    void *color_closure;		/* Application private data to pass to
					   alloc_color and free_colors */

}      XpmAttributes;

/* XpmAttributes value masks bits */

/* required masks bits for hw/nxagent/Holder.c */
#define XpmDepth	   (1L<<2)
#define XpmSize		   (1L<<3)	/* width & height */

/* macros for forward declarations of functions with prototypes */
#define FUNC(f, t, p) extern t f p
#define LFUNC(f, t, p) static t f p


/*
 * functions declarations (for building nxagent against system wide libXpm4,
 * but also against libNX_X11 (as opposed to system-wide libX11).
 */

_XFUNCPROTOBEGIN

/* Keep for hw/nxagent/Holder.c */
FUNC(XpmCreatePixmapFromData, int, (Display *display,
				    Drawable d,
				    char **data,
				    Pixmap *pixmap_return,
				    Pixmap *shapemask_return,
				    XpmAttributes *attributes));
/* Keep for hw/nxagent/Display.c */
FUNC(XpmReadFileToPixmap, int, (Display *display,
                                Drawable d,
                                const char *filename,
                                Pixmap *pixmap_return,
                                Pixmap *shapemask_return,
                                XpmAttributes *attributes));

_XFUNCPROTOEND

#endif /* XPM_NUMBERS */
#endif
