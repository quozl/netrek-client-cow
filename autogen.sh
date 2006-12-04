#!/bin/sh
aclocal
libtoolize --copy
autoconf
chmod +x debian/rules debian/postinst debian/postrm debian/init
echo "autogen.sh completed ok"
