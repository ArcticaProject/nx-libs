/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMPSHAD, NX protocol compression and NX extensions to this software */
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

#ifndef Shadow_H
#define Shadow_H

#include <X11/Xlib.h>

#define NXShadowCorrectColor(length, buffer) \
\
{ \
  unsigned short a; \
  unsigned short b; \
  unsigned short *shorts; \
  int i; \
\
  length >>= 1; \
  shorts = (unsigned short *)buffer; \
  for (i = 0; i < length ; i++) \
  { \
    a = shorts[i]; \
\
    b = a & 63; \
    a <<= 1; \
    a = (a & ~127) | b; \
\
    shorts[i] = a; \
  } \
}

#ifdef __cplusplus
extern "C" {
#endif

typedef char* UpdaterHandle;

typedef struct _ShadowOptions
{
  char  optionShmExtension;
  char  optionDamageExtension;
  int   optionShadowDisplayUid;
} ShadowOptions;

extern ShadowOptions NXShadowOptions;

extern int           NXShadowCreate(void *, char *, char *, void **);
extern void          NXShadowDestroy(void);

/*
 * Use an already opened Display connection.
 * We use <void *> instead of <Display *> to avoid
 * useless dependences from Xlib headers.
 */

extern int NXShadowAddUpdaterDisplay(void *display, int *width, int *height,
                                         unsigned char *depth);
extern UpdaterHandle NXShadowAddUpdater(char *displayName);
extern int           NXShadowRemoveUpdater(UpdaterHandle handle);
extern int           NXShadowRemoveAllUpdaters(void);

extern void          NXShadowHandleInput(void);
extern int           NXShadowHasChanged(int (*)(void *), void *, int *);
extern void          NXShadowExportChanges(long *, char **);
extern int           NXShadowHasUpdaters(void);
extern int           NXShadowCaptureCursor(unsigned int wnd, void *vis);
extern void          NXShadowColorCorrect(int, int, unsigned int, unsigned int, char *);
extern void          NXShadowUpdateBuffer(void **);

extern void          NXShadowEvent(Display *, XEvent);
extern void          NXShadowWebKeyEvent(KeySym keysym, Bool isKeyPress);

extern void          NXShadowSetDisplayUid(int uid);

extern void          NXShadowDisableShm(void);
extern void          NXShadowDisableDamage(void);

extern void          NXShadowGetScreenSize(int *width, int *height);
extern void          NXShadowSetScreenSize(int *width, int *height);

extern void          NXShadowInitKeymap(void *keysyms);

#ifdef __cplusplus
}
#endif

#endif /* Shadow_H */

