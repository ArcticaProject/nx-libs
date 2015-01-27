#!/bin/bash

# Copyright (C) 2011 by Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
# Copyright (C) 2012 by Reinhard Tartler <siretart@tauware.de>
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

# Thanks to Jonas Smedegaard <dr@jones.dk> for inspiration...

# Formatting/cleanups by siretart in December 2012

set -e

usage() {
    echo "To be called at the project root of an nx-libs checkout"
    echo "usage: $(basename "$0") {<release-version>,HEAD} {server|client}"
    exit 1
}

PROJECT="nx-libs"
NULL=""

test -d .git || usage
test -f debian/Makefile.nx-libs || usage
RELEASE="$1"
test -n "${RELEASE}" || usage
CHECKOUT="$2"
test -n "$CHECKOUT" || usage

if [ "x$CHECKOUT" = "xserver" ] || [ "x$CHECKOUT" = "xfull" ]; then
	MODE="full"
	CHECKOUT="redist-server/${RELEASE}"
	RELEASE_SUFFIX='-full'
elif [ "x$CHECKOUT" = "xclient" ] || [ "x$CHECKOUT" = "xlite" ]; then
	MODE="lite"
	CHECKOUT="redist-client/${RELEASE}"
	RELEASE_SUFFIX='-lite'
else
	usage
fi

if [ x"$RELEASE" == "xHEAD" ]; then
    CHECKOUT=HEAD
fi

if ! git rev-parse --verify -q "$CHECKOUT" >/dev/null; then
    echo "   '${RELEASE}' is not a valid release number because there is no git tag named $CHECKOUT."
    echo "   Please specify one of the following releases:"
    echo "HEAD"
    git tag -l | grep  ^redist | cut -f2 -d/ | sort -u
    exit 1
fi

TARGETDIR=".."

MANIFEST="$(mktemp)"
TEMP_DIR="$(mktemp -d)"

trap "rm -f \"${MANIFEST}\"; rm -rf \"${TEMP_DIR}\"" 0

# create local copy of Git project at temp location
git archive --format=tar ${CHECKOUT} --prefix=${PROJECT}-${RELEASE}/ | ( cd $TEMP_DIR; tar xf - )

echo "Created tarball for $CHECKOUT"

cd "$TEMP_DIR/${PROJECT}-${RELEASE}/"

mkdir -p doc/applied-patches

