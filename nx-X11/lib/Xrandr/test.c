/* $XFree86$ */

#include <stdio.h>
#include <X11/Xlib.h>
#include "Xrandr.h"

main (int argc, char **argv)

{
  char *display_name = ":0";
  Display *display;
  int major, minor, status;
  
  if ((display = XOpenDisplay (display_name)) == NULL) {
    fprintf(stderr, "Can't open display!\n");
  }
  status = XRRQueryVersion (display, &major, &minor);
  fprintf(stderr, "status = %d, major = %d, minor = %d\n, 
	status, major, minor");

}
