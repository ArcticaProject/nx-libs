#! /bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

autoreconf -v --install || exit 1
cd $ORIGDIR || exit $?

if [ "x$NOCONFIGURE" != "x1" ]; then
    if [ -n "${CONFIGURE}" ]; then
	$srcdir/${CONFIGURE}
    else
	$srcdir/configure --enable-maintainer-mode "$@"
    fi
fi
