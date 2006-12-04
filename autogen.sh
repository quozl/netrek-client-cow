#!/bin/sh
aclocal
libtoolize --copy
autoconf
chmod +x debian/rules tests/build
echo "autogen.sh completed ok"
