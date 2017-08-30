#!/bin/bash

# Copyright (C) 2011-2016 by Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
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

test -d ".git" || usage
RELEASE="$1"
test -n "${RELEASE}" || usage
MODE="$2"
test -n "$MODE" || usage


if [ "x$MODE" = "xserver" ] || [ "x${MODE}" = "xfull" ]; then
    MODE="full"
    RELEASE_SUFFIX='-full'
elif [ "x$MODE" = "xclient" ] || [ "x${MODE}" = "xlite" ]; then
    MODE="lite"
    RELEASE_SUFFIX='-lite'
else
    usage
fi

CHECKOUT="${RELEASE}"

if [ x"$RELEASE" == "xHEAD" ]; then
    CHECKOUT="refs/heads/$(git rev-parse --abbrev-ref HEAD)"
fi

if ! git rev-parse --verify -q "$CHECKOUT" >/dev/null; then
    echo "   '${RELEASE}' is not a valid release number because there is no git tag named ${CHECKOUT}."
    echo "   Please specify one of the following releases:"
    echo "HEAD (on branch `git rev-parse --abbrev-ref HEAD`)"
    git tag -l | grep "^redist" | cut -f2 -d"/" | sort -u
    exit 1
fi

TARGETDIR="../.."

MANIFEST="$(mktemp)"
TEMP_DIR="$(mktemp -d)"

trap "rm -f \"${MANIFEST}\"; rm -rf \"${TEMP_DIR}\"" 0

# create local copy of Git project at temp location
git archive --format=tar "${CHECKOUT}" --prefix="${PROJECT}-${RELEASE}/" | ( cd "$TEMP_DIR"; tar xf - )
git --no-pager log --after "1972-01-01" --format="%ai %aN (%h) %n%n%x09*%w(68,0,10) %s%d%n" "${CHECKOUT}" > "${TEMP_DIR}/${PROJECT}-${RELEASE}/ChangeLog"

echo "Created tarball for $CHECKOUT"

cd "${TEMP_DIR}/${PROJECT}-${RELEASE}/"

# Replace symlinks by copies of the linked target files
# Note: We don't have symlinked directories!!!
find . -type "l" | while read link; do
	TARGET="$(readlink "${link}")"
	pushd "$(dirname "${link}")" >/dev/null
	if [ -f "${TARGET}" ]; then
		rm -f "$(basename "${link}")"
		cp "${TARGET}" "$(basename "${link}")"
	fi
	popd >/dev/null
done

mkdir -p "doc/applied-patches"

