#!/bin/sh

set -x
libtoolize --automake --copy
aclocal
autoconf
autoheader
automake --add-missing --foreign --copy