# prepare patches for lite and full tarball
if [ "x$MODE" = "xfull" ]; then
    find debian/patches  | sort | egrep "(debian/patches/[0-9]+_.*\.(full|full\+lite)\.patch)" | while read file
    do
        cp -v $file doc/applied-patches
        echo ${file##*/} >> doc/applied-patches/series
    done
    cp -v debian/rgb ./
    cp -v debian/VERSION ./VERSION.x2goagent
else
    rm -Rf "nxcompshad"*
    rm -Rf "nxcompext"*
    rm -Rf "nx-X11"*
    find debian/patches | sort | egrep "(debian/patches/[0-9]+_.*\.full\+lite\.patch)" | while read file
    do
        cp -v $file doc/applied-patches
        echo ${file##*/} >> doc/applied-patches/series
    done
fi
cp -v debian/VERSION ./nxcomp/VERSION
cp -v debian/COPYING.full+lite COPYING

# apply all patches shipped in debian/patches and create a copy of them that we ship with the tarball
if [ -s "doc/applied-patches/series" ]; then
    QUILT_PATCHES=doc/applied-patches quilt --quiltrc /dev/null push -a -q
else
    echo "No patches applied at all. Very old release?"
fi

# very old release did not add any README
for f in $(ls README* 2>/dev/null); do
    mv -v $f doc/;
done

mkdir -p bin/
if [ "$MODE" = "lite" ]; then
    # copy wrapper script nxproxy only into tarball
    cp -v debian/wrappers/nxproxy bin/
else
    # copy wrapper scripts into tarball
    for w in $(ls debian/wrappers/* 2>/dev/null); do
        cp -v $w bin/
    done
    # provide a default keystrokes.cfg file
    mkdir -p etc
    test -f etc/keystrokes.cfg || test -f debian/keystrokes.cfg && cp -v debian/keystrokes.cfg etc/keystrokes.cfg
fi

mv -v debian/changelog doc/changelog

# copy the top-level makefile if no quilt patch created it before
test -f Makefile || test -f debian/Makefile.nx-libs && cp -v debian/Makefile.nx-libs Makefile
test -f replace.sh || test -f debian/Makefile.replace.sh && cp -v debian/Makefile.replace.sh replace.sh


# remove folders that we do not want to roll into the tarball
rm -Rf ".pc/"
rm -Rf "debian/"

#### bundled libraries and other stuff we do not need
UNUSED_FOLDERS="
                 nx-X11/config/docbook/ \
                 nx-X11/config/pswrap/ \
                 nx-X11/config/util/ \
                 nx-X11/extras/Mesa/bin/ \
                 nx-X11/extras/Mesa/config/ \
                 nx-X11/extras/Mesa/docs/ \
                 nx-X11/extras/Mesa/vms/ \
                 nx-X11/extras/Mesa/windows/ \
                 nx-X11/extras/Mesa/src/glw/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/fb/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/ffb/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/gamma/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/i810/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/i830/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/i915/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/mach64/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/mga/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/r128/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/r200/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/r300/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/radeon/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/s3v/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/savage/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/sis/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/tdfx/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/trident/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/unichrome/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/dri/x11/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/fbdev/ \
                 nx-X11/extras/Mesa/src/mesa/drivers/glide/ \
                 nx-X11/extras/drm/ \
                 nx-X11/extras/expat/ \
                 nx-X11/extras/fontconfig/ \
                 nx-X11/extras/freetype2/ \
                 nx-X11/extras/fonts/ \
                 nx-X11/extras/ogl-sample/ \
                 nx-X11/extras/regex/ \
                 nx-X11/extras/rman/ \
                 nx-X11/extras/ttf2pt1/ \
                 nx-X11/extras/x86emu/ \
                 nx-X11/extras/zlib/ \
                 nx-X11/lib/expat/ \
                 nx-X11/lib/fontconfig/ \
                 nx-X11/lib/fontenc/ \
                 nx-X11/lib/font/FreeType/ \
                 nx-X11/lib/font/include/fontenc.h \
                 nx-X11/lib/freetype2/ \
                 nx-X11/lib/regex/ \
                 nx-X11/lib/zlib/ \
                 nx-X11/lib/FS/ \
                 nx-X11/lib/ICE/ \
                 nx-X11/lib/SM/ \
                 nx-X11/lib/Xaw/ \
                 nx-X11/lib/Xft/ \
                 nx-X11/lib/Xt/ \
                 nx-X11/lib/Xmu/ \
                 nx-X11/lib/Xmuu/ \
                 nx-X11/nls/ \
                 nx-X11/programs/Xserver/afb/ \
                 nx-X11/programs/Xserver/cfb/ \
                 nx-X11/programs/Xserver/cfb16/ \
                 nx-X11/programs/Xserver/cfb24/ \
                 nx-X11/programs/Xserver/cfb32/ \
                 nx-X11/programs/Xserver/hw/darwin/ \
                 nx-X11/programs/Xserver/hw/dmx/ \
                 nx-X11/programs/Xserver/hw/kdrive/ \
                 nx-X11/programs/Xserver/hw/sun/ \
                 nx-X11/programs/Xserver/hw/sunLynx/ \
                 nx-X11/programs/Xserver/hw/vfb/ \
                 nx-X11/programs/Xserver/hw/xnest/ \
                 nx-X11/programs/Xserver/hw/xwin/ \
                 nx-X11/programs/Xserver/hw/xfree86/ \
                 nx-X11/programs/Xserver/hw/xfree86/ \
                 nx-X11/programs/Xserver/ilbm/ \
                 nx-X11/programs/Xserver/iplan2p2/ \
                 nx-X11/programs/Xserver/iplan2p4/ \
                 nx-X11/programs/Xserver/iplan2p8/ \
                 nx-X11/programs/Xserver/lbx/ \
                 nx-X11/programs/Xserver/mfb/ \
                 nx-X11/programs/Xserver/miext/layer/ \
                 nx-X11/programs/Xserver/miext/shadow/ \
                 nx-X11/programs/Xserver/XpConfig/ \
                 nx-X11/programs/Xserver/Xprint/ \
                 nx-X11/programs/xterm/ \
                 nx-X11/util/ \
                 ${NULL}
"

# folders to go away completely, but may get recreated by PRESERVE_SYMLINKED_FILES section below
CLEANUP_FOLDERS="
                  nx-X11/config/cf/ \
                  nx-X11/extras/Mesa/ \
                  nx-X11/extras/Xpm/ \
                  ${NULL}
"

# files that are symlinked into the nxagent Xserver, that we do need
PRESERVE_SYMLINKED_FILES="
                           nx-X11/config/cf/sunLib.tmpl.X.original
                           nx-X11/config/cf/Amoeba.cf
                           nx-X11/config/cf/sequent.cf
                           nx-X11/config/cf/cde.rules
                           nx-X11/config/cf/osfLib.rules
                           nx-X11/config/cf/mingw.rules
                           nx-X11/config/cf/X11.rules
                           nx-X11/config/cf/sunLib.tmpl
                           nx-X11/config/cf/cygwin.cf
                           nx-X11/config/cf/scoLib.rules
                           nx-X11/config/cf/os2def.db
                           nx-X11/config/cf/darwin.cf
                           nx-X11/config/cf/OpenBSDLib.tmpl
                           nx-X11/config/cf/oldlib.rules
                           nx-X11/config/cf/os2.rules
                           nx-X11/config/cf/pegasus.cf
                           nx-X11/config/cf/lnxLib.rules
                           nx-X11/config/cf/Win32.rules
                           nx-X11/config/cf/sco5.cf
                           nx-X11/config/cf/mingw.cf
                           nx-X11/config/cf/WinLib.tmpl
                           nx-X11/config/cf/apollo.cf
                           nx-X11/config/cf/convex.cf
                           nx-X11/config/cf/bsdLib.tmpl
                           nx-X11/config/cf/svr4.cf.X.original
                           nx-X11/config/cf/noop.rules
                           nx-X11/config/cf/dmx.cf
                           nx-X11/config/cf/sv3Lib.tmpl
                           nx-X11/config/cf/ibmLib.rules
                           nx-X11/config/cf/sv4Lib.rules
                           nx-X11/config/cf/hpLib.tmpl
                           nx-X11/config/cf/bsd.cf
                           nx-X11/config/cf/Motif.tmpl
                           nx-X11/config/cf/gnuLib.tmpl
                           nx-X11/config/cf/necLib.rules
                           nx-X11/config/cf/xorgsite.def
                           nx-X11/config/cf/QNX4.rules
                           nx-X11/config/cf/lynx.cf
                           nx-X11/config/cf/osf1.cf
                           nx-X11/config/cf/xf86.tmpl
                           nx-X11/config/cf/svr3.cf
                           nx-X11/config/cf/linux.cf
                           nx-X11/config/cf/minix.cf
                           nx-X11/config/cf/hp.cf
                           nx-X11/config/cf/QNX4.cf
                           nx-X11/config/cf/sgi.cf
                           nx-X11/config/cf/xf86.rules
                           nx-X11/config/cf/Imake.tmpl
                           nx-X11/config/cf/xprint_host.def
                           nx-X11/config/cf/xf86site.def
                           nx-X11/config/cf/ncr.cf
                           nx-X11/config/cf/sony.cf
                           nx-X11/config/cf/gnuLib.rules
                           nx-X11/config/cf/sun.cf.X.original
                           nx-X11/config/cf/OpenBSDLib.rules
                           nx-X11/config/cf/darwinLib.tmpl
                           nx-X11/config/cf/sequentLib.rules
                           nx-X11/config/cf/sv4Lib.tmpl
                           nx-X11/config/cf/hpLib.rules
                           nx-X11/config/cf/darwinLib.rules
                           nx-X11/config/cf/bsdiLib.tmpl
                           nx-X11/config/cf/host.def
                           nx-X11/config/cf/iPAQH3600.cf.NX.original
                           nx-X11/config/cf/Threads.tmpl
                           nx-X11/config/cf/nto.cf
                           nx-X11/config/cf/cygwin.tmpl
                           nx-X11/config/cf/sco.cf
                           nx-X11/config/cf/svr4.cf
                           nx-X11/config/cf/ServerLib.tmpl
                           nx-X11/config/cf/usl.cf
                           nx-X11/config/cf/sun.cf.NX.original
                           nx-X11/config/cf/host.def.NX.original
                           nx-X11/config/cf/sgiLib.tmpl
                           nx-X11/config/cf/cross.def.NX.original
                           nx-X11/config/cf/iPAQH3600.cf.X.original
                           nx-X11/config/cf/mingw.tmpl
                           nx-X11/config/cf/xorgversion.def
                           nx-X11/config/cf/sunLib.rules
                           nx-X11/config/cf/lnxLib.tmpl
                           nx-X11/config/cf/xfree86.cf
                           nx-X11/config/cf/sgiLib.rules
                           nx-X11/config/cf/ultrix.cf
                           nx-X11/config/cf/bsdiLib.rules
                           nx-X11/config/cf/ibm.cf
                           nx-X11/config/cf/cygwin.rules
                           nx-X11/config/cf/cross.def
                           nx-X11/config/cf/Win32.cf
                           nx-X11/config/cf/site.def
                           nx-X11/config/cf/os2.cf
                           nx-X11/config/cf/gnu.cf
                           nx-X11/config/cf/cross.rules
                           nx-X11/config/cf/nec.cf
                           nx-X11/config/cf/Library.tmpl
                           nx-X11/config/cf/OpenBSD.cf
                           nx-X11/config/cf/Server.tmpl
                           nx-X11/config/cf/fujitsu.cf
                           nx-X11/config/cf/os2Lib.tmpl
                           nx-X11/config/cf/Oki.cf
                           nx-X11/config/cf/README
                           nx-X11/config/cf/FreeBSD.cf
                           nx-X11/config/cf/site.sample
                           nx-X11/config/cf/bsdLib.rules
                           nx-X11/config/cf/Imake.cf
                           nx-X11/config/cf/cde.tmpl
                           nx-X11/config/cf/Motif.rules
                           nx-X11/config/cf/DragonFly.cf
                           nx-X11/config/cf/Mips.cf
                           nx-X11/config/cf/lnxdoc.rules
                           nx-X11/config/cf/necLib.tmpl
                           nx-X11/config/cf/lnxdoc.tmpl
                           nx-X11/config/cf/cross.def.X.original
                           nx-X11/config/cf/sunLib.tmpl.NX.original
                           nx-X11/config/cf/os2Lib.rules
                           nx-X11/config/cf/NetBSD.cf
                           nx-X11/config/cf/host.def.X.original
                           nx-X11/config/cf/moto.cf
                           nx-X11/config/cf/sv3Lib.rules
                           nx-X11/config/cf/bsdi.cf
                           nx-X11/config/cf/xorg.cf
                           nx-X11/config/cf/svr4.cf.NX.original
                           nx-X11/config/cf/DGUX.cf
                           nx-X11/config/cf/osfLib.tmpl
                           nx-X11/config/cf/x386.cf
                           nx-X11/config/cf/iPAQH3600.cf
                           nx-X11/config/cf/Imake.rules
                           nx-X11/config/cf/X11.tmpl
                           nx-X11/config/cf/luna.cf
                           nx-X11/config/cf/mach.cf
                           nx-X11/config/cf/xorg.tmpl
                           nx-X11/config/cf/ibmLib.tmpl
                           nx-X11/config/cf/isc.cf
                           nx-X11/config/cf/generic.cf
                           nx-X11/config/cf/sun.cf
                           nx-X11/config/cf/macII.cf
                           nx-X11/config/cf/cray.cf
                           nx-X11/config/cf/Imakefile
                           nx-X11/config/cf/nto.rules
                           nx-X11/extras/Mesa/include/GL/glext.h \
                           nx-X11/extras/Mesa/include/GL/gl.h \
                           nx-X11/extras/Mesa/include/GL/glxext.h \
                           nx-X11/extras/Mesa/include/GL/internal/glcore.h \
                           nx-X11/extras/Mesa/include/GL/osmesa.h \
                           nx-X11/extras/Mesa/include/GL/xmesa.h \
                           nx-X11/extras/Mesa/include/GL/xmesa_xf86.h \
                           nx-X11/extras/Mesa/src/glx/x11/compsize.c \
                           nx-X11/extras/Mesa/src/glx/x11/indirect_size.c \
                           nx-X11/extras/Mesa/src/glx/x11/indirect_size.h \
                           nx-X11/extras/Mesa/src/mesa/array_cache/acache.h \
                           nx-X11/extras/Mesa/src/mesa/array_cache/ac_context.c \
                           nx-X11/extras/Mesa/src/mesa/array_cache/ac_context.h \
                           nx-X11/extras/Mesa/src/mesa/array_cache/ac_import.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/common/driverfuncs.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/common/driverfuncs.h \
                           nx-X11/extras/Mesa/src/mesa/drivers/dri/common/glcontextmodes.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/dri/common/glcontextmodes.h \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/glxheader.h \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xm_api.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xm_buffer.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xm_dd.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xm_line.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xm_span.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xm_tri.c \
                           nx-X11/extras/Mesa/src/mesa/drivers/x11/xmesaP.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/dispatch.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/glapi.c \
                           nx-X11/extras/Mesa/src/mesa/glapi/glapi.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/glapitable.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/glapitemp.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/glapioffsets.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/glprocs.h \
                           nx-X11/extras/Mesa/src/mesa/glapi/glthread.c \
                           nx-X11/extras/Mesa/src/mesa/glapi/glthread.h \
                           nx-X11/extras/Mesa/src/mesa/main/accum.c \
                           nx-X11/extras/Mesa/src/mesa/main/accum.h \
                           nx-X11/extras/Mesa/src/mesa/main/api_arrayelt.c \
                           nx-X11/extras/Mesa/src/mesa/main/api_arrayelt.h \
                           nx-X11/extras/Mesa/src/mesa/main/api_eval.h \
                           nx-X11/extras/Mesa/src/mesa/main/api_loopback.c \
                           nx-X11/extras/Mesa/src/mesa/main/api_loopback.h \
                           nx-X11/extras/Mesa/src/mesa/main/api_noop.c \
                           nx-X11/extras/Mesa/src/mesa/main/api_noop.h \
                           nx-X11/extras/Mesa/src/mesa/main/api_validate.c \
                           nx-X11/extras/Mesa/src/mesa/main/api_validate.h \
                           nx-X11/extras/Mesa/src/mesa/main/attrib.c \
                           nx-X11/extras/Mesa/src/mesa/main/attrib.h \
                           nx-X11/extras/Mesa/src/mesa/main/blend.c \
                           nx-X11/extras/Mesa/src/mesa/main/blend.h \
                           nx-X11/extras/Mesa/src/mesa/main/bufferobj.c \
                           nx-X11/extras/Mesa/src/mesa/main/bufferobj.h \
                           nx-X11/extras/Mesa/src/mesa/main/buffers.c \
                           nx-X11/extras/Mesa/src/mesa/main/buffers.h \
                           nx-X11/extras/Mesa/src/mesa/main/clip.c \
                           nx-X11/extras/Mesa/src/mesa/main/clip.h \
                           nx-X11/extras/Mesa/src/mesa/main/colormac.h \
                           nx-X11/extras/Mesa/src/mesa/main/colortab.c \
                           nx-X11/extras/Mesa/src/mesa/main/colortab.h \
                           nx-X11/extras/Mesa/src/mesa/main/config.h \
                           nx-X11/extras/Mesa/src/mesa/main/context.c \
                           nx-X11/extras/Mesa/src/mesa/main/context.h \
                           nx-X11/extras/Mesa/src/mesa/main/convolve.c \
                           nx-X11/extras/Mesa/src/mesa/main/convolve.h \
                           nx-X11/extras/Mesa/src/mesa/main/dd.h \
                           nx-X11/extras/Mesa/src/mesa/main/debug.c \
                           nx-X11/extras/Mesa/src/mesa/main/debug.h \
                           nx-X11/extras/Mesa/src/mesa/main/depth.c \
                           nx-X11/extras/Mesa/src/mesa/main/depth.h \
                           nx-X11/extras/Mesa/src/mesa/main/dispatch.c \
                           nx-X11/extras/Mesa/src/mesa/main/dlist.c \
                           nx-X11/extras/Mesa/src/mesa/main/dlist.h \
                           nx-X11/extras/Mesa/src/mesa/main/drawpix.c \
                           nx-X11/extras/Mesa/src/mesa/main/drawpix.h \
                           nx-X11/extras/Mesa/src/mesa/main/enable.c \
                           nx-X11/extras/Mesa/src/mesa/main/enable.h \
                           nx-X11/extras/Mesa/src/mesa/main/enums.c \
                           nx-X11/extras/Mesa/src/mesa/main/enums.h \
                           nx-X11/extras/Mesa/src/mesa/main/eval.c \
                           nx-X11/extras/Mesa/src/mesa/main/eval.h \
                           nx-X11/extras/Mesa/src/mesa/main/execmem.c \
                           nx-X11/extras/Mesa/src/mesa/main/extensions.c \
                           nx-X11/extras/Mesa/src/mesa/main/extensions.h \
                           nx-X11/extras/Mesa/src/mesa/main/fbobject.c \
                           nx-X11/extras/Mesa/src/mesa/main/fbobject.h \
                           nx-X11/extras/Mesa/src/mesa/main/feedback.c \
                           nx-X11/extras/Mesa/src/mesa/main/feedback.h \
                           nx-X11/extras/Mesa/src/mesa/main/fog.c \
                           nx-X11/extras/Mesa/src/mesa/main/fog.h \
                           nx-X11/extras/Mesa/src/mesa/main/framebuffer.c \
                           nx-X11/extras/Mesa/src/mesa/main/framebuffer.h \
                           nx-X11/extras/Mesa/src/mesa/main/get.c \
                           nx-X11/extras/Mesa/src/mesa/main/get.h \
                           nx-X11/extras/Mesa/src/mesa/main/getstring.c \
                           nx-X11/extras/Mesa/src/mesa/main/glheader.h \
                           nx-X11/extras/Mesa/src/mesa/main/hash.c \
                           nx-X11/extras/Mesa/src/mesa/main/hash.h \
                           nx-X11/extras/Mesa/src/mesa/main/hint.c \
                           nx-X11/extras/Mesa/src/mesa/main/hint.h \
                           nx-X11/extras/Mesa/src/mesa/main/histogram.c \
                           nx-X11/extras/Mesa/src/mesa/main/histogram.h \
                           nx-X11/extras/Mesa/src/mesa/main/image.c \
                           nx-X11/extras/Mesa/src/mesa/main/image.h \
                           nx-X11/extras/Mesa/src/mesa/main/imports.c \
                           nx-X11/extras/Mesa/src/mesa/main/imports.h \
                           nx-X11/extras/Mesa/src/mesa/main/light.c \
                           nx-X11/extras/Mesa/src/mesa/main/light.h \
                           nx-X11/extras/Mesa/src/mesa/main/lines.c \
                           nx-X11/extras/Mesa/src/mesa/main/lines.h \
                           nx-X11/extras/Mesa/src/mesa/main/macros.h \
                           nx-X11/extras/Mesa/src/mesa/main/matrix.c \
                           nx-X11/extras/Mesa/src/mesa/main/matrix.h \
                           nx-X11/extras/Mesa/src/mesa/main/mm.c \
                           nx-X11/extras/Mesa/src/mesa/main/mm.h \
                           nx-X11/extras/Mesa/src/mesa/main/mtypes.h \
                           nx-X11/extras/Mesa/src/mesa/main/occlude.c \
                           nx-X11/extras/Mesa/src/mesa/main/occlude.h \
                           nx-X11/extras/Mesa/src/mesa/main/pixel.c \
                           nx-X11/extras/Mesa/src/mesa/main/pixel.h \
                           nx-X11/extras/Mesa/src/mesa/main/points.c \
                           nx-X11/extras/Mesa/src/mesa/main/points.h \
                           nx-X11/extras/Mesa/src/mesa/main/polygon.c \
                           nx-X11/extras/Mesa/src/mesa/main/polygon.h \
                           nx-X11/extras/Mesa/src/mesa/main/rastpos.c \
                           nx-X11/extras/Mesa/src/mesa/main/rastpos.h \
                           nx-X11/extras/Mesa/src/mesa/main/renderbuffer.c \
                           nx-X11/extras/Mesa/src/mesa/main/renderbuffer.h \
                           nx-X11/extras/Mesa/src/mesa/main/simple_list.h \
                           nx-X11/extras/Mesa/src/mesa/main/state.c \
                           nx-X11/extras/Mesa/src/mesa/main/state.h \
                           nx-X11/extras/Mesa/src/mesa/main/stencil.c \
                           nx-X11/extras/Mesa/src/mesa/main/stencil.h \
                           nx-X11/extras/Mesa/src/mesa/main/texcompress.c \
                           nx-X11/extras/Mesa/src/mesa/main/texcompress_fxt1.c \
                           nx-X11/extras/Mesa/src/mesa/main/texcompress.h \
                           nx-X11/extras/Mesa/src/mesa/main/texcompress_s3tc.c \
                           nx-X11/extras/Mesa/src/mesa/main/texenvprogram.c \
                           nx-X11/extras/Mesa/src/mesa/main/texenvprogram.h \
                           nx-X11/extras/Mesa/src/mesa/main/texformat.c \
                           nx-X11/extras/Mesa/src/mesa/main/texformat.h \
                           nx-X11/extras/Mesa/src/mesa/main/texformat_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/main/teximage.c \
                           nx-X11/extras/Mesa/src/mesa/main/teximage.h \
                           nx-X11/extras/Mesa/src/mesa/main/texobj.c \
                           nx-X11/extras/Mesa/src/mesa/main/texobj.h \
                           nx-X11/extras/Mesa/src/mesa/main/texrender.c \
                           nx-X11/extras/Mesa/src/mesa/main/texrender.h \
                           nx-X11/extras/Mesa/src/mesa/main/texstate.c \
                           nx-X11/extras/Mesa/src/mesa/main/texstate.h \
                           nx-X11/extras/Mesa/src/mesa/main/texstore.c \
                           nx-X11/extras/Mesa/src/mesa/main/texstore.h \
                           nx-X11/extras/Mesa/src/mesa/main/varray.c \
                           nx-X11/extras/Mesa/src/mesa/main/varray.h \
                           nx-X11/extras/Mesa/src/mesa/main/version.h \
                           nx-X11/extras/Mesa/src/mesa/main/vtxfmt.c \
                           nx-X11/extras/Mesa/src/mesa/main/vtxfmt.h \
                           nx-X11/extras/Mesa/src/mesa/main/vtxfmt_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/main/WSDrawBuffer.h \
                           nx-X11/extras/Mesa/src/mesa/math/mathmod.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_clip_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_copy_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_debug_clip.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_debug.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_debug_norm.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_debug_util.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_debug_xform.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_dotprod_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_eval.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_eval.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_matrix.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_matrix.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_norm_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_translate.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_translate.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_trans_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_vector.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_vector.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_xform.c \
                           nx-X11/extras/Mesa/src/mesa/math/m_xform.h \
                           nx-X11/extras/Mesa/src/mesa/math/m_xform_tmp.h \
                           nx-X11/extras/Mesa/src/mesa/shader/arbfragparse.c \
                           nx-X11/extras/Mesa/src/mesa/shader/arbfragparse.h \
                           nx-X11/extras/Mesa/src/mesa/shader/arbprogparse.c \
                           nx-X11/extras/Mesa/src/mesa/shader/arbprogparse.h \
                           nx-X11/extras/Mesa/src/mesa/shader/arbprogram.c \
                           nx-X11/extras/Mesa/src/mesa/shader/arbprogram.h \
                           nx-X11/extras/Mesa/src/mesa/shader/arbprogram_syn.h \
                           nx-X11/extras/Mesa/src/mesa/shader/arbvertparse.c \
                           nx-X11/extras/Mesa/src/mesa/shader/arbvertparse.h \
                           nx-X11/extras/Mesa/src/mesa/shader/atifragshader.c \
                           nx-X11/extras/Mesa/src/mesa/shader/atifragshader.h \
                           nx-X11/extras/Mesa/src/mesa/shader/grammar/grammar.c \
                           nx-X11/extras/Mesa/src/mesa/shader/grammar/grammar.h \
                           nx-X11/extras/Mesa/src/mesa/shader/grammar/grammar_mesa.c \
                           nx-X11/extras/Mesa/src/mesa/shader/grammar/grammar_mesa.h \
                           nx-X11/extras/Mesa/src/mesa/shader/grammar/grammar_syn.h \
                           nx-X11/extras/Mesa/src/mesa/shader/nvfragparse.c \
                           nx-X11/extras/Mesa/src/mesa/shader/nvfragparse.h \
                           nx-X11/extras/Mesa/src/mesa/shader/nvfragprog.h \
                           nx-X11/extras/Mesa/src/mesa/shader/nvprogram.c \
                           nx-X11/extras/Mesa/src/mesa/shader/nvprogram.h \
                           nx-X11/extras/Mesa/src/mesa/shader/nvvertexec.c \
                           nx-X11/extras/Mesa/src/mesa/shader/nvvertexec.h \
                           nx-X11/extras/Mesa/src/mesa/shader/nvvertparse.c \
                           nx-X11/extras/Mesa/src/mesa/shader/nvvertparse.h \
                           nx-X11/extras/Mesa/src/mesa/shader/nvvertprog.h \
                           nx-X11/extras/Mesa/src/mesa/shader/program.c \
                           nx-X11/extras/Mesa/src/mesa/shader/program.h \
                           nx-X11/extras/Mesa/src/mesa/shader/shaderobjects_3dlabs.c \
                           nx-X11/extras/Mesa/src/mesa/shader/shaderobjects_3dlabs.h \
                           nx-X11/extras/Mesa/src/mesa/shader/shaderobjects.c \
                           nx-X11/extras/Mesa/src/mesa/shader/shaderobjects.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_common_builtin_gc_bin.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_common_builtin_gc.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_core_gc_bin.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_core_gc.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_fragment_builtin_gc_bin.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_fragment_builtin_gc.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_shader_syn.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_version_syn.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_vertex_builtin_gc_bin.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/library/slang_vertex_builtin_gc.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_assignment.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_assignment.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_conditional.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_conditional.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_constructor.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_constructor.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_typeinfo.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_assemble_typeinfo.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_compile.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_compile.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_execute.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_execute.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_preprocess.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_preprocess.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_storage.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_storage.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_utility.c \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/slang_utility.h \
                           nx-X11/extras/Mesa/src/mesa/shader/slang/traverse_wrap.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_aaline.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_aaline.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_aalinetemp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_aatriangle.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_aatriangle.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_aatritemp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_accum.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_accum.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_alpha.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_alpha.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_atifragshader.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_atifragshader.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_bitmap.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_blend.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_blend.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_buffers.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_context.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_context.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_copypix.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_depth.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_depth.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_drawpix.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_drawpix.h \
                           nx-X11/extras/Mesa/src/mesa/swrast_setup/ss_context.c \
                           nx-X11/extras/Mesa/src/mesa/swrast_setup/ss_context.h \
                           nx-X11/extras/Mesa/src/mesa/swrast_setup/ss_triangle.c \
                           nx-X11/extras/Mesa/src/mesa/swrast_setup/ss_triangle.h \
                           nx-X11/extras/Mesa/src/mesa/swrast_setup/ss_tritmp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast_setup/swrast_setup.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_feedback.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_feedback.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_fog.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_fog.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_imaging.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_lines.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_lines.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_linetemp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_logic.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_logic.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_masking.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_masking.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_nvfragprog.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_nvfragprog.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_pixeltex.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_pixeltex.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_points.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_points.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_pointtemp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_readpix.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_span.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_span.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_spantemp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_stencil.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_stencil.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_texstore.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_texture.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_texture.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_triangle.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_triangle.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_trispan.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_tritemp.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/swrast.h \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_zoom.c \
                           nx-X11/extras/Mesa/src/mesa/swrast/s_zoom.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_array_api.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_array_api.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_array_import.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_array_import.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_context.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_context.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/tnl.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_pipeline.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_pipeline.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_save_api.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_save_api.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_save_loopback.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_save_playback.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_arbprogram.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_arbprogram.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_arbprogram_sse.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_cliptmp.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_cull.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_fog.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_light.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_lighttmp.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_normals.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_points.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_program.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_render.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_rendertmp.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_texgen.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_texmat.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vb_vertex.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vertex.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vertex_generic.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vertex.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vertex_sse.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vp_build.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vp_build.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vtx_api.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vtx_api.h \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vtx_eval.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vtx_exec.c \
                           nx-X11/extras/Mesa/src/mesa/tnl/t_vtx_generic.c \
                           nx-X11/extras/Xpm/lib/Attrib.c \
                           nx-X11/extras/Xpm/lib/CrBufFrI.c \
                           nx-X11/extras/Xpm/lib/CrBufFrP.c \
                           nx-X11/extras/Xpm/lib/CrDatFrI.c \
                           nx-X11/extras/Xpm/lib/CrDatFrP.c \
                           nx-X11/extras/Xpm/lib/create.c \
                           nx-X11/extras/Xpm/lib/CrIFrBuf.c \
                           nx-X11/extras/Xpm/lib/CrIFrDat.c \
                           nx-X11/extras/Xpm/lib/CrIFrP.c \
                           nx-X11/extras/Xpm/lib/CrPFrBuf.c \
                           nx-X11/extras/Xpm/lib/CrPFrDat.c \
                           nx-X11/extras/Xpm/lib/CrPFrI.c \
                           nx-X11/extras/Xpm/lib/data.c \
                           nx-X11/extras/Xpm/lib/hashtab.c \
                           nx-X11/extras/Xpm/lib/Image.c \
                           nx-X11/extras/Xpm/lib/Info.c \
                           nx-X11/extras/Xpm/lib/misc.c \
                           nx-X11/extras/Xpm/lib/parse.c \
                           nx-X11/extras/Xpm/lib/RdFToBuf.c \
                           nx-X11/extras/Xpm/lib/RdFToDat.c \
                           nx-X11/extras/Xpm/lib/RdFToI.c \
                           nx-X11/extras/Xpm/lib/RdFToP.c \
                           nx-X11/extras/Xpm/lib/rgb.c \
                           nx-X11/extras/Xpm/lib/scan.c \
                           nx-X11/extras/Xpm/lib/WrFFrBuf.c \
                           nx-X11/extras/Xpm/lib/WrFFrDat.c \
                           nx-X11/extras/Xpm/lib/WrFFrI.c \
                           nx-X11/extras/Xpm/lib/WrFFrP.c \
                           nx-X11/extras/Xpm/lib/xpm.h \
                           nx-X11/extras/Xpm/lib/XpmI.h \
                           nx-X11/include/ap_keysym.h \
                           nx-X11/include/bitmaps/1x1 \
                           nx-X11/include/bitmaps/2x2 \
                           nx-X11/include/bitmaps/black \
                           nx-X11/include/bitmaps/boxes \
                           nx-X11/include/bitmaps/calculator \
                           nx-X11/include/bitmaps/cntr_ptr \
                           nx-X11/include/bitmaps/cntr_ptrmsk \
                           nx-X11/include/bitmaps/cross_weave \
                           nx-X11/include/bitmaps/dimple1 \
                           nx-X11/include/bitmaps/dimple3 \
                           nx-X11/include/bitmaps/dot \
                           nx-X11/include/bitmaps/dropbar7 \
                           nx-X11/include/bitmaps/dropbar8 \
                           nx-X11/include/bitmaps/escherknot \
                           nx-X11/include/bitmaps/flagdown \
                           nx-X11/include/bitmaps/flagup \
                           nx-X11/include/bitmaps/flipped_gray \
                           nx-X11/include/bitmaps/gray \
                           nx-X11/include/bitmaps/gray1 \
                           nx-X11/include/bitmaps/gray3 \
                           nx-X11/include/bitmaps/grid16 \
                           nx-X11/include/bitmaps/grid2 \
                           nx-X11/include/bitmaps/grid4 \
                           nx-X11/include/bitmaps/grid8 \
                           nx-X11/include/bitmaps/hlines2 \
                           nx-X11/include/bitmaps/hlines3 \
                           nx-X11/include/bitmaps/icon \
                           nx-X11/include/bitmaps/keyboard16 \
                           nx-X11/include/bitmaps/left_ptr \
                           nx-X11/include/bitmaps/left_ptrmsk \
                           nx-X11/include/bitmaps/letters \
                           nx-X11/include/bitmaps/light_gray \
                           nx-X11/include/bitmaps/mailempty \
                           nx-X11/include/bitmaps/mailemptymsk \
                           nx-X11/include/bitmaps/mailfull \
                           nx-X11/include/bitmaps/mailfullmsk \
                           nx-X11/include/bitmaps/mensetmanus \
                           nx-X11/include/bitmaps/menu10 \
                           nx-X11/include/bitmaps/menu12 \
                           nx-X11/include/bitmaps/menu16 \
                           nx-X11/include/bitmaps/menu6 \
                           nx-X11/include/bitmaps/menu8 \
                           nx-X11/include/bitmaps/noletters \
                           nx-X11/include/bitmaps/opendot \
                           nx-X11/include/bitmaps/opendotMask \
                           nx-X11/include/bitmaps/plaid \
                           nx-X11/include/bitmaps/right_ptr \
                           nx-X11/include/bitmaps/right_ptrmsk \
                           nx-X11/include/bitmaps/root_weave \
                           nx-X11/include/bitmaps/scales \
                           nx-X11/include/bitmaps/sipb \
                           nx-X11/include/bitmaps/star \
                           nx-X11/include/bitmaps/starMask \
                           nx-X11/include/bitmaps/stipple \
                           nx-X11/include/bitmaps/target \
                           nx-X11/include/bitmaps/terminal \
                           nx-X11/include/bitmaps/tie_fighter \
                           nx-X11/include/bitmaps/vlines2 \
                           nx-X11/include/bitmaps/vlines3 \
                           nx-X11/include/bitmaps/weird_size \
                           nx-X11/include/bitmaps/wide_weave \
                           nx-X11/include/bitmaps/wingdogs \
                           nx-X11/include/bitmaps/woman \
                           nx-X11/include/bitmaps/xfd_icon \
                           nx-X11/include/bitmaps/xlogo11 \
                           nx-X11/include/bitmaps/xlogo16 \
                           nx-X11/include/bitmaps/xlogo32 \
                           nx-X11/include/bitmaps/xlogo64 \
                           nx-X11/include/bitmaps/xsnow \
                           nx-X11/include/DECkeysym.h \
                           nx-X11/include/extensions/bigreqstr.h \
                           nx-X11/include/extensions/composite.h \
                           nx-X11/include/extensions/compositeproto.h \
                           nx-X11/include/extensions/damageproto.h \
                           nx-X11/include/extensions/damagewire.h \
                           nx-X11/include/extensions/dmxext.h \
                           nx-X11/include/extensions/dmxproto.h \
                           nx-X11/include/extensions/dpms.h \
                           nx-X11/include/extensions/dpmsstr.h \
                           nx-X11/include/extensions/extutil.h \
                           nx-X11/include/extensions/lbxbuf.h \
                           nx-X11/include/extensions/lbxbufstr.h \
                           nx-X11/include/extensions/lbxdeltastr.h \
                           nx-X11/include/extensions/lbximage.h \
                           nx-X11/include/extensions/lbxopts.h \
                           nx-X11/include/extensions/lbxstr.h \
                           nx-X11/include/extensions/lbxzlib.h \
                           nx-X11/include/extensions/MITMisc.h \
                           nx-X11/include/extensions/mitmiscstr.h \
                           nx-X11/include/extensions/multibuf.h \
                           nx-X11/include/extensions/multibufst.h \
                           nx-X11/include/extensions/panoramiXext.h \
                           nx-X11/include/extensions/panoramiXproto.h \
                           nx-X11/include/extensions/Print.h \
                           nx-X11/include/extensions/Printstr.h \
                           nx-X11/include/extensions/randr.h \
                           nx-X11/include/extensions/randrproto.h \
                           nx-X11/include/extensions/record.h \
                           nx-X11/include/extensions/recordstr.h \
                           nx-X11/include/extensions/render.h \
                           nx-X11/include/extensions/renderproto.h \
                           nx-X11/include/extensions/security.h \
                           nx-X11/include/extensions/securstr.h \
                           nx-X11/include/extensions/shape.h \
                           nx-X11/include/extensions/shapestr.h \
                           nx-X11/include/extensions/shmstr.h \
                           nx-X11/include/extensions/sync.h \
                           nx-X11/include/extensions/syncstr.h \
                           nx-X11/include/extensions/vldXvMC.h \
                           nx-X11/include/extensions/Xag.h \
                           nx-X11/include/extensions/Xagsrv.h \
                           nx-X11/include/extensions/Xagstr.h \
                           nx-X11/include/extensions/xcmiscstr.h \
                           nx-X11/include/extensions/Xcup.h \
                           nx-X11/include/extensions/Xcupstr.h \
                           nx-X11/include/extensions/Xdbe.h \
                           nx-X11/include/extensions/Xdbeproto.h \
                           nx-X11/include/extensions/Xevie.h \
                           nx-X11/include/extensions/Xeviestr.h \
                           nx-X11/include/extensions/XEVI.h \
                           nx-X11/include/extensions/XEVIstr.h \
                           nx-X11/include/extensions/Xext.h \
                           nx-X11/include/extensions/xf86bigfont.h \
                           nx-X11/include/extensions/xf86bigfstr.h \
                           nx-X11/include/extensions/xf86dga1.h \
                           nx-X11/include/extensions/xf86dga1str.h \
                           nx-X11/include/extensions/xf86dga.h \
                           nx-X11/include/extensions/xf86dgastr.h \
                           nx-X11/include/extensions/xf86misc.h \
                           nx-X11/include/extensions/xf86mscstr.h \
                           nx-X11/include/extensions/xf86vmode.h \
                           nx-X11/include/extensions/xf86vmstr.h \
                           nx-X11/include/extensions/xfixesproto.h \
                           nx-X11/include/extensions/xfixeswire.h \
                           nx-X11/include/extensions/XI.h \
                           nx-X11/include/extensions/Xinerama.h \
                           nx-X11/include/extensions/XInput.h \
                           nx-X11/include/extensions/XIproto.h \
                           nx-X11/include/extensions/XKBgeom.h \
                           nx-X11/include/extensions/XKB.h \
                           nx-X11/include/extensions/XKBproto.h \
                           nx-X11/include/extensions/XKBsrv.h \
                           nx-X11/include/extensions/XKBstr.h \
                           nx-X11/include/extensions/XLbx.h \
                           nx-X11/include/extensions/XRes.h \
                           nx-X11/include/extensions/XResproto.h \
                           nx-X11/include/extensions/XShm.h \
                           nx-X11/include/extensions/xtestext1.h \
                           nx-X11/include/extensions/XTest.h \
                           nx-X11/include/extensions/xteststr.h \
                           nx-X11/include/extensions/xtrapbits.h \
                           nx-X11/include/extensions/xtrapddmi.h \
                           nx-X11/include/extensions/xtrapdi.h \
                           nx-X11/include/extensions/xtrapemacros.h \
                           nx-X11/include/extensions/xtraplib.h \
                           nx-X11/include/extensions/xtraplibp.h \
                           nx-X11/include/extensions/xtrapproto.h \
                           nx-X11/include/extensions/Xv.h \
                           nx-X11/include/extensions/Xvlib.h \
                           nx-X11/include/extensions/XvMC.h \
                           nx-X11/include/extensions/XvMClib.h \
                           nx-X11/include/extensions/XvMCproto.h \
                           nx-X11/include/extensions/Xvproto.h \
                           nx-X11/include/fonts/font.h \
                           nx-X11/include/fonts/fontstruct.h \
                           nx-X11/include/fonts/FS.h \
                           nx-X11/include/fonts/fsmasks.h \
                           nx-X11/include/fonts/FSproto.h \
                           nx-X11/include/GL/glu.h \
                           nx-X11/include/GL/glx.h \
                           nx-X11/include/GL/glxint.h \
                           nx-X11/include/GL/glxmd.h \
                           nx-X11/include/GL/glxproto.h \
                           nx-X11/include/GL/glxtokens.h \
                           nx-X11/include/HPkeysym.h \
                           nx-X11/include/keysymdef.h \
                           nx-X11/include/keysym.h \
                           nx-X11/include/Sunkeysym.h \
                           nx-X11/include/Xalloca.h \
                           nx-X11/include/Xarch.h \
                           nx-X11/include/Xatom.h \
                           nx-X11/include/Xdefs.h \
                           nx-X11/include/XF86keysym.h \
                           nx-X11/include/Xfuncproto.h \
                           nx-X11/include/Xfuncs.h \
                           nx-X11/include/X.h \
                           nx-X11/include/Xmd.h \
                           nx-X11/include/Xosdefs.h \
                           nx-X11/include/Xos.h \
                           nx-X11/include/Xos_r.h \
                           nx-X11/include/Xproto.h \
                           nx-X11/include/Xprotostr.h \
                           nx-X11/include/Xthreads.h \
                           nx-X11/include/XWDFile.h \
                           nx-X11/lib/font/bitmap/bdfint.h \
                           nx-X11/lib/font/bitmap/pcf.h \
                           nx-X11/lib/font/include/bitmap.h \
                           nx-X11/lib/font/include/bufio.h \
                           nx-X11/lib/font/include/fntfil.h \
                           nx-X11/lib/font/include/fntfilio.h \
                           nx-X11/lib/font/include/fntfilst.h \
                           nx-X11/lib/font/include/fontencc.h \
                           nx-X11/lib/font/include/fontmisc.h \
                           nx-X11/lib/font/include/fontmod.h \
                           nx-X11/lib/font/include/fontshow.h \
                           nx-X11/lib/font/include/fontutil.h \
                           nx-X11/lib/font/include/fontxlfd.h \
                           nx-X11/lib/misc/strlcat.c \
                           nx-X11/lib/misc/strlcpy.c \
                           nx-X11/lib/oldX/X10.h \
                           nx-X11/lib/X11/cursorfont.h \
                           nx-X11/lib/X11/ImUtil.h \
                           nx-X11/lib/X11/Xcms.h \
                           nx-X11/lib/X11/XKBAlloc.c \
                           nx-X11/lib/X11/XKBGAlloc.c \
                           nx-X11/lib/X11/XKBlib.h \
                           nx-X11/lib/X11/XKBMAlloc.c \
                           nx-X11/lib/X11/XKBMisc.c \
                           nx-X11/lib/X11/XlibConf.h \
                           nx-X11/lib/X11/Xlib.h \
                           nx-X11/lib/X11/Xlibint.h \
                           nx-X11/lib/X11/Xlocale.h \
                           nx-X11/lib/X11/Xregion.h \
                           nx-X11/lib/X11/Xresource.h \
                           nx-X11/lib/X11/Xutil.h \
                           nx-X11/lib/Xau/AuDispose.c \
                           nx-X11/lib/Xau/AuFileName.c \
                           nx-X11/lib/Xau/AuGetBest.c \
                           nx-X11/lib/Xau/AuRead.c \
                           nx-X11/lib/Xau/Xauth.h \
                           nx-X11/lib/Xcomposite/Xcomposite.h \
                           nx-X11/lib/Xdamage/Xdamage.h \
                           nx-X11/lib/Xdmcp/Wrap.c \
                           nx-X11/lib/Xdmcp/Wrap.h \
                           nx-X11/lib/Xdmcp/Wraphelp.c \
                           nx-X11/lib/Xdmcp/Xdmcp.h \
                           nx-X11/lib/Xfixes/Xfixes.h \
                           nx-X11/lib/xkbfile/maprules.c \
                           nx-X11/lib/xkbfile/XKBbells.h \
                           nx-X11/lib/xkbfile/xkbconfig.c \
                           nx-X11/lib/xkbfile/XKBconfig.h \
                           nx-X11/lib/xkbfile/xkberrs.c \
                           nx-X11/lib/xkbfile/XKBfile.h \
                           nx-X11/lib/xkbfile/xkbmisc.c \
                           nx-X11/lib/xkbfile/xkbout.c \
                           nx-X11/lib/xkbfile/XKBrules.h \
                           nx-X11/lib/xkbfile/xkbtext.c \
                           nx-X11/lib/xkbfile/XKMformat.h \
                           nx-X11/lib/xkbfile/XKM.h \
                           nx-X11/lib/xkbfile/xkmread.c \
                           nx-X11/lib/Xrandr/Xrandr.h \
                           nx-X11/lib/Xrender/Xrender.h \
                           nx-X11/lib/xtrans/transport.c \
                           nx-X11/lib/xtrans/Xtrans.c \
                           nx-X11/lib/xtrans/Xtransdnet.c \
                           nx-X11/lib/xtrans/Xtrans.h \
                           nx-X11/lib/xtrans/Xtransint.h \
                           nx-X11/lib/xtrans/Xtranslcl.c \
                           nx-X11/lib/xtrans/Xtransos2.c \
                           nx-X11/lib/xtrans/Xtranssock.c \
                           nx-X11/lib/xtrans/Xtranstli.c \
                           nx-X11/lib/xtrans/Xtransutil.c \
                           nx-X11/programs/Xserver/hw/xfree86/common/compiler.h \
                           nx-X11/programs/Xserver/hw/xfree86/os-support/xf86_ansic.h \
                           nx-X11/programs/Xserver/hw/xfree86/os-support/xf86_libc.h \
                           nx-X11/programs/Xserver/hw/xfree86/xf86Version.h \
                           nx-X11/programs/Xserver/include/misc.h \
                           nx-X11/programs/Xserver/include/os.h \
                           nx-X11/programs/Xserver/mi/miinitext.c \
                           nx-X11/programs/Xserver/os/osdep.h \
                           nx-X11/programs/Xserver/Xext/extmod/modinit.h \
                           nx-X11/programs/Xserver/Xi/stubs.c \
                           ${NULL}
"

PRESERVE_INCLUDED_FILES="
                           nx-X11/programs/Xserver/mfb/maskbits.h \
                           ${NULL}
"

mkdir -p .preserve/
for path in ${PRESERVE_SYMLINKED_FILES} ${PRESERVE_INCLUDED_FILES}; do
	if [ ! -d $path ]; then
		path_dirname=$(dirname "$path")
	else
		path_dirname="$path"
	fi
	mkdir -vp ".preserve/$path_dirname"
	cp -av "$path" ".preserve/$path"
done

for folder in ${UNUSED_FOLDERS} ${CLEAN_FOLDERS}; do
    rm -Rf "$folder"
done

# re-create the to-be-preserved files
cp -a .preserve/* ./
rm -Rf .preserve/

# remove files, that we do not want in the tarballs (build cruft)
rm -Rf nx*/configure nx*/autom4te.cache*

cd $OLDPWD

# create target location for tarball
mkdir -p "$TARGETDIR/_releases_/source/${PROJECT}/"

# roll the ball...
cd "$TEMP_DIR"
find "${PROJECT}-${RELEASE}" -type f | sort > "$MANIFEST"
cd $OLDPWD

tar c -C "$TEMP_DIR" \
    --owner 0 \
    --group 0 \
    --numeric-owner \
    --no-recursion \
    --files-from "$MANIFEST" \
    --gzip \
    > "$TARGETDIR/_releases_/source/${PROJECT}/${PROJECT}-${RELEASE}${RELEASE_SUFFIX}.tar.gz" \

echo "$TARGETDIR/_releases_/source/${PROJECT}/${PROJECT}-${RELEASE}${RELEASE_SUFFIX}.tar.gz  is ready"
