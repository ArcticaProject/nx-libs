#!/bin/bash

set -x

NULL=

PKG_CONFIG_DEFAULT_LIBDIR=$( pkg-config --variable pc_path pkg-config)
export PKG_CONFIG_LIBDIR="../../../nxcomp:../../lib/:../../../nxcompshad:$PKG_CONFIG_DEFAULT_LIBDIR"

SRC=.
PRJ="${SRC}/$@"

build() {
	PKG=$1
	shift
	[ -d ${SRC}/$PKG ] || return 0

	OPT="$@"

	pushd "${SRC}/$PKG"
	if [ -e Makefile ]; then
		make distclean
	fi

	if [ -e configure ]; then
		rm -f configure
	fi

	if [ ! -e configure ]; then
		./autogen.sh $OPT || exit 1
	fi

	make

	popd
}

export CONFFLAGS="--enable-unix-transport		\
                  --enable-tcp-transport		\
                  --disable-kbd_mode			\
                  --enable-ipv6				\
                  --enable-secure-rpc			\
                  --enable-nxagent			\
                  ${NULL}"

export NXAGENTMODULES_LIBS="-L`pwd`/../exports/lib			  \
                            -L`pwd`/../../nxcomp/src/.libs		  \
                            -L`pwd`/../../nxcompshad/src/.libs		  \
                            -lXcomp -lXcompshad -lNX_X11		  \
                            ${NULL}"

build Xserver $CONFFLAGS --with-mesa-source=$PWD/${SRC}/../extras/Mesa/ PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR" || exit 1
