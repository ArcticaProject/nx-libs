# src/glw/Makefile

TOP = ../..
include $(TOP)/configs/current

MAJOR = 1
MINOR = 0
TINY = 0

INCDIRS = -I$(TOP)/include -I/usr/include/Motif1.2 $(X11_INCLUDES)


OBJECTS = $(GLW_SOURCES:.c=.o)



##### RULES #####

.c.o:
	$(CC) -c $(INCDIRS) $(CFLAGS) $<



##### TARGETS #####

default: $(TOP)/$(LIB_DIR)/$(GLW_LIB_NAME)

# GLU pkg-config file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@LIB_DIR@,$(LIB_DIR),' \
	-e 's,@VERSION@,$(MAJOR).$(MINOR).$(TINY),'
glw.pc: glw.pc.in
	$(pcedit) $< > $@

install: glw.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 *.h $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) $(TOP)/$(LIB_DIR)/libGLw.* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 glw.pc $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig

clean:
	-rm depend depend.bak
	-rm -f *.o *~


# Make the library
$(TOP)/$(LIB_DIR)/$(GLW_LIB_NAME): $(OBJECTS)
	$(TOP)/bin/mklib -o $(GLW_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MAJOR) -minor $(MINOR) -patch $(TINY) \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(GLW_LIB_DEPS) $(OBJECTS)


#
# Run 'make depend' to update the dependencies if you change what's included
# by any source file.
# 
depend: $(GLW_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(GLW_SOURCES) \
		> /dev/null 


include depend
