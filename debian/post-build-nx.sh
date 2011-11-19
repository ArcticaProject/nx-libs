#!/bin/bash

set -ex

# create nx-X11/.build-exports
mkdir -p nx-X11/.build-exports/include
mkdir -p nx-X11/.build-exports/lib

# copy headers (libnx-x11-dev)
cp -aL nx-X11/exports/include/* nx-X11/.build-exports/include

# copy libs (libnx-x11)
find nx-X11/exports/lib/ | egrep "^.*\.so$" | while read libpath; do
    libfile=$(basename $libpath)
    libdir=$(dirname $libpath)
    mkdir -p ${libdir//exports/.build-exports}
    cp -L $libpath ${libdir//exports/.build-exports}
    find $libdir/$libfile.* | while read symlink; do
        ln -s $libfile ${libdir//exports/.build-exports}/$(basename $symlink)
    done
done

exit 0
