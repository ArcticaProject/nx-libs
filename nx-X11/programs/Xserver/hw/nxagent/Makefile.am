AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = nxagent

SRCS =  Agent.h \
	Args.c \
	Args.h \
	Atoms.c \
	Atoms.h \
	Binder.c \
	Binder.h \
	Client.c \
	Client.h \
	Clipboard.c \
	Clipboard.h \
	Colormap.c \
	Colormap.h \
	Composite.c \
	Composite.h \
	Cursor.c \
	Cursor.h \
	Dialog.c \
	Dialog.h \
	Display.c \
	Display.h \
	Drawable.c \
	Drawable.h \
	Error.c \
	Error.h \
	Events.h \
	Font.c \
	Font.h \
	GC.c \
	GCOps.c \
	GCOps.h \
	GCs.h \
	Handlers.c \
	Handlers.h \
	Holder.c \
	Holder.h \
	Icons.h \
	Image.c \
	Image.h \
	Init.c \
	Init.h \
	Literals.h \
	Millis.c \
	Millis.h \
	NXcomposite.c \
	NXcompositeext.h \
	NXcomposite.h \
	NXcompositeint.h \
	NXcompositeproto.h \
	NXdamage.c \
	NXdixfonts.c \
	NXevents.c \
	NXextension.c \
	NXglyph.c \
	NXglyphcurs.c \
	NXglyphstr.h \
	NXmiglyph.c \
	NXmitrap.c \
	NXmiwindow.c \
	NXpicture.c \
	NXpicturestr.h \
	NXproperty.c \
	NXrender.c \
	NXresource.c \
	NXshm.c \
	NXxrandr.c \
	NXxrandr.h \
	NXxrandrint.h \
	NXxvdisp.c \
	Options.c \
	Options.h \
	Pixels.c \
	Pixels.h \
	Pixmap.c \
	Pixmaps.h \
	Pointer.c \
	Pointer.h \
	Reconnect.c \
	Reconnect.h \
	Render.h \
	Rootless.c \
	Rootless.h \
	Screen.h \
	Splash.c \
	Splash.h \
	Split.c \
	Split.h \
	Trap.c \
	Trap.h \
	Utils.h \
	Visual.c \
	Visual.h \
	Window.c \
	Windows.h \
	Extensions.c \
	Extensions.h \
	Keyboard.c \
	Keyboard.h \
	Keystroke.c \
	Keystroke.h \
	NXdispatch.c \
	Events.c \
	NXglxext.c \
	NXwindow.c \
	Render.c \
	Screen.c \
	nxagent-config.h \
	$(top_srcdir)/Xi/stubs.c \
	$(top_srcdir)/mi/miinitext.c \
	$(top_srcdir)/fb/fbcmap.c


nxagent_SOURCES = $(SRCS)

nxagent_LDADD = $(XORG_CORE_LIBS) \
		$(NXAGENT_LIBS) \
		$(NXAGENTMODULES_LIBS)

# don't warn during linking when a symbol is defined more than once.
# This is a fundamental principle in nxagent: Replace upstream functions 
# with own functions. 
nxagent_LDFLAGS = -Wl,-zmuldefs

### NXAGENT Defines:
#
# NXAGENT_SHAPE                    Old shape code
# NXAGENT_SHAPE2                   New shape code
# NXAGENT_FIXKEYS                  Force the release of pressed key when loosing focus
# NXAGENT_CLIPBOARD                Enables clipboard cut and paste function between X servers.
#
# these are not referenced in the code:
# NXAGENT_CONSTRAINCURSOR
# NXAGENT_EXPOSURES                Manage expose events
# NXAGENT_FONTCACHE_SIZE           Number of cache slots
# NXAGENT_FONTEXCLUDE              Exclude some specific font names (only "-ult1mo" at this moment).
# NXAGENT FULLSCREEN               Fullscreen mode
# NXAGENT_GLYPHCACHE
# NXAGENT_GLYPHCACHE_SIZE          Slots for glyph cache
# NXAGENT_PACKEDIMAGES
# NXAGENT_VISIBILITY

#FIXME: move these to configure.ac
#FIXME: the 5 -U can probably be removed
AM_CFLAGS = -DHAVE_NXAGENT_CONFIG_H \
            -UXF86VIDMODE -UXFreeXDGA -UXF86MISC -UXF86DRI -UXFree86LOADER \
            -DNXAGENT_SERVER \
            -DNXAGENT_SHAPE2 \
            -DNXAGENT_FIXKEYS \
            -DNXAGENT_CLIPBOARD \
            -DNXAGENT_WAKEUP=1000 \
            -DNXAGENT_ONSTART \
            -DNXAGENT_SPLASH \
            -DNXAGENT_ARTSD \
            -DNXAGENT_UPGRADE \
            -UNX_DEBUG_INPUT \
            -DRANDR_10_INTERFACE \
            -DRANDR_12_INTERFACE \
            -UPANORAMIX \
            -UDEBUG_TREE \
            $(NXAGENTMODULES_CFLAGS) \
            -I../../GL/include \
            -I$(top_srcdir)/include

EXTRA_DIST = os2Stub.c \
             nxagent.xpm \
             nxmissing.xpm


# Man page
include $(top_srcdir)/cpprules.in

appmandir = $(APP_MAN_DIR)

appman_PRE =
appman_DATA = $(appman_PRE:man=@APP_MAN_SUFFIX@)

EXTRAMANDEFS = \
	-D__XCONFIGFILE__=$(__XCONFIGFILE__) \
	-D__XSERVERNAME__=$(XSERVERNAME)

BUILT_SOURCES = $(appman_PRE)
CLEANFILES = $(appman_PRE) $(appman_DATA)

SUFFIXES += .$(APP_MAN_SUFFIX) .man

.man.$(APP_MAN_SUFFIX):
	-rm -f $@
	$(LN_S) $< $@
