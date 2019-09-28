#!/bin/bash

set -x

NULL=

PKG_CONFIG_DEFAULT_LIBDIR=$(pkg-config --variable pc_path pkg-config)
export PKG_CONFIG_LIBDIR="../../../nxcomp:../../lib/:../../../nxcompshad:$PKG_CONFIG_DEFAULT_LIBDIR"

SRC=.

export CONFIGURE

build() {
	PKG=$1
	shift
	[ -d ${SRC}/$PKG ] || return 0

	OPT="$@"

	pushd "${SRC}/$PKG"
	[ -e Makefile ] && make distclean

	[ -e configure ] && rm -f configure

	if [ ! -e configure ]; then
		./autogen.sh $OPT || exit 1
	fi

	make

	popd
}

export CONFFLAGS="--enable-unix-transport		\
                  --enable-tcp-transport		\
                  --enable-ipv6				\
                  --enable-secure-rpc			\
                  ${NULL}"

export NXAGENTMODULES_LIBS="-L`pwd`/../exports/lib			  \
                            -L`pwd`/../../nxcomp/src/.libs		  \
                            -L`pwd`/../../nxcompshad/src/.libs		  \
                            -lXcomp -lXcompshad -lNX_X11		  \
                            ${NULL}"

build Xserver $CONFFLAGS PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR" || exit 1

(
	# create the binaries at the same location imake did. Also create
	# development binaries and links.
	cd ${SRC}/Xserver
	if [ -e hw/nxagent/.libs/nxagent ]; then
		ln -snf hw/nxagent/.libs/nxagent nxagent-relink
		cp -f --suffix=.previous --backup=simple hw/nxagent/.libs/nxagent nxagent
		patchelf --set-rpath '$ORIGIN/../../exports/lib:$ORIGIN/../../../nxcomp/src/.libs:$ORIGIN/../../../nxcompshad/src/.libs' nxagent
		ln -snf nxagent x2goagent
	fi
)
