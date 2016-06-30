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

test -d ".git" || usage
test -f "debian/Makefile.nx-libs" || usage
RELEASE="$1"
test -n "${RELEASE}" || usage
CHECKOUT="$2"
test -n "$CHECKOUT" || usage

if [ "x$CHECKOUT" = "xserver" ] || [ "x${CHECKOUT}" = "xfull" ]; then
    MODE="full"
    CHECKOUT="redist-server/${RELEASE}"
    RELEASE_SUFFIX='-full'
elif [ "x$CHECKOUT" = "xclient" ] || [ "x${CHECKOUT}" = "xlite" ]; then
    MODE="lite"
    CHECKOUT="redist-client/${RELEASE}"
    RELEASE_SUFFIX='-lite'
else
    usage
fi

if [ x"$RELEASE" == "xHEAD" ]; then
    CHECKOUT=refs/heads/$(git rev-parse --abbrev-ref HEAD)
fi

if ! git rev-parse --verify -q "$CHECKOUT" >/dev/null; then
    echo "   '${RELEASE}' is not a valid release number because there is no git tag named ${CHECKOUT}."
    echo "   Please specify one of the following releases:"
    echo "HEAD (on branch `git rev-parse --abbrev-ref HEAD`)"
    git tag -l | grep "^redist" | cut -f2 -d"/" | sort -u
    exit 1
fi

TARGETDIR=".."

MANIFEST="$(mktemp)"
TEMP_DIR="$(mktemp -d)"

trap "rm -f \"${MANIFEST}\"; rm -rf \"${TEMP_DIR}\"" 0

# create local copy of Git project at temp location
git archive --format=tar "${CHECKOUT}" --prefix="${PROJECT}-${RELEASE}/" | ( cd "$TEMP_DIR"; tar xf - )

echo "Created tarball for $CHECKOUT"

cd "${TEMP_DIR}/${PROJECT}-${RELEASE}/"

mkdir -p "doc/applied-patches"

# prepare patches for lite and full tarball
if [ "x$MODE" = "xfull" ]; then
    cat "debian/patches/series" | sort | grep -v '^#' | egrep "([0-9]+(_|-).*\.(full|full\+lite)\.patch)" | while read file
    do
        cp -v "debian/patches/$file" "doc/applied-patches/"
        echo "${file##*/}" >> "doc/applied-patches/series"
    done
    mkdir -p "./etc/"
    cp -v "debian/rgb" "./etc/"
    cp -v "debian/nxagent.keyboard" "./etc/"
    cp -v "debian/x2goagent.keyboard" "./etc/"
    cp -v "debian/x2goagent.options" "./etc/"
    cp -v "debian/VERSION" "./VERSION.x2goagent"
else
    rm -Rf "nxcompshad"*
    rm -Rf "nxcompext"*
    rm -Rf "nx-X11"*
    cat "debian/patches/series" | sort | grep -v '^#' | egrep "([0-9]+(_|-).*\.full\+lite\.patch)" | while read file
    do
        cp -v "debian/patches/$file" "doc/applied-patches/"
        echo "${file##*/}" >> "doc/applied-patches/series"
    done
fi
cp -v "debian/VERSION" "./nxcomp/VERSION"
cp -v "debian/COPYING.full+lite" "COPYING"

# apply all patches shipped in debian/patches and create a copy of them that we ship with the tarball
if [ -s "doc/applied-patches/series" ]; then
    QUILT_PATCHES="doc/applied-patches" quilt --quiltrc /dev/null push -a -q
else
    echo "No patches applied at all. Very old release?"
fi

# very old release did not add any README
for f in $(ls README* 2>/dev/null); do
    mv -v "$f" "doc/";
done

mkdir -p bin/
if [ "$MODE" = "lite" ]; then
    # copy wrapper script nxproxy only into tarball
    cp -v "debian/wrappers/nxproxy" "bin/"
else
    # copy wrapper scripts into tarball
    for w in $(ls debian/wrappers/* 2>/dev/null); do
        cp -v "$w" "bin/"
    done
    # provide a default keystrokes.cfg file
    mkdir -p "etc"
    test -f "etc/keystrokes.cfg" || test -f "debian/keystrokes.cfg" && cp -v "debian/keystrokes.cfg" "etc/keystrokes.cfg"
fi

mv -v "debian/changelog" "doc/changelog"

# copy the top-level makefile if no quilt patch created it before
test -f "Makefile" || test -f "debian/Makefile.nx-libs" && cp -v "debian/Makefile.nx-libs" "Makefile"
test -f "replace.sh" || test -f "debian/Makefile.replace.sh" && cp -v "debian/Makefile.replace.sh" "replace.sh"


if [ "x$MODE" = "xfull" ]; then
    #### stuff we do not needed
    UNUSED_FOLDERS="$(cat "debian/CODE-REDUCTION_CLEANUP-UNUSED")"
    CLEANUP_FOLDERS="$(cat "debian/CODE-REDUCTION_CLEANUP-BRUTAL")"
    UNNEEDED_BACKUPS="$(cat "debian/CODE-REDUCTION_CLEANUP-BACKUPFILES")"
    PRESERVE_SYMLINKED_FILES="$(cat "debian/CODE-REDUCTION_PRESERVE-SYMLINKED")"
    PRESERVE_INCLUDED_FILES="$(cat "debian/CODE-REDUCTION_PRESERVE-INCLUDED")"
    PRESERVE_NEEDED_FILES="$(cat "debian/CODE-REDUCTION_PRESERVE-NEEDED")"

    mkdir -p ".preserve/"
    for path in ${PRESERVE_SYMLINKED_FILES} ${PRESERVE_INCLUDED_FILES} ${PRESERVE_NEEDED_FILES}; do
        if [ ! -d "$path" ]; then
            path_dirname="$(dirname "$path")"
        else
            path_dirname="$path"
        fi
        mkdir -vp ".preserve/$path_dirname"
        cp -av "$path" ".preserve/$path"
    done

    for path in ${UNUSED_FOLDERS} ${CLEANUP_FOLDERS} ${UNNEEDED_BACKUPS}; do
        rm -R "$path"
    done

    # re-create the to-be-preserved files
    cp -a ".preserve/"* "./"
    rm -Rf ".preserve/"
fi

# remove files/folders that we do not want to roll into the tarball
rm -Rf ".pc/"
rm -Rf "debian/"
rm -f  "nx-libs.spec"

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