# prepare patches for lite and full tarball
if [ "x$MODE" = "xfull" ]; then

    rm -f "README.md"
    rm -Rf "doc/_attic_/"
    rm -f  ".gitignore"
    rm -f  "nxcomp/.gitignore"
    rm -f  "nxcompext/.gitignore"
    rm -f  "nxcompshad/.gitignore"
    rm -f  "nxproxy/.gitignore"
    rm -f  "nx-X11/lib/X11/.gitignore"
    rm -f  "nx-X11/.gitignore"
    rm -f  "nx-X11/lib/include/X11/.gitignore"
    rm -f  "nx-X11/lib/src/.gitignore"
    rm -f  "nx-X11/programs/Xserver/composite/.gitignore"
    rm -f  "nx-X11/programs/Xserver/hw/nxagent/.gitignore"
    rm -f  "nx-X11/programs/Xserver/.gitignore"
    rm -f  "nx-X11/programs/Xserver/include/.gitignore"
    rm -f  "nx-X11/programs/Xserver/GL/.gitignore"
    rm -f  "nx-X11/include/.gitignore"

    # bring Mesa in shape, drop symlinks and move versioned Mesa-bundle to
    # nx-X11/extras/Mesa. Deal with the Mesa.patches symlink/folder accordingly
    cp -Lr "nx-X11/extras/Mesa" "nx-X11/extras/tmpMesa"
    cp -Lr "nx-X11/extras/Mesa.patches" "nx-X11/extras/tmpMesa.patches"
    ls -d nx-X11/extras/* | grep -v "nx-X11/extras/tmpMesa*" | xargs rm -r
    mv "nx-X11/extras/tmpMesa" "nx-X11/extras/Mesa"
    mv "nx-X11/extras/tmpMesa.patches" "nx-X11/extras/Mesa.patches"

    # shrink Mesa to what we really need (and nothing else)
    rm -Rf "nx-X11/extras/Mesa/"{bin/,configs/,docs/,doxygen/,progs/,vms/,windows/,Makefile,Makefile.*,descrip.mms,mms-config.}
    rm -Rf "nx-X11/extras/Mesa/include/"{GLES,GLView.h}
    rm -f  "nx-X11/extras/Mesa/include/GL/"{amesa.h,directfbgl.h,dmesa.h,foomesa.h,fxmesa.h,ggimesa.h,glfbdev.h,gl_mangle.h,glu.h,glu_mangle.h,glutf90.h,glut.h,glut_h.dja,glx.h,glx_mangle.h,Makefile.am,mesa_wgl.h,mglmesa.h,miniglx.h,svgamesa.h,uglglutshapes.h,uglmesa.h,vms_x_fix.h,wmesa.h,xmesa_x.h}
    rm -f  "nx-X11/extras/Mesa/include/GL/internal/"{dri_interface.h,sarea.h}
    rm -Rf "nx-X11/extras/Mesa/src/"{egl/,glu/,glut/,glw/,glx/mini/,Makefile,glx/Makefile,descrip.mms}
    rm -f  "nx-X11/extras/Mesa/src/glx/x11/"{clientattrib.c,dri_glx.c,dri_glx.h,eval.c,glxclient.h,glxcmds.c,glxext.c,glxextensions.c,glxextensions.h,glx_pbuffer.c,glx_query.c,glx_texture_compression.c,indirect.c,indirect.h,indirect_init.c,indirect_init.h,indirect_transpose_matrix.c,indirect_va_private.h,indirect_vertex_array.c,indirect_vertex_array.h,indirect_vertex_program.c,indirect_window_pos.c,packrender.h,packsingle.h,pixel.c,pixelstore.c,render2.c,renderpix.c,single2.c,singlepix.c,vertarr.c,XF86dri.c,xf86dri.h,xf86dristr.h,xfont.c}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/"{ppc,sparc,tnl_dd,x86,x86-64,sources}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/drivers/"{allegro,beos,d3d,directfb,dos,fbdev,ggi,glide,osmesa,svga,windows}
    rm -f  "nx-X11/extras/Mesa/src/mesa/drivers/x11/"{fakeglx.c,glxapi.c,glxapi.h,realglx.*,xfonts.*}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/drivers/dri/"{ffb,i810,i915,mga,r200,radeon,savage,tdfx,unichrome,fb,gamma,i830,mach64,r128,r300,s3v,sis,trident}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/drivers/dri/x11/"x11_dri.{c,h}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/drivers/dri/common/"{depthtmp.h,drirenderbuffer.h,dri_util.h,memops.h,mmx.h,spantmp_common.h,stenciltmp.h,texmem.h,utils.h,vblank.h,xmlconfig.h,xmlpool.h,drirenderbuffer.c,dri_util.c,extension_helper.h,mmio.h,spantmp2.h,spantmp.h,texmem.c,utils.c,vblank.c,xmlconfig.c,xmlpool/}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/glapi/"{*.dtd,*.py,*.sh,*.xml,.cvsignore}
    rm -f  "nx-X11/extras/Mesa/src/mesa/shader/"*.syn
    rm -f  "nx-X11/extras/Mesa/src/mesa/shader/asmopcodes.reg"
    rm -f  "nx-X11/extras/Mesa/src/mesa/shader/grammar/grammar_crt."{c,h}
    rm -f  "nx-X11/extras/Mesa/src/mesa/shader/grammar/"*.syn
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/MachineIndependent/"
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/OGLCompilersDLL/"
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/OSDependent/"
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/slang_mesa."{cpp,h}
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/library/"*.*c
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/library/"*.py
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/library/"*.syn
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/Include/"
    rm -Rf "nx-X11/extras/Mesa/src/mesa/shader/slang/Public/"
    rm -f  "nx-X11/extras/Mesa/src/mesa/swrast/s_fragprog_to_c.c"
    rm -f  "nx-X11/extras/Mesa/src/mesa/swrast/s_tcc.c"
    rm -f  "nx-X11/extras/Mesa/src/mesa/swrast_setup/ss_vb.h"
    rm -Rf "nx-X11/extras/Mesa/src/mesa/"*/NOTES
    rm -f  "nx-X11/extras/Mesa/src/mesa/main/"*.py
    rm -f  "nx-X11/extras/Mesa/src/mesa/main/"{mesa.def,Imakefile,vsnprintf.c}

    find nx-X11/extras/Mesa/ -name Makefile | while read file; do rm "$file"; done
    find nx-X11/extras/Mesa/ -name Makefile.* | while read file; do rm "$file"; done
    find nx-X11/extras/Mesa/ -name descrip.mms | while read file; do rm "$file"; done

    # this is for 3.5.0.x only...
    cat "debian/patches/series" | sort | grep -v '^#' | egrep "([0-9]+(_|-).*\.(full|full\+lite)\.patch)" | while read file
    do
        cp -v "debian/patches/$file" "doc/applied-patches/"
        echo "${file##*/}" >> "doc/applied-patches/series"
    done

