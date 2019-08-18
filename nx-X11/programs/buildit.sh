#!/bin/bash

set -x

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

build Xserver PKG_CONFIG_LIBDIR="$PKG_CONFIG_LIBDIR" || exit 1
