
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"
#include "globals.h"

static MODULESETUPPROTO(dbeSetup);

extern void DbeExtensionInit(void);

ExtensionModule dbeExt = {
    DbeExtensionInit,
    "DOUBLE-BUFFER",
    &noDbeExtension,
    NULL,
    NULL
};

static XF86ModuleVersionInfo VersRec =
{
	"dbe",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_EXTENSION,
	ABI_EXTENSION_VERSION,
	MOD_CLASS_EXTENSION,
	{0,0,0,0}
};

/*
 * Data for the loader
 */
XF86ModuleData dbeModuleData = { &VersRec, dbeSetup, NULL };

static void *
dbeSetup(void * module, void * opts, int *errmaj, int *errmin)
{
    LoadExtension(&dbeExt, FALSE);

    /* Need a non-NULL return value to indicate success */
    return (void *)1;
}