else
    rm -f "README.md"
    rm -f  "bin/nxagent"*
    rm -Rf "nxcompshad"*
    rm -Rf "nx-X11"*
    rm -Rf "etc"*
    rm -f  "generate-symbol-docs.sh"
    rm -Rf "testscripts/"*nxagent*
    rm -Rf "testscripts/"slave*
    rm -Rf "doc/libNX_X11/"
    rm -Rf "doc/nxagent/"
    rm -Rf "doc/nxcompext/"
    rm -Rf "doc/nxcompshad/"
    rm -Rf "doc/_attic_/"
    rm -f  ".gitignore"
    rm -f  "m4/nx-xtrans.m4"
    rm -f  "nxcomp/.gitignore"
    rm -f  "nxproxy/.gitignore"

    # for old nx-libs releases, re-arranged since 3.5.99.3
    rm -Rf "doc/nx-X11_vs_XOrg69_patches"*
    rm -Rf "doc/X11-symbols"*
    rm -f  "README.keystrokes"
    rm -Rf "nxcompext"*

    mv LICENSE.nxcomp LICENSE

    # this is for 3.5.0.x only...
    cat "debian/patches/series" | sort | grep -v '^#' | egrep "([0-9]+(_|-).*\.full\+lite\.patch)" | while read file
    do
        cp -v "debian/patches/$file" "doc/applied-patches/"
        echo "${file##*/}" >> "doc/applied-patches/series"
    done

fi

# apply all patches shipped in debian/patches and create a copy of them that we ship with the tarball
if [ -s "doc/applied-patches/series" ]; then
    QUILT_PATCHES="doc/applied-patches" quilt --quiltrc /dev/null push -a -q
fi

# remove folders that we do not want to roll into the tarball
rm -Rf ".pc/"
rm -Rf "debian/"
rm -Rf "nx-libs.spec"

# very old release did not add any README
for f in $(ls README* 2>/dev/null); do
    mv -v "$f" "doc/";
done

# remove files, that we do not want in the tarballs (build cruft)
rm -Rf nx*/configure nx*/autom4te.cache*

cd "$OLDPWD"

# create target location for tarball
mkdir -p "${TARGETDIR}/_releases_/source/${PROJECT}/"

# roll the ball...
cd "$TEMP_DIR"
find "${PROJECT}-${RELEASE}" -type f | sort > "$MANIFEST"
cd "$OLDPWD"

tar c -C "$TEMP_DIR" \
    --owner 0 \
    --group 0 \
    --numeric-owner \
    --no-recursion \
    --files-from "$MANIFEST" \
    --gzip \
    > "$TARGETDIR/_releases_/source/${PROJECT}/${PROJECT}-${RELEASE}${RELEASE_SUFFIX}.tar.gz"

echo "$TARGETDIR/_releases_/source/${PROJECT}/${PROJECT}-${RELEASE}${RELEASE_SUFFIX}.tar.gz  is ready"
