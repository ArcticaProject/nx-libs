
#include <string.h>
#include <nx-X11/Xdefs.h>
#include "Trap.h"


/* Hook called from dix/extension.c */

Bool
nxagentHook_IsFaultyRenderExtension(char *name)
{
  /*
   * Hide RENDER if our implementation
   * is faulty.
   */

  return (nxagentRenderTrap && strcmp(name, "RENDER") == 0);
}

