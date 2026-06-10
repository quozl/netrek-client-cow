#!/bin/sh
rm -f config.guess config.sub ltmain.sh
aclocal
libtoolize --copy >/dev/null 2>&1
if [ ! -f config.sub ]; then
    # later versions of libtool silently fail to create config.sub
    # unless --install is added, yet --install is not valid on the
    # older versions.
    libtoolize --install --copy >/dev/null 2>&1
fi
autoconf
chmod +x debian/rules tests/build
echo "autogen.sh completed ok"
