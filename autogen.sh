#!/bin/sh
rm -f config.guess config.sub ltmain.sh
aclocal
libtoolize --copy
if [ ! -f config.sub ]; then
    # later versions of libtool silently fail to create config.sub
    # unless --install is added, yet --install is not valid on the
    # older versions.  later versions also support --no-warn to reduce
    # output volume
    libtoolize --install --copy --no-warn
fi
autoconf
chmod +x debian/rules tests/build
echo "autogen.sh completed ok"
