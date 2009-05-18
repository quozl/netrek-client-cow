#!/bin/sh
rm -f config.guess config.sub ltmain.sh
aclocal
libtoolize --copy
autoconf
chmod +x debian/rules tests/build
echo "autogen.sh completed ok"
